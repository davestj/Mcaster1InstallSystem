#pragma once
/*
 * ComponentsEditor.h — Component dependency tree editor.
 * Phase 1: displays read-only list; Phase 2 adds dependency drag-connect.
 */
#include <QWidget>
#include "Manifest.h"

class QTreeWidget;

class ComponentsEditor : public QWidget
{
    Q_OBJECT
public:
    explicit ComponentsEditor(QWidget *parent = nullptr);
    void load(const Manifest &m);
    void save(Manifest &m) const;
private:
    QTreeWidget *m_tree = nullptr;
};
