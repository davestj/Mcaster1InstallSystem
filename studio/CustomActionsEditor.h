#pragma once
/*
 * CustomActionsEditor.h — Studio tab for editing custom install/uninstall actions.
 *
 * A custom action has:
 *   id        — machine identifier
 *   trigger   — when to run: before-install | after-install |
 *                            before-uninstall | after-uninstall
 *   type      — shell | script
 *   command   — the command/script to execute
 *   platforms — restrict to a subset of platforms (empty = all)
 */

#include <QWidget>
#include "Manifest.h"

class QTableWidget;
class QPushButton;
class QLabel;
class QComboBox;

class CustomActionsEditor : public QWidget
{
    Q_OBJECT

public:
    explicit CustomActionsEditor(QWidget *parent = nullptr);

    void load(const Manifest &m);
    void save(Manifest &m) const;

private slots:
    void onAdd();
    void onRemove();
    void onCellChanged(int row, int col);

private:
    void buildUi();
    void rebuildTable();
    void addTableRow(int row, const CustomAction &a);
    void collectRow(int row, CustomAction &a) const;

    // ── Data ─────────────────────────────────────────────────────────────
    QList<CustomAction> m_actions;
    bool                m_blockCellSignal = false;

    // ── Widgets ───────────────────────────────────────────────────────────
    QTableWidget *m_table     = nullptr;
    QPushButton  *m_btnAdd    = nullptr;
    QPushButton  *m_btnRemove = nullptr;
};
