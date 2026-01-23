#pragma once
/*
 * PrerequisitesEditor.h — Studio tab for editing installer prerequisites.
 *
 * A prerequisite has:
 *   id         — machine identifier
 *   name       — display name shown to the end-user
 *   checkCmd   — shell command: exit 0 = present, non-zero = missing
 *   installCmd — command / URL to install it (shown to user on failure)
 *   platforms  — restrict to a subset of platforms (empty = all)
 */

#include <QWidget>
#include "Manifest.h"

class QListWidget;
class QGroupBox;
class QLineEdit;
class QLabel;
class QPushButton;

class PrerequisitesEditor : public QWidget
{
    Q_OBJECT

public:
    explicit PrerequisitesEditor(QWidget *parent = nullptr);

    void load(const Manifest &m);
    void save(Manifest &m) const;

private slots:
    void onRowSelected(int row);
    void onAdd();
    void onRemove();

private:
    void buildUi();
    void flushCurrentRow();
    void populateRow(int row);

    // ── Data ─────────────────────────────────────────────────────────────
    QList<Prerequisite> m_prereqs;
    int                 m_currentRow = -1;

    // ── Widgets ───────────────────────────────────────────────────────────
    QListWidget *m_list       = nullptr;
    QGroupBox   *m_detail     = nullptr;
    QLineEdit   *m_id         = nullptr;
    QLineEdit   *m_name       = nullptr;
    QLineEdit   *m_checkCmd   = nullptr;
    QLineEdit   *m_installCmd = nullptr;
    QLineEdit   *m_platforms  = nullptr;
    QPushButton *m_btnAdd     = nullptr;
    QPushButton *m_btnRemove  = nullptr;
};
