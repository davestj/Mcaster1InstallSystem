#pragma once
/*
 * EventLog.h — Timestamped, color-coded event log dock widget
 *
 * Events are categorized by level and displayed in a QPlainTextEdit with
 * ANSI-style color coding using HTML-aware QTextCursor insertion.
 *
 * Signals consumed by StudioMainWindow:
 *   BuildPanel::buildLog(QString)       → EventLog::appendBuild(QString)
 *   BuildPanel::buildFinished(bool, QString) → EventLog::appendBuildResult(bool)
 */

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDateTime>
#include <QScrollBar>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QLabel>

class EventLog : public QWidget
{
    Q_OBJECT

public:
    enum Level { Info, Warn, Error, Build, Debug, Success };
    enum Theme { Dark, Enterprise };

    explicit EventLog(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *vbox = new QVBoxLayout(this);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->setSpacing(0);

        // ── Toolbar row ───────────────────────────────────────────────────────
        m_bar = new QWidget(this);
        m_bar->setFixedHeight(28);
        m_bar->setStyleSheet("background:#16213e; border-bottom:1px solid #0f3460;");
        auto *hb = new QHBoxLayout(m_bar);
        hb->setContentsMargins(6, 2, 6, 2);
        hb->setSpacing(6);

        auto *title = new QLabel("Event Log", m_bar);
        title->setStyleSheet("color:#888899; font-size:11px; font-weight:600;");
        hb->addWidget(title);
        hb->addStretch();

        m_btnClear = new QPushButton("Clear", m_bar);
        m_btnClear->setFixedHeight(20);
        m_btnClear->setStyleSheet(
            "QPushButton { background:#0f3460; color:#e0e0e8; border:none;"
            "  border-radius:3px; padding:0 8px; font-size:11px; }"
            "QPushButton:hover { background:#1a4a8a; }");
        connect(m_btnClear, &QPushButton::clicked, this, &EventLog::onClear);
        hb->addWidget(m_btnClear);

        auto *btnExport = new QPushButton("Export…", m_bar);
        btnExport->setFixedHeight(20);
        btnExport->setStyleSheet(m_btnClear->styleSheet());
        connect(btnExport, &QPushButton::clicked, this, &EventLog::onExport);
        hb->addWidget(btnExport);

        vbox->addWidget(m_bar);

        // ── Log view ──────────────────────────────────────────────────────────
        m_view = new QPlainTextEdit(this);
        m_view->setReadOnly(true);
        m_view->setMaximumBlockCount(10000);   // cap to avoid memory runaway
        m_view->setStyleSheet(
            "QPlainTextEdit {"
            "  background:#0a0a1a; color:#c0c0cc;"
            "  font-family: 'SF Mono', 'Menlo', 'Consolas', monospace;"
            "  font-size: 11px;"
            "  border: none;"
            "}");
        vbox->addWidget(m_view, 1);
    }

    // ── Append methods ────────────────────────────────────────────────────────
public slots:
    void append(const QString &msg, Level level = Info)
    {
        const QString ts   = QDateTime::currentDateTime().toString("HH:mm:ss");
        const QString pfx  = prefixFor(level);
        const QString col  = colorFor(level);
        const QString line = QString("[%1] %2 %3").arg(ts, pfx, msg.trimmed());

        // Use HTML to colorize; QPlainTextEdit supports setTextInteractionFlags but
        // we append via appendHtml for the colored prefix, then plain text remainder.
        QTextCursor c = m_view->textCursor();
        c.movePosition(QTextCursor::End);

        // Timestamp in dim grey
        QTextCharFormat fmtTs;
        fmtTs.setForeground(QColor("#555566"));
        c.setCharFormat(fmtTs);
        c.insertText(QString("[%1] ").arg(ts));

        // Level prefix in accent color
        QTextCharFormat fmtLvl;
        fmtLvl.setForeground(QColor(col));
        fmtLvl.setFontWeight(QFont::Bold);
        c.setCharFormat(fmtLvl);
        c.insertText(pfx + " ");

        // Message in default color
        QTextCharFormat fmtMsg;
        fmtMsg.setForeground(QColor(messageColorFor(level)));
        c.setCharFormat(fmtMsg);
        c.insertText(msg.trimmed() + "\n");

        m_view->setTextCursor(c);
        m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());

        m_plainLog << line;   // keep plain copy for export
    }

    void appendInfo   (const QString &msg) { append(msg, Info);    }
    void appendWarn   (const QString &msg) { append(msg, Warn);    }
    void appendError  (const QString &msg) { append(msg, Error);   }
    void appendBuild  (const QString &msg) { append(msg, Build);   }
    void appendDebug  (const QString &msg) { append(msg, Debug);   }
    void appendSuccess(const QString &msg) { append(msg, Success); }

    void appendBuildResult(bool ok, const QString &outputPath = {})
    {
        if (ok) {
            appendSuccess(QString("Build SUCCEEDED — output: %1").arg(outputPath));
        } else {
            appendError("Build FAILED");
        }
    }

private slots:
    void onClear()
    {
        m_view->clear();
        m_plainLog.clear();
    }

    void onExport()
    {
        const QString path = QFileDialog::getSaveFileName(
            this, "Export Event Log",
            QDir::homePath() + "/build-log.txt",
            "Text files (*.txt);;All files (*)");
        if (path.isEmpty()) return;
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << m_plainLog.join('\n') << '\n';
        }
    }

private:
    static const char *prefixFor(Level l) {
        switch (l) {
        case Info:    return "[INFO]   ";
        case Warn:    return "[WARN]   ";
        case Error:   return "[ERROR]  ";
        case Build:   return "[BUILD]  ";
        case Debug:   return "[DEBUG]  ";
        case Success: return "[OK]     ";
        }
        return "[INFO]   ";
    }
    static const char *colorFor(Level l) {
        switch (l) {
        case Info:    return "#4fc3f7";
        case Warn:    return "#ffb74d";
        case Error:   return "#ef5350";
        case Build:   return "#00c9ff";
        case Debug:   return "#888899";
        case Success: return "#66bb6a";
        }
        return "#4fc3f7";
    }
    static const char *messageColorFor(Level l) {
        switch (l) {
        case Error:   return "#ffcdd2";
        case Warn:    return "#fff9c4";
        case Success: return "#c8e6c9";
        default:      return "#c0c0cc";
        }
    }

    // ── Theme adaptation ──────────────────────────────────────────────────────
public:
    void setTheme(Theme t)
    {
        if (t == Enterprise) {
            m_bar->setStyleSheet("background:#e8e8e8; border-bottom:1px solid #c0c0c0;");
            const QString btnStyle =
                "QPushButton { background:#d0d0d0; color:#1a1a1a; border:1px solid #aaa;"
                "  border-radius:3px; padding:0 8px; font-size:11px; }"
                "QPushButton:hover { background:#c0c0c0; }";
            m_btnClear->setStyleSheet(btnStyle);
            m_view->setStyleSheet(
                "QPlainTextEdit {"
                "  background:#fafafa; color:#1a1a1a;"
                "  font-family: 'Menlo','Consolas',monospace; font-size:11px; border:none; }");
        } else {
            m_bar->setStyleSheet("background:#16213e; border-bottom:1px solid #0f3460;");
            const QString btnStyle =
                "QPushButton { background:#0f3460; color:#e0e0e8; border:none;"
                "  border-radius:3px; padding:0 8px; font-size:11px; }"
                "QPushButton:hover { background:#1a4a8a; }";
            m_btnClear->setStyleSheet(btnStyle);
            m_view->setStyleSheet(
                "QPlainTextEdit {"
                "  background:#0a0a1a; color:#c0c0cc;"
                "  font-family:'SF Mono','Menlo','Consolas',monospace;"
                "  font-size:11px; border:none; }");
        }
    }

    QWidget        *m_bar      = nullptr;
    QPushButton    *m_btnClear = nullptr;
    QPlainTextEdit *m_view     = nullptr;
    QStringList     m_plainLog;
};
