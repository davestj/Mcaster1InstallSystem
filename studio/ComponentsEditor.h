#pragma once
/*
 * ComponentsEditor.h — Component dependency and flags editor.
 *
 * Split-panel layout:
 *   Left  — QListWidget of all components (click to select)
 *   Right — Detail panel for the selected component:
 *             • Required / Selected checkboxes
 *             • Platforms field (comma-separated)
 *             • Depends-On checkboxes (all OTHER components)
 *
 * save() writes back required, selected, platforms, and depends for every
 * component into the manifest.
 */
#include <QWidget>
#include "Manifest.h"

class QListWidget;
class QListWidgetItem;
class QCheckBox;
class QLineEdit;
class QLabel;
class QGroupBox;

class ComponentsEditor : public QWidget
{
    Q_OBJECT
public:
    explicit ComponentsEditor(QWidget *parent = nullptr);
    void load(const Manifest &m);
    void save(Manifest &m) const;

private slots:
    void onComponentSelected(int row);

private:
    void buildUi();
    void populateDependsPanel(int compIndex);
    void flushCurrentComponent();

    // ── Data ──────────────────────────────────────────────────────────────
    QList<Component> m_components;
    int              m_currentRow = -1;

    // ── Widgets ───────────────────────────────────────────────────────────
    QListWidget *m_compList    = nullptr;

    // Detail panel (right side)
    QGroupBox   *m_detailGroup = nullptr;
    QLabel      *m_nameLabel   = nullptr;
    QLabel      *m_descLabel   = nullptr;
    QCheckBox   *m_chkRequired = nullptr;
    QCheckBox   *m_chkSelected = nullptr;
    QLineEdit   *m_platforms   = nullptr;   // comma-separated
    QGroupBox   *m_depsGroup   = nullptr;   // dependency checkboxes live here
};
