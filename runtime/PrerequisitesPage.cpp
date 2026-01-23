/*
 * PrerequisitesPage.cpp — Prerequisite check page for the installer wizard.
 */

#include "PrerequisitesPage.h"
#include "InstallerWizard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QCoreApplication>

// Column indices
enum Col { ColName = 0, ColStatus, ColAction, ColCount };

PrerequisitesPage::PrerequisitesPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Prerequisites");
    setSubTitle("The following software must be present before installation can continue.\n"
                "You may proceed even if some items are missing.");

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 8, 0, 0);
    vbox->setSpacing(10);

    // ── Table ─────────────────────────────────────────────────────────────
    m_table = new QTableWidget(0, ColCount, this);
    m_table->setHorizontalHeaderLabels({"Prerequisite", "Status", "Action"});
    m_table->horizontalHeader()->setSectionResizeMode(ColName,   QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColStatus, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(ColAction, QHeaderView::Fixed);
    m_table->horizontalHeader()->resizeSection(ColStatus, 100);
    m_table->horizontalHeader()->resizeSection(ColAction, 120);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    vbox->addWidget(m_table, 1);

    // ── Summary + recheck button ──────────────────────────────────────────
    auto *footer = new QHBoxLayout;

    m_summary = new QLabel(this);
    m_summary->setObjectName("hintLabel");
    footer->addWidget(m_summary, 1);

    m_btnCheck = new QPushButton("Recheck All", this);
    connect(m_btnCheck, &QPushButton::clicked, this, &PrerequisitesPage::onRecheckAll);
    footer->addWidget(m_btnCheck);

    vbox->addLayout(footer);
}

// ── QWizardPage overrides ─────────────────────────────────────────────────────
void PrerequisitesPage::initializePage()
{
    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) return;

    const auto &prereqs  = wiz->manifest().prerequisites;
    const QString curPlat = currentPlatform();

    // Build rows for prerequisites that apply to the current platform
    if (!m_populated) {
        m_populated = true;
        m_table->setRowCount(0);

        for (const Prerequisite &p : prereqs) {
            // Skip if this prereq is restricted to other platforms
            if (!p.platforms.isEmpty() && !p.platforms.contains(curPlat))
                continue;

            int row = m_table->rowCount();
            m_table->insertRow(row);

            // Name column
            auto *nameItem = new QTableWidgetItem(p.name.isEmpty() ? p.id : p.name);
            nameItem->setToolTip(p.checkCmd);
            m_table->setItem(row, ColName, nameItem);

            // Status column — will be filled by runChecks()
            auto *statusItem = new QTableWidgetItem("Checking...");
            statusItem->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, ColStatus, statusItem);

            // Action column — "Install" button (enabled only if missing)
            auto *btn = new QPushButton("Install", m_table);
            btn->setEnabled(false);
            btn->setToolTip(p.installCmd);
            const int capturedRow = row;
            connect(btn, &QPushButton::clicked, this, [this, capturedRow]() {
                onActionClicked(capturedRow);
            });
            m_table->setCellWidget(row, ColAction, btn);
        }
    }

    runChecks();
}

bool PrerequisitesPage::isComplete() const
{
    // Always allow the user to proceed — prerequisites are advisory
    return true;
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void PrerequisitesPage::onRecheckAll()
{
    // Reset status labels and re-run all checks
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (auto *item = m_table->item(row, ColStatus))
            item->setText("Checking...");
        if (auto *btn = qobject_cast<QPushButton *>(m_table->cellWidget(row, ColAction)))
            btn->setEnabled(false);
    }
    runChecks();
}

void PrerequisitesPage::onActionClicked(int row)
{
    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) return;

    const auto &prereqs  = wiz->manifest().prerequisites;
    const QString curPlat = currentPlatform();

    // Map table row back to prerequisite index (filtering by platform)
    int tableRow = 0;
    for (const Prerequisite &p : prereqs) {
        if (!p.platforms.isEmpty() && !p.platforms.contains(curPlat))
            continue;
        if (tableRow == row) {
            const QString &cmd = p.installCmd;
            if (cmd.startsWith("http://", Qt::CaseInsensitive) ||
                cmd.startsWith("https://", Qt::CaseInsensitive)) {
                QDesktopServices::openUrl(QUrl(cmd));
            } else if (!cmd.isEmpty()) {
                QMessageBox::information(this, "Install " + (p.name.isEmpty() ? p.id : p.name),
                    QString("Run the following command to install this prerequisite:\n\n%1")
                        .arg(cmd));
            } else {
                QMessageBox::information(this, "Install " + (p.name.isEmpty() ? p.id : p.name),
                    "No install command is configured for this prerequisite.");
            }
            return;
        }
        ++tableRow;
    }
}

// ── Private ───────────────────────────────────────────────────────────────────
void PrerequisitesPage::runChecks()
{
    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) return;

    const auto &prereqs   = wiz->manifest().prerequisites;
    const QString curPlat = currentPlatform();

    int tableRow = 0;
    int totalOk  = 0;
    int totalFail= 0;

    for (const Prerequisite &p : prereqs) {
        if (!p.platforms.isEmpty() && !p.platforms.contains(curPlat))
            continue;

        bool ok = false;
        if (!p.checkCmd.isEmpty()) {
            // Run the check command; exit 0 = present
#ifdef Q_OS_WIN
            int exitCode = QProcess::execute("cmd.exe", {"/c", p.checkCmd});
#else
            int exitCode = QProcess::execute("sh", {"-c", p.checkCmd});
#endif
            ok = (exitCode == 0);
        }

        updateStatusRow(tableRow, ok);
        ok ? ++totalOk : ++totalFail;

        // Allow UI to repaint between checks for responsiveness
        QCoreApplication::processEvents();

        ++tableRow;
    }

    // Update summary label
    if (tableRow == 0) {
        m_summary->setText("No prerequisites to check for this platform.");
    } else if (totalFail == 0) {
        m_summary->setText(QString("All %1 prerequisite(s) satisfied.").arg(totalOk));
    } else {
        m_summary->setText(
            QString("%1 of %2 prerequisite(s) missing — you may still continue, "
                    "but the application may not work correctly.")
                .arg(totalFail).arg(tableRow));
    }

    emit completeChanged();
}

void PrerequisitesPage::updateStatusRow(int row, bool ok)
{
    if (row < 0 || row >= m_table->rowCount()) return;

    if (auto *item = m_table->item(row, ColStatus)) {
        item->setText(ok ? "OK" : "Missing");
        item->setForeground(ok ? QColor("#4caf50") : QColor("#f44336"));
    }

    if (auto *btn = qobject_cast<QPushButton *>(m_table->cellWidget(row, ColAction)))
        btn->setEnabled(!ok);
}

QString PrerequisitesPage::currentPlatform() const
{
#ifdef Q_OS_MACOS
    return "macos";
#elif defined(Q_OS_WIN)
    return "windows";
#else
    return "linux";
#endif
}
