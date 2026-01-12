#pragma once
/*
 * ShortcutsEditor.h — Shortcut (launch icon) editor.
 * Creates desktop / Start Menu / Dock shortcuts for the installed app.
 */
#include <QWidget>
#include "Manifest.h"

class QTableWidget;
class QPushButton;

class ShortcutsEditor : public QWidget
{
    Q_OBJECT
public:
    explicit ShortcutsEditor(QWidget *parent = nullptr);
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
