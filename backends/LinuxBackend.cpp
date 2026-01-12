#include "LinuxBackend.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QFileInfo>

QStringList LinuxBackend::validate(const Manifest &m, const QString &projectDir) const
{
    Q_UNUSED(projectDir)
    return m.validate();
}

QString LinuxBackend::outputFilename(const Manifest &m) const
{
    return QString("%1-%2-%3-linux-amd64.deb")
        .arg(m.app.publisher.toLower().replace(' ', '-'))
        .arg(m.app.name.toLower().replace(' ', '-'))
        .arg(m.app.version);
}

bool LinuxBackend::build(const Manifest   &m,
                          const QString    &projectDir,
                          const QString    &outputDir,
                          const ProgressFn &progress,
                          QString          *errOut)
{
    report(progress, 5,  "=== Linux Build Started ===");
    report(progress, 10, QString("Package: %1 %2").arg(m.app.name, m.app.version));

    QDir().mkpath(outputDir);

    // ── Build .deb staging tree ───────────────────────────────────────────────
    QString stageDir = outputDir + "/_deb_stage";
    QString debianDir = stageDir + "/DEBIAN";
    QDir().mkpath(debianDir);

    report(progress, 20, "[1/3] Writing Debian control file...");
    QFile ctrlFile(debianDir + "/control");
    if (!ctrlFile.open(QIODevice::WriteOnly)) {
        if (errOut) *errOut = "Cannot write DEBIAN/control";
        return false;
    }
    ctrlFile.write(generateDebControl(m).toUtf8());
    ctrlFile.close();

    // Write postinst script
    QFile postinst(debianDir + "/postinst");
    if (postinst.open(QIODevice::WriteOnly)) {
        postinst.write("#!/bin/sh\nldconfig\n");
        postinst.setPermissions(postinst.permissions() | QFile::ExeOwner);
    }

    // ── Copy payload files ────────────────────────────────────────────────────
    report(progress, 35, "[2/3] Copying payload...");
    QString installDir = m.defaultInstallDir("linux");
    QString dstBase    = stageDir + installDir;
    QDir().mkpath(dstBase);

    QString payloadSrc = projectDir + "/payload";
    if (QDir(payloadSrc).exists()) {
        QProcess cp;
        cp.start("cp", {"-r", payloadSrc + "/.", dstBase + "/"});
        cp.waitForFinished(60000);
    } else {
        report(progress, 40, "  WARN: No payload/ directory found.");
    }

    // ── AppDir skeleton ────────────────────────────────────────────────────────
    report(progress, 50, "[2b] Building AppDir skeleton...");
    QString safeName = m.app.name;
    safeName.replace(' ', '_');
    QString appDir = outputDir + "/" + safeName + ".AppDir";
    buildAppDir(m, projectDir, appDir);
    report(progress, 60, "  AppDir: " + appDir);
    report(progress, 62, "  To create AppImage: appimagetool " + appDir);

    // ── Build .deb ────────────────────────────────────────────────────────────
    report(progress, 70, "[3/3] Building .deb package...");
    QString debPath = outputDir + "/" + outputFilename(m);
    if (!buildDebPackage(stageDir, debPath)) {
        report(progress, 75, "  WARN: dpkg-deb not available — stage tree ready at " + stageDir);
        report(progress, 80, "  To build manually: dpkg-deb --build " + stageDir + " " + debPath);
    } else {
        report(progress, 90, "  Package: " + debPath);
        QDir(stageDir).removeRecursively();
    }

    report(progress, 100, "=== Linux Build Complete ===");
    return true;
}

QString LinuxBackend::generateDebControl(const Manifest &m) const
{
    QString s;
    QTextStream ts(&s);
    ts << "Package: "      << m.app.name.toLower().replace(' ', '-') << "\n";
    ts << "Version: "      << m.app.version << "\n";
    ts << "Architecture: " << "amd64\n";
    ts << "Maintainer: "   << m.app.publisher << "\n";
    ts << "Description: "  << m.app.description << "\n";
    ts << "Homepage: "     << m.app.url << "\n";
    ts << "Priority: optional\n";
    ts << "Section: misc\n";

    qint64 installedSize = 0;  // TODO: calculate from payload
    ts << "Installed-Size: " << installedSize << "\n";
    return s;
}

bool LinuxBackend::buildDebPackage(const QString &stageDir, const QString &debPath) const
{
    QProcess which;
    which.start("which", {"dpkg-deb"});
    which.waitForFinished(3000);
    if (which.exitCode() != 0) return false;

    QProcess p;
    p.start("dpkg-deb", {"--build", stageDir, debPath});
    p.waitForFinished(60000);
    return p.exitCode() == 0;
}

bool LinuxBackend::buildAppDir(const Manifest &m, const QString &projectDir,
                                const QString &appDir) const
{
    QDir().mkpath(appDir + "/usr/bin");
    QDir().mkpath(appDir + "/usr/lib");
    QDir().mkpath(appDir + "/usr/share/applications");
    QDir().mkpath(appDir + "/usr/share/icons/hicolor/256x256/apps");

    // .desktop file
    QString pkgName = m.app.name.toLower().replace(' ', '-');
    QFile desktop(appDir + "/usr/share/applications/" + pkgName + ".desktop");
    if (desktop.open(QIODevice::WriteOnly)) {
        QTextStream ts(&desktop);
        ts << "[Desktop Entry]\n";
        ts << "Type=Application\n";
        ts << "Name=" << m.app.name << "\n";
        ts << "Comment=" << m.app.description << "\n";
        ts << "Exec=" << pkgName << "\n";
        ts << "Icon=" << pkgName << "\n";
        ts << "Categories=Utility;\n";
        ts << "Terminal=false\n";
    }

    // AppRun symlink
    QFile::link("usr/bin/" + pkgName, appDir + "/AppRun");

    Q_UNUSED(projectDir)
    return true;
}
