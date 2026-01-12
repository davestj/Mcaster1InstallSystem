#pragma once
/*
 * FilesEditor.h — Component and files editor.
 *
 * Left panel: component tree (QTreeWidget)
 * Right panel: file list for selected component (QTableWidget)
 *              + component properties (name, required, platforms)
 */

#include <QWidget>
#include "Manifest.h"

class QSplitter;
class QTreeWidget;
class QTreeWidgetItem;
class QTableWidget;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QPushButton;
class QLabel;

class FilesEditor : public QWidget
{
    Q_OBJECT

public:
    explicit FilesEditor(QWidget *parent = nullptr);

    void load(const Manifest &m);
    void save(Manifest &m) const;

private slots:
    void onComponentSelected();
    void onAddComponent();
    void onRemoveComponent();
    void onAddFile();
    void onRemoveFile();
    void onMoveUp();
    void onMoveDown();

private:
    void buildUi();
    void populateFileTable(int componentIndex);
    void flushComponentEdits();

    QSplitter    *m_splitter   = nullptr;

    // Left: component list
    QTreeWidget  *m_compTree   = nullptr;
    QPushButton  *m_btnAddComp = nullptr;
    QPushButton  *m_btnDelComp = nullptr;

    // Right top: component properties
    QGroupBox *m_compProps     = nullptr;
    QLineEdit *m_compId        = nullptr;
    QLineEdit *m_compName      = nullptr;
    QLineEdit *m_compDesc      = nullptr;
    QCheckBox *m_compRequired  = nullptr;
    QCheckBox *m_compSelected  = nullptr;

    // Right bottom: file list for selected component
    QTableWidget *m_fileTable  = nullptr;
    QPushButton  *m_btnAddFile = nullptr;
    QPushButton  *m_btnDelFile = nullptr;
    QPushButton  *m_btnUp      = nullptr;
    QPushButton  *m_btnDown    = nullptr;

    // In-memory copy for editing
    QList<Component> m_components;
    int m_currentComp = -1;
};
