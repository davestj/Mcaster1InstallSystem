/*
 * LicensePage.cpp — License agreement page
 */
#include "LicensePage.h"
#include "InstallerWizard.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QRadioButton>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

LicensePage::LicensePage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("License Agreement");
    setSubTitle("Please read and accept the license agreement before continuing.");

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 8, 0, 0);

    m_licenseText = new QTextEdit(this);
    m_licenseText->setReadOnly(true);
    m_licenseText->setMinimumHeight(240);
    vbox->addWidget(m_licenseText, 1);

    vbox->addSpacing(12);

    m_accept  = new QRadioButton("I accept the terms of the license agreement", this);
    m_decline = new QRadioButton("I do not accept the terms", this);
    m_decline->setChecked(true);

    vbox->addWidget(m_accept);
    vbox->addWidget(m_decline);

    connect(m_accept,  &QRadioButton::toggled, this, &LicensePage::completeChanged);
    connect(m_decline, &QRadioButton::toggled, this, &LicensePage::completeChanged);
}

void LicensePage::initializePage()
{
    auto *wiz = qobject_cast<InstallerWizard*>(wizard());
    if (!wiz) return;

    const QString &licFile = wiz->manifest().app.licenseFile;
    if (licFile.isEmpty()) {
        m_licenseText->setPlainText("No license file provided.");
        m_accept->setChecked(true);
        return;
    }

    // Try to load from payload dir or bundle Resources
    QStringList candidates{
        licFile,
        wiz->payloadDir() + "/" + licFile,
        QCoreApplication::applicationDirPath() + "/../Resources/" + licFile
    };

    for (const QString &path : candidates) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_licenseText->setPlainText(QTextStream(&f).readAll());
            return;
        }
    }
    m_licenseText->setPlainText(QString("License file not found: %1").arg(licFile));
}

bool LicensePage::isComplete() const
{
    return m_accept->isChecked();
}
