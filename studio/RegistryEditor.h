#pragma once
/*
 * RegistryEditor.h — Windows registry entries editor.
 * Shows all registry writes the installer will perform.
 */
#include <QWidget>
#include "Manifest.h"

class QTableWidget;
class QPushButton;

class RegistryEditor : public QWidget
{
    Q_OBJECT
public:
    explicit RegistryEditor(QWidget *parent = nullptr);
    void load(const Manifest &m);
    void save(Manifest &m) const;
private slots:
    void onAdd();
    void onRemove();
private:
    QTableWidget *m_table  = nullptr;
    QPushButton  *m_btnAdd = nullptr;
    QPushButton  *m_btnDel = nullptr;
};
