#pragma once
/*
 * BuildHistory.h — Persistent build record journal
 *
 * Every completed build (success or fail) is appended to a JSON file at:
 *   macOS/Linux: ~/.local/share/mcaster1/installstudio/build-history.json
 *   macOS alt:   ~/Library/Application Support/mcaster1/installstudio/build-history.json
 *
 * The panel shows records in a QTableWidget (newest first) with icon indicators.
 *
 * Usage:
 *   BuildHistory *bh = new BuildHistory(this);
 *   bh->load();
 *   // wire BuildPanel::buildFinished → bh->addRecord(...)
 *   // add bh as a dock or tab
 */

#include <QWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QMenu>
#include <QCursor>

// ─────────────────────────────────────────────────────────────────────────────
struct BuildRecord {
    QString   appName;
    QString   appVersion;
    QStringList platforms;       // targets that were built
    QDateTime timestamp;
    QString   outputPath;
    QString   codeSignStatus;    // "signed" | "unsigned" | "notarized"
    bool      success = false;

    QJsonObject toJson() const {
        QJsonObject o;
        o["appName"]        = appName;
        o["appVersion"]     = appVersion;
        o["platforms"]      = QJsonArray::fromStringList(platforms);
        o["timestamp"]      = timestamp.toString(Qt::ISODate);
        o["outputPath"]     = outputPath;
        o["codeSignStatus"] = codeSignStatus;
        o["success"]        = success;
        return o;
    }
    static BuildRecord fromJson(const QJsonObject &o) {
        BuildRecord r;
        r.appName        = o["appName"].toString();
        r.appVersion     = o["appVersion"].toString();
        for (const QJsonValue &v : o["platforms"].toArray())
            r.platforms << v.toString();
        r.timestamp      = QDateTime::fromString(o["timestamp"].toString(), Qt::ISODate);
        r.outputPath     = o["outputPath"].toString();
        r.codeSignStatus = o["codeSignStatus"].toString();
        r.success        = o["success"].toBool();
        return r;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
class BuildHistory : public QWidget
{
    Q_OBJECT

public:
    enum Theme { Dark, Enterprise };

    explicit BuildHistory(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *vbox = new QVBoxLayout(this);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->setSpacing(0);

        // ── Toolbar ───────────────────────────────────────────────────────────
        m_bar = new QWidget(this);
        m_bar->setFixedHeight(28);
        m_bar->setStyleSheet("background:#16213e; border-bottom:1px solid #0f3460;");
        auto *hb = new QHBoxLayout(m_bar);
        hb->setContentsMargins(6, 2, 6, 2);
        hb->setSpacing(6);

        auto *title = new QLabel("Build History", m_bar);
        title->setStyleSheet("color:#888899; font-size:11px; font-weight:600;");
        hb->addWidget(title);

        m_countLabel = new QLabel("0 records", m_bar);
        m_countLabel->setStyleSheet("color:#555566; font-size:10px;");
        hb->addWidget(m_countLabel);
        hb->addStretch();

        m_btnClear = new QPushButton("Clear", m_bar);
        m_btnClear->setFixedHeight(20);
        m_btnClear->setStyleSheet(
            "QPushButton { background:#0f3460; color:#e0e0e8; border:none;"
            "  border-radius:3px; padding:0 8px; font-size:11px; }"
            "QPushButton:hover { background:#7b1a1a; }");
        connect(m_btnClear, &QPushButton::clicked, this, [this]() {
            m_records.clear();
            refreshTable();
            save();
        });
        hb->addWidget(m_btnClear);
        vbox->addWidget(m_bar);

        // ── Table ─────────────────────────────────────────────────────────────
        m_table = new QTableWidget(0, 6, this);
        m_table->setHorizontalHeaderLabels({
            "", "App", "Version", "Platform(s)", "Date / Time", "Output"
        });
        m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
        m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
        m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
        m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Interactive);
        m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
        m_table->setColumnWidth(0, 28);
        m_table->setColumnWidth(1, 140);
        m_table->setColumnWidth(2, 80);
        m_table->setColumnWidth(3, 120);
        m_table->setColumnWidth(4, 140);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_table->setAlternatingRowColors(true);
        m_table->verticalHeader()->setVisible(false);
        m_table->setStyleSheet(
            "QTableWidget { background:#0a0a1a; color:#c0c0cc; gridline-color:#16213e; border:none; }"
            "QTableWidget::item:selected { background:#0f3460; }"
            "QHeaderView::section { background:#16213e; color:#888899; border:none;"
            "  border-right:1px solid #0f3460; padding:4px; font-size:11px; }");
        vbox->addWidget(m_table, 1);

        // ── Double-click: Reveal in Finder / File Manager ─────────────────
        connect(m_table, &QTableWidget::doubleClicked,
                this, [this](const QModelIndex &idx) {
            if (idx.row() < 0 || idx.row() >= m_records.size()) return;
            const QString path = m_records.at(idx.row()).outputPath;
            if (path.isEmpty()) return;
            // Use a file: URL to the parent directory so Finder/Nautilus selects the file
            const QFileInfo fi(path);
            const QString dir = fi.absoluteDir().absolutePath();
            QDesktopServices::openUrl(QUrl::fromLocalFile(
                fi.exists() ? dir : dir));
        });

        // ── Right-click context menu ───────────────────────────────────────
        m_table->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_table, &QTableWidget::customContextMenuRequested,
                this, [this](const QPoint &) {
            const int row = m_table->currentRow();
            if (row < 0 || row >= m_records.size()) return;
            const BuildRecord &r = m_records.at(row);

            QMenu menu;
            menu.addAction("Reveal in Finder", [&r]() {
                const QFileInfo fi(r.outputPath);
                if (!r.outputPath.isEmpty())
                    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absoluteDir().absolutePath()));
            });
            menu.addAction("Copy Path", [&r]() {
                if (!r.outputPath.isEmpty())
                    QApplication::clipboard()->setText(r.outputPath);
            });
            menu.addSeparator();
            menu.addAction("Remove This Record", [this, row]() {
                m_records.removeAt(row);
                refreshTable();
                save();
            });
            menu.exec(QCursor::pos());
        });
    }

    // ── Persistence ───────────────────────────────────────────────────────────
    bool load()
    {
        m_records.clear();
        QFile f(historyFilePath());
        if (!f.open(QIODevice::ReadOnly)) return false;
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        if (err.error != QJsonParseError::NoError) return false;
        for (const QJsonValue &v : doc.array())
            m_records.append(BuildRecord::fromJson(v.toObject()));
        refreshTable();
        return true;
    }

    bool save() const
    {
        const QString path = historyFilePath();
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        QJsonArray arr;
        for (const BuildRecord &r : m_records) arr.append(r.toJson());
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        return true;
    }

    static QString historyFilePath()
    {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        return dir + "/build-history.json";
    }

public slots:
    // Called by StudioMainWindow after a build finishes
    void addRecord(const BuildRecord &r)
    {
        m_records.prepend(r);   // newest first
        if (m_records.size() > 500) m_records.resize(500);   // cap
        refreshTable();
        save();
    }

private:
    void refreshTable()
    {
        m_table->setRowCount(0);
        for (int i = 0; i < m_records.size(); ++i) {
            const BuildRecord &r = m_records[i];
            m_table->insertRow(i);
            m_table->setRowHeight(i, 22);

            // Status icon cell
            auto *statusItem = new QTableWidgetItem(r.success ? "✓" : "✗");
            statusItem->setTextAlignment(Qt::AlignCenter);
            statusItem->setForeground(r.success ? QColor("#66bb6a") : QColor("#ef5350"));
            m_table->setItem(i, 0, statusItem);

            m_table->setItem(i, 1, new QTableWidgetItem(r.appName));
            m_table->setItem(i, 2, new QTableWidgetItem(r.appVersion));
            m_table->setItem(i, 3, new QTableWidgetItem(r.platforms.join(", ")));
            m_table->setItem(i, 4, new QTableWidgetItem(
                r.timestamp.toString("yyyy-MM-dd hh:mm:ss")));

            QString outText = r.outputPath;
            if (!r.codeSignStatus.isEmpty())
                outText += "  [" + r.codeSignStatus + "]";
            m_table->setItem(i, 5, new QTableWidgetItem(outText));
        }
        m_countLabel->setText(QString("%1 record%2")
            .arg(m_records.size()).arg(m_records.size() == 1 ? "" : "s"));
    }

    // ── Theme adaptation ──────────────────────────────────────────────────────
public:
    void setTheme(Theme t)
    {
        if (t == Enterprise) {
            m_bar->setStyleSheet("background:#e8e8e8; border-bottom:1px solid #c0c0c0;");
            m_btnClear->setStyleSheet(
                "QPushButton { background:#d0d0d0; color:#1a1a1a; border:1px solid #aaa;"
                "  border-radius:3px; padding:0 8px; font-size:11px; }"
                "QPushButton:hover { background:#c0c0c0; }");
            m_countLabel->setStyleSheet("color:#666677; font-size:10px;");
            m_table->setStyleSheet(
                "QTableWidget { background:#fafafa; color:#1a1a1a; gridline-color:#ddd; border:none; }"
                "QTableWidget::item:selected { background:#cce0ff; color:#000; }"
                "QHeaderView::section { background:#e8e8e8; color:#333; border:none;"
                "  border-right:1px solid #ccc; padding:4px; font-size:11px; }");
        } else {
            m_bar->setStyleSheet("background:#16213e; border-bottom:1px solid #0f3460;");
            m_btnClear->setStyleSheet(
                "QPushButton { background:#0f3460; color:#e0e0e8; border:none;"
                "  border-radius:3px; padding:0 8px; font-size:11px; }"
                "QPushButton:hover { background:#7b1a1a; }");
            m_countLabel->setStyleSheet("color:#555566; font-size:10px;");
            m_table->setStyleSheet(
                "QTableWidget { background:#0a0a1a; color:#c0c0cc; gridline-color:#16213e; border:none; }"
                "QTableWidget::item:selected { background:#0f3460; }"
                "QHeaderView::section { background:#16213e; color:#888899; border:none;"
                "  border-right:1px solid #0f3460; padding:4px; font-size:11px; }");
        }
    }

    QWidget        *m_bar        = nullptr;
    QPushButton    *m_btnClear   = nullptr;
    QTableWidget   *m_table      = nullptr;
    QLabel         *m_countLabel = nullptr;
    QList<BuildRecord> m_records;
};
