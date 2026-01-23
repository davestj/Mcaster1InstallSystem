#pragma once
/*
 * ComponentsPage.h — Optional component selection page.
 *
 * Builds one checkbox row per Manifest::Component.
 * Required components are pre-checked and disabled.
 * validatePage() pushes the selection to InstallerWizard::setSelectedComponents().
 */
#include <QWizardPage>
#include <QList>

class QScrollArea;
class QVBoxLayout;
class QCheckBox;

class ComponentsPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit ComponentsPage(QWidget *parent = nullptr);
    void initializePage() override;
    bool isComplete()    const override;
    bool validatePage()        override;

private:
    QStringList collectSelectedIds() const;

    QScrollArea  *m_scroll      = nullptr;
    QVBoxLayout  *m_checkLayout = nullptr;
    QList<QCheckBox *> m_checks;   // parallel to manifest().components
    bool          m_populated   = false;
};
