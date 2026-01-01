#include <QApplication>
#include "InstallerWizard.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Mcaster1InstallSystem");
    app.setOrganizationName("Mcaster1");
    app.setOrganizationDomain("mcaster1.com");
    app.setApplicationVersion("2.5.3-beta");

    InstallerWizard wiz;
    wiz.show();

    return app.exec();
}
