/*
 * StudioMainWindow.cpp — Mcaster1 Install Studio main window implementation
 */

#include "StudioMainWindow.h"
#include "ProjectSidebar.h"
#include "AppInfoEditor.h"
#include "FilesEditor.h"
#include "ComponentsEditor.h"
#include "ShortcutsEditor.h"
#include "RegistryEditor.h"
#include "SecurityEditor.h"
#include "PrerequisitesEditor.h"
#include "CustomActionsEditor.h"
#include "BuildPanel.h"
#include "EventLog.h"
#include "BuildHistory.h"
#include "HelpPanel.h"
#include "BuilderProfile.h"
#include "BuilderProfileDialog.h"
#include "SvgIcons.h"
#include "StudioStyle.h"
#include "NsisImporter.h"
#include "InnoSetupImporter.h"

#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QSplitter>
#include <QTabWidget>
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QLabel>
#include <QAction>
#include <QComboBox>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>
#include <QDateTime>
#include <QFrame>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QActionGroup>
#include <QInputDialog>
#include <QUuid>
#include <QRegularExpression>

// ── Constructor ───────────────────────────────────────────────────────────────
StudioMainWindow::StudioMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Mcaster1 Install Studio");
    setMinimumSize(1100, 700);
    resize(1280, 820);

    // Load builder profiles from disk (non-fatal if missing)
    m_profileManager.load();

    buildUi();
    buildMenuBar();
    buildToolBar();

    // Auto-save timer (fires 1500ms after inline name edit stops)
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);
    m_autoSaveTimer->setInterval(1500);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &StudioMainWindow::onAutoSave);

    // Clock — updates every second
    m_clockTimer = new QTimer(this);
    m_clockTimer->setInterval(1000);
    connect(m_clockTimer, &QTimer::timeout, this, &StudioMainWindow::onClockTick);
    m_clockTimer->start();
    onClockTick();   // populate immediately

    // Start with one blank project open
    {
        OpenProject op;
        op.manifest = Manifest::newProject();
        op.tempId   = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_openProjects.append(op);
        m_activeIdx = 0;
    }
    loadEditors();
    updateWindowTitle();
    updateProjectInfoLabel();

    // Initial HUD — idle
    updateHud(SvgIcons::kStatusIdle, "Build status: idle");
}

// ── Public ────────────────────────────────────────────────────────────────────
void StudioMainWindow::openProject(const QString &path)
{
    // Already open? Just switch to it.
    for (int i = 0; i < m_openProjects.size(); ++i) {
        if (m_openProjects[i].path == path) {
            switchToProject(i);
            return;
        }
    }

    OpenProject op;
    QString err;
    if (!op.manifest.load(path, &err)) {
        QMessageBox::warning(this, "Open Failed",
            QString("Could not load project:\n%1\n\n%2").arg(path, err));
        return;
    }
    op.path  = path;
    op.dirty = false;

    autoSaveActive();   // silently save current before switching
    m_openProjects.append(op);
    m_activeIdx = m_openProjects.size() - 1;
    loadEditors();
    updateWindowTitle();
    updateProjectInfoLabel();
    associateProjectWithProfile(path);
    m_eventLog->appendInfo(QString("Opened project: %1").arg(path));
    statusBar()->showMessage(QString("Opened: %1").arg(path), 4000);
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void StudioMainWindow::onNewProject()
{
    OpenProject op;
    op.manifest = Manifest::newProject();
    op.tempId   = QUuid::createUuid().toString(QUuid::WithoutBraces);
    op.dirty    = false;

    autoSaveActive();   // silently save current project before adding new one
    m_openProjects.append(op);
    m_activeIdx = m_openProjects.size() - 1;
    loadEditors();
    updateWindowTitle();
    updateProjectInfoLabel();
    m_editorTabs->setCurrentWidget(m_appInfo);
    m_eventLog->appendInfo("New project created.");
}

void StudioMainWindow::onOpenProject()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Open Project",
        QDir::homePath(),
        "Mcaster1 Install Spec (*.mis);;All files (*)");
    if (!path.isEmpty())
        openProject(path);
}

void StudioMainWindow::onSaveProject()
{
    if (!hasActive()) return;
    if (active().path.isEmpty()) {
        onSaveProjectAs();
        return;
    }
    collectEditors();
    QString err;
    if (!active().manifest.save(active().path, &err)) {
        QMessageBox::critical(this, "Save Failed",
            QString("Could not save:\n%1\n\n%2").arg(active().path, err));
        return;
    }
    active().dirty = false;
    updateWindowTitle();
    m_eventLog->appendInfo(QString("Saved: %1").arg(active().path));
    statusBar()->showMessage(QString("Saved: %1").arg(active().path), 3000);
    associateProjectWithProfile(active().path);
    refreshSidebar();
}

void StudioMainWindow::onSaveProjectAs()
{
    if (!hasActive()) return;
    const QString defaultName = sanitizeFilename(
        active().manifest.app.name.isEmpty() ? "NewProject" : active().manifest.app.name);
    QString path = QFileDialog::getSaveFileName(
        this, "Save Project As",
        active().path.isEmpty()
            ? QDir::homePath() + "/" + defaultName + ".mis"
            : active().path,
        "Mcaster1 Install Spec (*.mis);;All files (*)");

    if (path.isEmpty()) return;
    if (!path.endsWith(".mis", Qt::CaseInsensitive))
        path += ".mis";

    active().path = path;
    onSaveProject();
}

void StudioMainWindow::onBuildStart()
{
    if (!hasActive()) return;
    m_editorTabs->setCurrentWidget(m_build);
    collectEditors();

    updateHud(SvgIcons::kStatusBuilding, "Build in progress…");
    m_eventLog->appendBuild(
        QString("Starting build for %1 v%2")
            .arg(active().manifest.app.name, active().manifest.app.version));

    // Push the active builder profile's signing mode into the build panel.
    // index 0 = "— no profile —"; indices 1..N = profiles[index-1].
    const int pIdx = m_profileCombo ? m_profileCombo->currentIndex() - 1 : -1;
    if (pIdx >= 0 && pIdx < m_profileManager.profiles().size())
        m_build->applyProfile(m_profileManager.profiles().at(pIdx));

    m_build->startBuild(active().manifest,
                        active().path.isEmpty()
                            ? QDir::homePath()
                            : QFileInfo(active().path).absolutePath());
}

void StudioMainWindow::onImportNsis()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Import NSIS Script",
        QDir::homePath(),
        "NSIS Script (*.nsi *.nsh);;All files (*)");
    if (path.isEmpty()) return;

    NsisImporter imp;
    if (!imp.parse(path)) {
        QMessageBox::warning(this, "Import Failed",
            QString("NSIS import errors:\n%1").arg(imp.errors().join('\n')));
        return;
    }
    if (!imp.warnings().isEmpty()) {
        QMessageBox::information(this, "Import Warnings",
            QString("Imported with warnings:\n%1").arg(imp.warnings().join('\n')));
    }
    {
        OpenProject op;
        op.manifest = imp.manifest();
        op.tempId   = QUuid::createUuid().toString(QUuid::WithoutBraces);
        op.dirty    = true;
        autoSaveActive();
        m_openProjects.append(op);
        m_activeIdx = m_openProjects.size() - 1;
    }
    loadEditors();
    updateWindowTitle();
    updateProjectInfoLabel();
    m_eventLog->appendInfo(QString("NSIS script imported: %1").arg(path));
    statusBar()->showMessage("NSIS script imported.", 4000);
}

void StudioMainWindow::onImportInno()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Import Inno Setup Script",
        QDir::homePath(),
        "Inno Setup Script (*.iss);;All files (*)");
    if (path.isEmpty()) return;

    InnoSetupImporter imp;
    if (!imp.parse(path)) {
        QMessageBox::warning(this, "Import Failed",
            QString("Inno Setup import errors:\n%1").arg(imp.errors().join('\n')));
        return;
    }
    if (!imp.warnings().isEmpty()) {
        QMessageBox::information(this, "Import Warnings",
            QString("Imported with warnings:\n%1").arg(imp.warnings().join('\n')));
    }
    {
        OpenProject op;
        op.manifest = imp.manifest();
        op.tempId   = QUuid::createUuid().toString(QUuid::WithoutBraces);
        op.dirty    = true;
        autoSaveActive();
        m_openProjects.append(op);
        m_activeIdx = m_openProjects.size() - 1;
    }
    loadEditors();
    updateWindowTitle();
    updateProjectInfoLabel();
    m_eventLog->appendInfo(QString("Inno Setup script imported: %1").arg(path));
    statusBar()->showMessage("Inno Setup script imported.", 4000);
}

void StudioMainWindow::onAbout()
{
    QMessageBox::about(this,
        "About Mcaster1 Install Studio",
        "<h2 style='color:#00c9ff;'>Mcaster1 Install Studio</h2>"
        "<p style='color:#e0e0e8;'>Version 1.0.0</p>"
        "<p style='color:#888899;'>A cross-platform installer authoring studio for<br>"
        "macOS, Windows and Linux applications.</p>"
        "<p style='color:#888899;'>Copyright &copy; 2025 Mcaster1</p>"
        "<p><a style='color:#00c9ff;' href='https://mcaster1.com'>mcaster1.com</a></p>");
}

void StudioMainWindow::onOpenHelp()
{
    QStringList candidates = {
        QCoreApplication::applicationDirPath() + "/../Resources/docs/index.html",
        QCoreApplication::applicationDirPath() + "/docs/index.html",
        QDir::current().filePath("docs/index.html"),
    };
    for (const QString &p : candidates) {
        if (QFile::exists(p)) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(p));
            return;
        }
    }
    QMessageBox::information(this, "Documentation Not Found",
        "The help documentation could not be located.\n\n"
        "Expected: docs/index.html relative to the application or project root.\n\n"
        "See README.md for build instructions.");
}

void StudioMainWindow::onOpenBuildHistory()
{
    if (m_historyDock) {
        m_historyDock->show();
        m_historyDock->raise();
    }
}

// ── Builder Profile slots ─────────────────────────────────────────────────────
void StudioMainWindow::onProfileChanged(int index)
{
    if (index <= 0) return;   // index 0 = "— no profile —"
    const int profileIdx = index - 1;
    const auto &profiles = m_profileManager.profiles();
    if (profileIdx >= profiles.size()) return;

    const BuilderProfile &p = profiles.at(profileIdx);

    // Ask before overwriting
    auto ans = QMessageBox::question(this, "Apply Builder Profile",
        QString("Apply profile <b>%1</b> to the current project?<br><br>"
                "This will fill in publisher, website, support URL and signing fields.")
            .arg(p.displayName),
        QMessageBox::Yes | QMessageBox::No);
    if (ans != QMessageBox::Yes) {
        m_profileCombo->setCurrentIndex(0);
        return;
    }

    // Apply profile → active manifest, then reload editors
    if (!hasActive()) { m_profileCombo->setCurrentIndex(0); return; }
    p.applyToManifest(active().manifest);
    m_appInfo->load(active().manifest);
    m_security->load(active().manifest);
    active().dirty = true;
    updateWindowTitle();
    updateProjectInfoLabel();
    m_eventLog->appendInfo(QString("Applied builder profile: %1").arg(p.displayName));
    statusBar()->showMessage(QString("Profile applied: %1").arg(p.displayName), 3000);

    // Show this profile's project list in the sidebar
    refreshSidebar();

    // Keep combo on the selected profile — it is now the active build profile.
    // The signing mode (skipSigning / devSignMode) is applied at build-time by
    // BuildPanel::applyProfile(); sync the label now so it's visible immediately.
    m_build->applyProfile(p);
}

void StudioMainWindow::onManageProfiles()
{
    // Show a simple manager: list of profiles, Add / Edit / Delete buttons
    // For now we open an Add dialog; full manager can be a follow-on
    QDialog mgr(this);
    mgr.setWindowTitle("Manage Builder Profiles");
    mgr.setMinimumWidth(500);
    mgr.setMinimumHeight(400);

    auto *vbox = new QVBoxLayout(&mgr);

    auto *list = new QListWidget(&mgr);
    for (const BuilderProfile &p : m_profileManager.profiles())
        list->addItem(p.displayName.isEmpty() ? p.id : p.displayName);
    vbox->addWidget(list, 1);

    auto *hbox = new QHBoxLayout;
    auto *btnAdd    = new QPushButton("Add…",    &mgr);
    auto *btnEdit   = new QPushButton("Edit…",   &mgr);
    auto *btnDelete = new QPushButton("Delete",  &mgr);
    auto *btnClose  = new QPushButton("Close",   &mgr);
    hbox->addWidget(btnAdd);
    hbox->addWidget(btnEdit);
    hbox->addWidget(btnDelete);
    hbox->addStretch();
    hbox->addWidget(btnClose);
    vbox->addLayout(hbox);

    connect(btnClose, &QPushButton::clicked, &mgr, &QDialog::accept);

    connect(btnAdd, &QPushButton::clicked, &mgr, [&]() {
        BuilderProfileDialog dlg(&mgr);
        if (dlg.exec() != QDialog::Accepted) return;
        m_profileManager.add(dlg.profile());
        m_profileManager.save();
        list->addItem(dlg.profile().displayName);
        refreshProfileCombo();
    });

    connect(btnEdit, &QPushButton::clicked, &mgr, [&]() {
        const int row = list->currentRow();
        if (row < 0 || row >= m_profileManager.profiles().size()) return;
        BuilderProfileDialog dlg(&mgr);
        dlg.setProfile(m_profileManager.profiles().at(row));
        if (dlg.exec() != QDialog::Accepted) return;
        m_profileManager.update(row, dlg.profile());
        m_profileManager.save();
        list->item(row)->setText(dlg.profile().displayName);
        refreshProfileCombo();
    });

    connect(btnDelete, &QPushButton::clicked, &mgr, [&]() {
        const int row = list->currentRow();
        if (row < 0 || row >= m_profileManager.profiles().size()) return;
        auto ans = QMessageBox::question(&mgr, "Delete Profile",
            QString("Delete profile \"%1\"?").arg(m_profileManager.profiles().at(row).displayName));
        if (ans != QMessageBox::Yes) return;
        m_profileManager.remove(row);
        m_profileManager.save();
        delete list->takeItem(row);
        refreshProfileCombo();
    });

    mgr.exec();
}

// ── Clock slot ────────────────────────────────────────────────────────────────
void StudioMainWindow::onClockTick()
{
    if (m_clockLabel)
        m_clockLabel->setText(
            QDateTime::currentDateTime().toString("  ddd yyyy-MM-dd  hh:mm:ss  "));
}

// ── Tab changed ───────────────────────────────────────────────────────────────
void StudioMainWindow::onTabChanged(int /*index*/)
{
    // Sync sidebar selection to active tab when needed
}

void StudioMainWindow::onApplicationSelected(const QString &appGroupId)
{
    if (hasActive()) {
        active().activeAppGroupId = appGroupId;
        // future: m_files->selectAppGroup(appGroupId); m_components->selectAppGroup(appGroupId);
    }
    m_sidebar->setActiveAppGroup(appGroupId);
    m_editorTabs->setCurrentWidget(m_files);
}

void StudioMainWindow::onProjectModified()
{
    if (m_loading || !hasActive()) return;
    if (!active().dirty) {
        active().dirty = true;
        updateWindowTitle();
    }
    m_autoSaveTimer->start();   // debounce → onAutoSave → autoSaveActive()
}

void StudioMainWindow::onProjectInfoChanged()
{
    updateProjectInfoLabel();
}

// ── Build signal handlers ─────────────────────────────────────────────────────
void StudioMainWindow::onBuildLog(const QString &line)
{
    m_eventLog->appendBuild(line);
    m_logDock->show();
    m_logDock->raise();
}

void StudioMainWindow::onBuildProgress(int pct, const QString &msg)
{
    statusBar()->showMessage(QString("[%1%] %2").arg(pct).arg(msg));
}

void StudioMainWindow::onBuildFinished(bool ok, const QString &outputPath)
{
    if (ok) {
        m_eventLog->appendSuccess(
            QString("Build SUCCESS — output: %1").arg(outputPath));
        updateHud(SvgIcons::kStatusOk, "Last build: SUCCESS");
        statusBar()->showMessage("Build finished successfully.", 8000);
    } else {
        m_eventLog->appendError("Build FAILED — see event log for details.");
        updateHud(SvgIcons::kStatusFail, "Last build: FAILED");
        statusBar()->showMessage("Build failed.", 8000);
    }

    // Record in build history
    if (hasActive()) {
        const Manifest &m = active().manifest;
        BuildRecord rec;
        rec.appName        = m.app.name;
        rec.appVersion     = m.app.version;
        rec.platforms      = m.targets;
        rec.timestamp      = QDateTime::currentDateTime();
        rec.outputPath     = outputPath;
        rec.codeSignStatus = m.signing.macosSigner.isEmpty() ? "unsigned" : "signed";
        rec.success        = ok;
        m_buildHistory->addRecord(rec);
    }
}

// ── Protected ─────────────────────────────────────────────────────────────────
void StudioMainWindow::closeEvent(QCloseEvent *event)
{
    // Auto-save every dirty project that has a path; just let the rest go
    collectEditors();   // flush the active project's UI state first
    for (OpenProject &op : m_openProjects) {
        if (!op.dirty || op.path.isEmpty()) continue;
        QString err;
        op.manifest.save(op.path, &err);
    }
    event->accept();
}

// ── Private helpers ───────────────────────────────────────────────────────────
void StudioMainWindow::buildUi()
{
    // ── Splitter (sidebar | editor tabs) ──────────────────────────────────
    m_splitter   = new QSplitter(Qt::Horizontal, this);
    m_sidebar    = new ProjectSidebar(m_splitter);
    m_editorTabs = new QTabWidget(m_splitter);
    m_editorTabs->setDocumentMode(true);
    m_editorTabs->setTabPosition(QTabWidget::North);

    m_splitter->addWidget(m_sidebar);
    m_splitter->addWidget(m_editorTabs);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({240, 1040});

    setCentralWidget(m_splitter);

    // ── Editor tabs ──────────────────────────────────────────────────────
    m_appInfo    = new AppInfoEditor(m_editorTabs);
    m_files      = new FilesEditor(m_editorTabs);
    m_components = new ComponentsEditor(m_editorTabs);
    m_shortcuts  = new ShortcutsEditor(m_editorTabs);
    m_registry   = new RegistryEditor(m_editorTabs);
    m_security   = new SecurityEditor(m_editorTabs);
    m_prereqs    = new PrerequisitesEditor(m_editorTabs);
    m_actions    = new CustomActionsEditor(m_editorTabs);
    m_build      = new BuildPanel(m_editorTabs);

    auto si = [this](const char *svg) { return svgIcon(svg); };

    m_editorTabs->addTab(m_appInfo,    si(SvgIcons::kPackage),  "App Info");
    m_editorTabs->addTab(m_files,      si(SvgIcons::kFolder),   "Files");
    m_editorTabs->addTab(m_components, si(SvgIcons::kLinux),    "Components");
    m_editorTabs->addTab(m_shortcuts,  si(SvgIcons::kMacOs),    "Shortcuts");
    m_editorTabs->addTab(m_registry,   si(SvgIcons::kWindows),  "Registry");
    m_editorTabs->addTab(m_security,   si(SvgIcons::kKey),      "Security");
    m_editorTabs->addTab(m_prereqs,    si(SvgIcons::kPrereqs),  "Prerequisites");
    m_editorTabs->addTab(m_actions,    si(SvgIcons::kActions),  "Actions");
    m_editorTabs->addTab(m_build,      si(SvgIcons::kBuild),    "Build");

    // Tab tooltips
    m_editorTabs->setTabToolTip(0, "App metadata: name, version, publisher, install directories");
    m_editorTabs->setTabToolTip(1, "Files and payload items to be installed");
    m_editorTabs->setTabToolTip(2, "Optional components the user can choose during install");
    m_editorTabs->setTabToolTip(3, "Desktop and Start Menu shortcuts to create");
    m_editorTabs->setTabToolTip(4, "Windows registry entries to write during install");
    m_editorTabs->setTabToolTip(5, "Code signing, HTTP headers and security options");
    m_editorTabs->setTabToolTip(6, "Prerequisites checked before install begins");
    m_editorTabs->setTabToolTip(7, "Custom shell/script actions triggered at install stages");
    m_editorTabs->setTabToolTip(8, "Build and package the installer for selected targets");

    connect(m_editorTabs, &QTabWidget::currentChanged, this, &StudioMainWindow::onTabChanged);
    connect(m_sidebar, &ProjectSidebar::newProjectRequested,
            this, &StudioMainWindow::onNewProject);
    connect(m_sidebar, &ProjectSidebar::openProjectRequested,
            this, &StudioMainWindow::onOpenProject);
    connect(m_sidebar, &ProjectSidebar::switchProjectRequested,
            this, &StudioMainWindow::onSwitchProject);
    connect(m_sidebar, &ProjectSidebar::closeProjectRequested,
            this, &StudioMainWindow::onCloseProject);
    connect(m_sidebar, &ProjectSidebar::copyProjectRequested,
            this, &StudioMainWindow::onCopyProject);
    connect(m_sidebar, &ProjectSidebar::deleteProjectRequested,
            this, &StudioMainWindow::onDeleteProject);
    connect(m_sidebar, &ProjectSidebar::addApplicationRequested,
            this, &StudioMainWindow::onAddApplication);
    connect(m_sidebar, &ProjectSidebar::deleteApplicationRequested,
            this, &StudioMainWindow::onDeleteApplication);
    connect(m_sidebar, &ProjectSidebar::renameProjectRequested,
            this, &StudioMainWindow::onRenameProject);
    connect(m_sidebar, &ProjectSidebar::applicationSelected,
            this, &StudioMainWindow::onApplicationSelected);

    // ── Connect build signals ────────────────────────────────────────────
    // buildRequested: user clicked "Build Installer" inside the panel — route
    // through onBuildStart() so collectEditors() runs before startBuild().
    connect(m_build, &BuildPanel::buildRequested, this, &StudioMainWindow::onBuildStart);
    connect(m_build, &BuildPanel::buildLog,       this, &StudioMainWindow::onBuildLog);
    connect(m_build, &BuildPanel::buildProgress,  this, &StudioMainWindow::onBuildProgress);
    connect(m_build, &BuildPanel::buildFinished, this, &StudioMainWindow::onBuildFinished);

    // ── Connect editor modified() signals ─────────────────────────────────
    // AppInfoEditor: modified() → onProjectModified; projectInfoChanged → status bar
    connect(m_appInfo, &AppInfoEditor::modified,
            this, &StudioMainWindow::onProjectModified);
    connect(m_appInfo, &AppInfoEditor::projectInfoChanged,
            this, [this](const QString &name, const QString &) {
                onProjectInfoChanged();
                // Also trigger auto-save + sidebar sync + file rename when name changes
                onProjectNameChanged(name);
            });

    // FilesEditor: modified() → onProjectModified
    connect(m_files, &FilesEditor::modified,
            this, &StudioMainWindow::onProjectModified);

    // ── EventLog dock (bottom) ───────────────────────────────────────────
    m_logDock = new QDockWidget("Event Log", this);
    m_logDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    m_logDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_logDock->setMinimumHeight(130);

    m_eventLog = new EventLog(m_logDock);
    m_logDock->setWidget(m_eventLog);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);
    m_logDock->hide();

    // ── BuildHistory dock (right) ────────────────────────────────────────
    m_historyDock = new QDockWidget("Build History", this);
    m_historyDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_historyDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    m_historyDock->setMinimumWidth(380);

    m_buildHistory = new BuildHistory(m_historyDock);
    m_buildHistory->load();
    m_historyDock->setWidget(m_buildHistory);
    addDockWidget(Qt::RightDockWidgetArea, m_historyDock);
    m_historyDock->hide();

    // ── Help panel dock ───────────────────────────────────────────────────
    m_helpDock = new QDockWidget("Help & Documentation", this);
    m_helpDock->setFeatures(
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable  |
        QDockWidget::DockWidgetFloatable);
    m_helpDock->setAllowedAreas(
        Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea | Qt::BottomDockWidgetArea);
    m_helpDock->setMinimumWidth(360);

    m_helpPanel = new HelpPanel(m_helpDock);
    m_helpDock->setWidget(m_helpPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_helpDock);
    m_helpDock->hide();

    // ── Status bar ───────────────────────────────────────────────────────
    m_projectInfoLabel = new QLabel("Project: (new)");
    m_projectInfoLabel->setStyleSheet("color:#888899; font-size:11px; padding:0 8px;");
    statusBar()->addWidget(m_projectInfoLabel);   // left side (stretches)

    // Separator
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color:#333355;");
    statusBar()->addPermanentWidget(sep);

    m_studioVersionLabel = new QLabel("Mcaster1 Install Studio v1.0.0");
    m_studioVersionLabel->setStyleSheet("color:#555577; font-size:11px; padding:0 8px;");
    statusBar()->addPermanentWidget(m_studioVersionLabel);
}

void StudioMainWindow::buildMenuBar()
{
    // ── File ─────────────────────────────────────────────────────────────
    QMenu *file = menuBar()->addMenu("&File");

    m_actNew = new QAction(svgIcon(SvgIcons::kNew), "&New Project", this);
    m_actNew->setShortcut(QKeySequence::New);
    m_actNew->setToolTip("Create a new empty installer project");
    connect(m_actNew, &QAction::triggered, this, &StudioMainWindow::onNewProject);
    file->addAction(m_actNew);

    m_actOpen = new QAction(svgIcon(SvgIcons::kOpen), "&Open Project...", this);
    m_actOpen->setShortcut(QKeySequence::Open);
    m_actOpen->setToolTip("Open an existing .mis installer project file");
    connect(m_actOpen, &QAction::triggered, this, &StudioMainWindow::onOpenProject);
    file->addAction(m_actOpen);

    file->addSeparator();

    m_actSave = new QAction(svgIcon(SvgIcons::kSave), "&Save", this);
    m_actSave->setShortcut(QKeySequence::Save);
    m_actSave->setToolTip("Save the current project");
    connect(m_actSave, &QAction::triggered, this, &StudioMainWindow::onSaveProject);
    file->addAction(m_actSave);

    m_actSaveAs = new QAction(svgIcon(SvgIcons::kSave), "Save &As...", this);
    m_actSaveAs->setShortcut(QKeySequence::SaveAs);
    m_actSaveAs->setToolTip("Save the current project to a new file");
    connect(m_actSaveAs, &QAction::triggered, this, &StudioMainWindow::onSaveProjectAs);
    file->addAction(m_actSaveAs);

    file->addSeparator();

    QMenu *importMenu = file->addMenu(svgIcon(SvgIcons::kImport), "&Import");
    m_actImportNsis = new QAction(svgIcon(SvgIcons::kImport), "&NSIS Script (.nsi)...", this);
    connect(m_actImportNsis, &QAction::triggered, this, &StudioMainWindow::onImportNsis);
    importMenu->addAction(m_actImportNsis);

    m_actImportInno = new QAction(svgIcon(SvgIcons::kImport), "&Inno Setup Script (.iss)...", this);
    connect(m_actImportInno, &QAction::triggered, this, &StudioMainWindow::onImportInno);
    importMenu->addAction(m_actImportInno);

    file->addSeparator();

    QAction *quitAct = new QAction(svgIcon(SvgIcons::kQuit), "&Quit", this);
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, qApp, &QCoreApplication::quit);
    file->addAction(quitAct);

    // ── Build ─────────────────────────────────────────────────────────────
    QMenu *build = menuBar()->addMenu("&Build");
    m_actBuild = new QAction(svgIcon(SvgIcons::kBuild), "&Build Installer...", this);
    m_actBuild->setShortcut(QKeySequence("Ctrl+B"));
    m_actBuild->setToolTip("Build the installer package for selected target platforms");
    connect(m_actBuild, &QAction::triggered, this, &StudioMainWindow::onBuildStart);
    build->addAction(m_actBuild);

    build->addSeparator();

    QAction *histAct = new QAction(svgIcon(SvgIcons::kHistory), "Build &History...", this);
    histAct->setToolTip("View past build records");
    connect(histAct, &QAction::triggered, this, &StudioMainWindow::onOpenBuildHistory);
    build->addAction(histAct);

    // ── Profiles ──────────────────────────────────────────────────────────
    QMenu *profiles = menuBar()->addMenu("&Profiles");

    QAction *mgAct = new QAction(svgIcon(SvgIcons::kProfile), "&Manage Profiles...", this);
    mgAct->setToolTip("Create, edit or delete builder identity profiles");
    connect(mgAct, &QAction::triggered, this, &StudioMainWindow::onManageProfiles);
    profiles->addAction(mgAct);

    // ── View ──────────────────────────────────────────────────────────────
    QMenu *view = menuBar()->addMenu("&View");

    QAction *logAct = new QAction(svgIcon(SvgIcons::kLog), "Event &Log", this);
    logAct->setShortcut(QKeySequence("Ctrl+L"));
    connect(logAct, &QAction::triggered, this, [this]() {
        m_logDock->setVisible(!m_logDock->isVisible());
        if (m_logDock->isVisible()) m_logDock->raise();
    });
    view->addAction(logAct);

    QAction *bhAct = new QAction(svgIcon(SvgIcons::kHistory), "Build &History", this);
    connect(bhAct, &QAction::triggered, this, &StudioMainWindow::onOpenBuildHistory);
    view->addAction(bhAct);

    view->addSeparator();

    // ── Theme submenu ──────────────────────────────────────────────────────
    QMenu *themeMenu = view->addMenu(svgIcon(SvgIcons::kSettings, 16), "&Theme");

    m_actThemeDark = new QAction("Mcaster1 &Dark", this);
    m_actThemeDark->setCheckable(true);
    m_actThemeDark->setToolTip("Deep-blue dark palette (default Mcaster1 brand theme)");

    m_actThemeEnterprise = new QAction("&Enterprise Professional", this);
    m_actThemeEnterprise->setCheckable(true);
    m_actThemeEnterprise->setToolTip("White background, nickel buttons, enterprise-grade 3D chrome look");

    // Exclusive action group — acts like radio buttons
    auto *themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);
    themeGroup->addAction(m_actThemeDark);
    themeGroup->addAction(m_actThemeEnterprise);

    themeMenu->addAction(m_actThemeDark);
    themeMenu->addAction(m_actThemeEnterprise);

    // Reflect the currently active theme
    const auto cur = StudioStyle::ThemeManager::currentTheme();
    m_actThemeDark->setChecked(cur == StudioStyle::ThemeManager::Dark);
    m_actThemeEnterprise->setChecked(cur == StudioStyle::ThemeManager::Enterprise);

    connect(m_actThemeDark, &QAction::triggered, this, [this]() {
        onThemeChanged(StudioStyle::ThemeManager::Dark);
    });
    connect(m_actThemeEnterprise, &QAction::triggered, this, [this]() {
        onThemeChanged(StudioStyle::ThemeManager::Enterprise);
    });

    // ── Help ──────────────────────────────────────────────────────────────
    QMenu *help = menuBar()->addMenu("&Help");

    QAction *helpAct = new QAction(svgIcon(SvgIcons::kAbout), "&Documentation...", this);
    helpAct->setShortcut(QKeySequence("F1"));
    helpAct->setToolTip("Open the Mcaster1 Install System help documentation");
    connect(helpAct, &QAction::triggered, this, &StudioMainWindow::onOpenHelp);
    help->addAction(helpAct);

    QAction *helpPanelAct = new QAction(svgIcon(SvgIcons::kAbout), "Show &Help Panel", this);
    helpPanelAct->setShortcut(QKeySequence("Ctrl+Shift+H"));
    helpPanelAct->setCheckable(true);
    helpPanelAct->setToolTip("Toggle the inline help & documentation panel (Ctrl+Shift+H)");
    connect(helpPanelAct, &QAction::triggered, this, &StudioMainWindow::onToggleHelpPanel);
    // Keep action check-state in sync when the dock is closed by the user
    connect(m_helpDock, &QDockWidget::visibilityChanged,
            helpPanelAct, &QAction::setChecked);
    help->addAction(helpPanelAct);

    QAction *planAct = new QAction(svgIcon(SvgIcons::kSettings), "Phase &Tracker...", this);
    planAct->setToolTip("View the development phase planning tracker");
    connect(planAct, &QAction::triggered, this, []() {
        QStringList candidates = {
            QCoreApplication::applicationDirPath() + "/../Resources/docs/PLANNING.html",
            QCoreApplication::applicationDirPath() + "/docs/PLANNING.html",
            QDir::current().filePath("docs/PLANNING.html"),
        };
        for (const QString &p : candidates) {
            if (QFile::exists(p)) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(p));
                return;
            }
        }
    });
    help->addAction(planAct);

    help->addSeparator();
    QAction *aboutAct = new QAction(svgIcon(SvgIcons::kAbout), "&About Mcaster1 Install Studio", this);
    connect(aboutAct, &QAction::triggered, this, &StudioMainWindow::onAbout);
    help->addAction(aboutAct);
}

void StudioMainWindow::buildToolBar()
{
    // ── Toolbar 1: primary actions + HUD ─────────────────────────────────
    QToolBar *tb = addToolBar("Main");
    tb->setIconSize(QSize(20, 20));
    tb->setMovable(false);

    // Logo
    QLabel *logo = new QLabel(tb);
    logo->setPixmap(svgIcon(SvgIcons::kLogo, 32).pixmap(32, 32));
    logo->setContentsMargins(6, 0, 10, 0);
    logo->setToolTip("Mcaster1 Install Studio");
    tb->addWidget(logo);

    tb->addAction(m_actNew);
    tb->addAction(m_actOpen);
    tb->addAction(m_actSave);
    tb->addSeparator();
    tb->addAction(m_actBuild);
    m_actBuild->setObjectName("buildBtn");

    // Spacer (pushes HUD to the right)
    auto *spacer1 = new QWidget(tb);
    spacer1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->addWidget(spacer1);

    // ── HUD: build status icon ────────────────────────────────────────────
    m_hudIcon = new QLabel(tb);
    m_hudIcon->setFixedSize(22, 22);
    m_hudIcon->setToolTip("Build status: idle");
    tb->addWidget(m_hudIcon);

    // ── Clock ─────────────────────────────────────────────────────────────
    m_clockLabel = new QLabel(tb);
    m_clockLabel->setStyleSheet(
        "color:#888899; font-size:11px; font-family:'SF Mono','Menlo','Consolas',monospace;"
        "padding:0 8px;");
    m_clockLabel->setToolTip("Current date and time");
    tb->addWidget(m_clockLabel);

    // ── Toolbar 2: Builder Profile selector ───────────────────────────────
    QToolBar *tb2 = addToolBar("Profile");
    tb2->setIconSize(QSize(16, 16));
    tb2->setMovable(false);

    QLabel *profileIcon = new QLabel(tb2);
    profileIcon->setPixmap(svgIcon(SvgIcons::kProfile, 18).pixmap(18, 18));
    profileIcon->setContentsMargins(8, 0, 4, 0);
    profileIcon->setToolTip("Builder Profile — apply company/publisher identity to the project");
    tb2->addWidget(profileIcon);

    QLabel *profileLbl = new QLabel("Profile:", tb2);
    profileLbl->setStyleSheet("color:#888899; font-size:11px;");
    profileLbl->setContentsMargins(0, 0, 6, 0);
    tb2->addWidget(profileLbl);

    m_profileCombo = new QComboBox(tb2);
    m_profileCombo->setMinimumWidth(220);
    m_profileCombo->setMaximumWidth(320);
    m_profileCombo->setToolTip(
        "Select a builder profile to auto-populate publisher, signing identity\n"
        "and contact fields for the current project.");
    refreshProfileCombo();
    connect(m_profileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StudioMainWindow::onProfileChanged);
    tb2->addWidget(m_profileCombo);

    QAction *mgAct = new QAction(svgIcon(SvgIcons::kSettings, 16), "Manage…", this);
    mgAct->setToolTip("Add, edit or delete builder profiles");
    connect(mgAct, &QAction::triggered, this, &StudioMainWindow::onManageProfiles);
    tb2->addAction(mgAct);

    // Info label on right of profile bar
    auto *spacer2 = new QWidget(tb2);
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb2->addWidget(spacer2);

    QLabel *infoLbl = new QLabel(
        "Builder profiles auto-fill publisher, signing and contact fields.", tb2);
    infoLbl->setStyleSheet("color:#555577; font-size:10px; padding:0 8px;");
    tb2->addWidget(infoLbl);
}

void StudioMainWindow::updateWindowTitle()
{
    if (!hasActive()) { setWindowTitle("Mcaster1 Install Studio"); return; }
    const OpenProject &op = active();
    QString name = op.manifest.app.name.isEmpty() ? "Untitled Project" : op.manifest.app.name;
    if (op.dirty) name.prepend("* ");
    if (!op.path.isEmpty())
        name += QString(" — %1").arg(QFileInfo(op.path).fileName());
    setWindowTitle(name + " — Mcaster1 Install Studio");
}

void StudioMainWindow::updateProjectInfoLabel()
{
    if (!hasActive() || !m_projectInfoLabel) return;
    const Manifest &m = active().manifest;
    const QString appName = m.app.name.isEmpty() ? "(untitled)" : m.app.name;
    const QString appVer  = m.app.version.isEmpty() ? "" : " v" + m.app.version;
    m_projectInfoLabel->setText(QString("Project: %1%2").arg(appName, appVer));
}

void StudioMainWindow::updateHud(const char *iconSvg, const QString &tip)
{
    if (!m_hudIcon) return;
    const QPixmap px = svgIcon(iconSvg, 22).pixmap(22, 22);
    m_hudIcon->setPixmap(px);
    m_hudIcon->setToolTip(tip);
}

void StudioMainWindow::refreshProfileCombo()
{
    if (!m_profileCombo) return;
    QSignalBlocker block(m_profileCombo);
    m_profileCombo->clear();
    m_profileCombo->addItem("— no profile —");
    for (const BuilderProfile &p : m_profileManager.profiles())
        m_profileCombo->addItem(p.displayName.isEmpty() ? p.id : p.displayName);
}

void StudioMainWindow::loadEditors()
{
    if (!hasActive()) return;
    m_loading = true;
    const Manifest &m = active().manifest;
    m_appInfo   ->load(m);
    m_files     ->load(m);
    m_components->load(m);
    m_shortcuts ->load(m);
    m_registry  ->load(m);
    m_security  ->load(m);
    m_prereqs   ->load(m);
    m_actions   ->load(m);
    refreshSidebar();
    m_loading = false;
}

void StudioMainWindow::collectEditors()
{
    if (!hasActive()) return;
    Manifest &m = active().manifest;
    m_appInfo   ->save(m);
    m_files     ->save(m);
    m_components->save(m);
    m_shortcuts ->save(m);
    m_registry  ->save(m);
    m_security  ->save(m);
    m_prereqs   ->save(m);
    m_actions   ->save(m);
}

// ── Theme switching ───────────────────────────────────────────────────────────
void StudioMainWindow::onThemeChanged(StudioStyle::ThemeManager::ThemeId id)
{
    StudioStyle::ThemeManager::applyTheme(id, qApp);
    StudioStyle::ThemeManager::saveTheme(id);

    // Update checkmarks
    if (m_actThemeDark)       m_actThemeDark->setChecked(id == StudioStyle::ThemeManager::Dark);
    if (m_actThemeEnterprise) m_actThemeEnterprise->setChecked(id == StudioStyle::ThemeManager::Enterprise);

    // Re-render all icons for the new theme
    rebuildIcons();

    // Adjust any hardcoded style strings in status bar / toolbars that
    // are colour-specific — easier to just regenerate them inline.
    if (m_projectInfoLabel) {
        m_projectInfoLabel->setStyleSheet(
            id == StudioStyle::ThemeManager::Enterprise
                ? "color:#3a3a3a; font-size:11px; padding:0 8px;"
                : "color:#888899; font-size:11px; padding:0 8px;");
    }
    if (m_studioVersionLabel) {
        m_studioVersionLabel->setStyleSheet(
            id == StudioStyle::ThemeManager::Enterprise
                ? "color:#5a5a5a; font-size:11px; padding:0 8px;"
                : "color:#555577; font-size:11px; padding:0 8px;");
    }
    if (m_clockLabel) {
        m_clockLabel->setStyleSheet(
            id == StudioStyle::ThemeManager::Enterprise
                ? "color:#3a3a3a; font-size:11px; "
                  "font-family:'Consolas','SF Mono','Menlo',monospace; padding:0 8px;"
                : "color:#888899; font-size:11px; "
                  "font-family:'SF Mono','Menlo','Consolas',monospace; padding:0 8px;");
    }

    // Push theme into docked widgets that have hardcoded palette styles
    const EventLog::Theme eTheme =
        (id == StudioStyle::ThemeManager::Enterprise)
            ? EventLog::Enterprise : EventLog::Dark;
    if (m_eventLog)    m_eventLog->setTheme(eTheme);
    if (m_buildHistory) m_buildHistory->setTheme(
        (id == StudioStyle::ThemeManager::Enterprise)
            ? BuildHistory::Enterprise : BuildHistory::Dark);

    m_eventLog->appendInfo(
        QString("Theme changed to: %1")
            .arg(StudioStyle::ThemeManager::themeName(id)));
    statusBar()->showMessage(
        QString("Theme: %1").arg(StudioStyle::ThemeManager::themeName(id)), 4000);
}

void StudioMainWindow::rebuildIcons()
{
    auto si = [this](const char *svg, int sz = 24) { return svgIcon(svg, sz); };

    // ── Toolbar action icons ──────────────────────────────────────────────
    if (m_actNew)        m_actNew       ->setIcon(si(SvgIcons::kNew));
    if (m_actOpen)       m_actOpen      ->setIcon(si(SvgIcons::kOpen));
    if (m_actSave)       m_actSave      ->setIcon(si(SvgIcons::kSave));
    if (m_actSaveAs)     m_actSaveAs    ->setIcon(si(SvgIcons::kSave));
    if (m_actBuild)      m_actBuild     ->setIcon(si(SvgIcons::kBuild));
    if (m_actImportNsis) m_actImportNsis->setIcon(si(SvgIcons::kImport));
    if (m_actImportInno) m_actImportInno->setIcon(si(SvgIcons::kImport));

    // ── Tab icons ─────────────────────────────────────────────────────────
    if (m_editorTabs) {
        m_editorTabs->setTabIcon(0, si(SvgIcons::kPackage));
        m_editorTabs->setTabIcon(1, si(SvgIcons::kFolder));
        m_editorTabs->setTabIcon(2, si(SvgIcons::kLinux));
        m_editorTabs->setTabIcon(3, si(SvgIcons::kMacOs));
        m_editorTabs->setTabIcon(4, si(SvgIcons::kWindows));
        m_editorTabs->setTabIcon(5, si(SvgIcons::kKey));
        m_editorTabs->setTabIcon(6, si(SvgIcons::kPrereqs));
        m_editorTabs->setTabIcon(7, si(SvgIcons::kActions));
        m_editorTabs->setTabIcon(8, si(SvgIcons::kBuild));
    }

    // ── HUD status icon — re-render with same tip ─────────────────────────
    if (m_hudIcon) {
        // Re-render whichever HUD state is current by forcing a no-op update.
        // The next build event will naturally update it; for now just re-apply
        // the idle icon.
        updateHud(SvgIcons::kStatusIdle, m_hudIcon->toolTip());
    }
}

QIcon StudioMainWindow::svgIcon(const char *svgStr, int size)
{
    // Recolour the SVG for the active theme (Enterprise needs dark strokes on white)
    const QByteArray data = StudioStyle::ThemeManager::recolorIcon(svgStr);
    QSvgRenderer renderer(data);
    QIcon icon;
    // Render at multiple sizes so Qt picks the sharpest for each context
    for (int sz : {size, size * 2}) {
        QPixmap px(sz, sz);
        px.fill(Qt::transparent);
        QPainter p(&px);
        renderer.render(&p);
        p.end();
        icon.addPixmap(px);
    }
    return icon;
}

// ── Project name / auto-save ──────────────────────────────────────────────────

void StudioMainWindow::onProjectNameChanged(const QString &name)
{
    if (!hasActive()) return;
    if (name.isEmpty() || name == active().manifest.app.name) return;
    active().manifest.app.name = name;
    m_appInfo->setAppName(name);       // QSignalBlocker inside — no feedback loop
    m_sidebar->setActiveProjectName(name);   // keep tree node in sync
    updateWindowTitle();
    updateProjectInfoLabel();
    active().dirty = true;
    m_autoSaveTimer->start();          // fires onAutoSave in 1500ms
}

void StudioMainWindow::autoSaveActive()
{
    if (!hasActive() || !active().dirty || active().path.isEmpty()) return;
    collectEditors();
    QString err;
    if (active().manifest.save(active().path, &err))
        active().dirty = false;
}

void StudioMainWindow::switchToProject(int index)
{
    if (index == m_activeIdx) return;
    if (index < 0 || index >= m_openProjects.size()) return;
    autoSaveActive();
    m_activeIdx = index;
    loadEditors();
    updateWindowTitle();
    updateProjectInfoLabel();
}

void StudioMainWindow::onAutoSave()
{
    if (!hasActive()) return;
    OpenProject &op = active();

    // For brand-new unsaved projects, auto-save to ~/Documents/Mcaster1Projects/
    if (op.path.isEmpty()) {
        if (op.manifest.app.name.isEmpty()) return;   // no name yet, nothing to do
        const QString dir = QDir::homePath() + "/Documents/Mcaster1Projects";
        QDir().mkpath(dir);
        const QString base = sanitizeFilename(op.manifest.app.name);
        QString candidate = dir + "/" + base + ".mis";
        int n = 2;
        while (QFile::exists(candidate))
            candidate = dir + "/" + base + "_" + QString::number(n++) + ".mis";
        op.path = candidate;
        m_eventLog->appendInfo(QString("Auto-saving new project to: %1").arg(op.path));
    }

    // Attempt file rename if app name changed
    const QString base    = sanitizeFilename(
        op.manifest.app.name.isEmpty() ? "NewProject" : op.manifest.app.name);
    const QFileInfo fi(op.path);
    const QString   newPath = fi.dir().absoluteFilePath(base + ".mis");

    if (newPath != op.path && !QFile::exists(newPath)) {
        if (QFile::rename(op.path, newPath)) {
            // Update stored path in active profile
            const int pIdx = m_profileCombo->currentIndex() - 1;
            if (pIdx >= 0 && pIdx < m_profileManager.profiles().size()) {
                BuilderProfile &p = m_profileManager.profiles()[pIdx];
                const int pi = p.projectPaths.indexOf(op.path);
                if (pi >= 0) p.projectPaths[pi] = newPath;
                m_profileManager.save();
            }
            op.path = newPath;
        }
    }

    collectEditors();
    QString err;
    if (op.manifest.save(op.path, &err)) {
        op.dirty = false;
        updateWindowTitle();
        refreshSidebar();
        const QString fname = QFileInfo(op.path).fileName();
        m_eventLog->appendInfo(QString("Auto-saved: %1").arg(fname));
        statusBar()->showMessage(QString("Auto-saved: %1").arg(fname), 3000);
    }
}

// ── Sidebar action slots ──────────────────────────────────────────────────────

void StudioMainWindow::onAddApplication()
{
    if (!hasActive()) return;
    bool ok;
    const QString name = QInputDialog::getText(
        this, "Add Application",
        "New application name:",
        QLineEdit::Normal,
        QString("App %1").arg(active().manifest.appGroups.size() + 1),
        &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    AppGroup g;
    g.id   = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    g.name = name.trimmed();
    active().manifest.appGroups.append(g);
    loadEditors();
    m_editorTabs->setCurrentWidget(m_files);
    onProjectModified();
    m_eventLog->appendInfo(QString("Added application: %1").arg(g.name));
}

void StudioMainWindow::onSwitchProject(const QString &path)
{
    // Check if already open in the session
    for (int i = 0; i < m_openProjects.size(); ++i) {
        if (m_openProjects[i].path == path) {
            switchToProject(i);
            return;
        }
    }
    openProject(path);
}

void StudioMainWindow::onCloseProject(const QString &path)
{
    // Find the project in the open list
    int idx = -1;
    for (int i = 0; i < m_openProjects.size(); ++i) {
        if (m_openProjects[i].path == path) { idx = i; break; }
    }
    if (idx < 0) return;

    // Collect + save before removing
    if (idx == m_activeIdx) collectEditors();
    if (m_openProjects[idx].dirty && !m_openProjects[idx].path.isEmpty()) {
        QString err;
        m_openProjects[idx].manifest.save(m_openProjects[idx].path, &err);
    }

    m_openProjects.removeAt(idx);

    // Always keep at least one project open
    if (m_openProjects.isEmpty()) {
        OpenProject op;
        op.manifest = Manifest::newProject();
        op.tempId   = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_openProjects.append(op);
        m_activeIdx = 0;
    } else {
        if (idx < m_activeIdx)
            --m_activeIdx;
        if (m_activeIdx >= m_openProjects.size())
            m_activeIdx = m_openProjects.size() - 1;
    }

    loadEditors();
    updateWindowTitle();
    updateProjectInfoLabel();
    refreshSidebar();
    m_eventLog->appendInfo(
        QString("Closed project: %1").arg(QFileInfo(path).fileName()));
}

// ── New project context-menu actions ─────────────────────────────────────────

void StudioMainWindow::onRenameProject(const QString &path, const QString &newName)
{
    if (newName.isEmpty()) return;

    // Find the project — empty path = unsaved, fall back to active
    int idx = m_activeIdx;
    if (!path.isEmpty()) {
        for (int i = 0; i < m_openProjects.size(); ++i) {
            if (m_openProjects[i].path == path) { idx = i; break; }
        }
    }
    if (idx < 0 || idx >= m_openProjects.size()) return;
    OpenProject &op = m_openProjects[idx];
    if (newName == op.manifest.app.name) return;

    op.manifest.app.name = newName;
    op.dirty = true;

    if (idx == m_activeIdx) {
        m_appInfo->setAppName(newName);
        updateWindowTitle();
        updateProjectInfoLabel();
        m_autoSaveTimer->start();  // debounced auto-save for active project
    } else {
        // Non-active project — save immediately
        if (!op.path.isEmpty()) {
            const QString base    = sanitizeFilename(newName);
            const QFileInfo fi(op.path);
            const QString   newPath = fi.dir().absoluteFilePath(base + ".mis");
            if (newPath != op.path && !QFile::exists(newPath)) {
                if (QFile::rename(op.path, newPath)) {
                    const int pIdx = m_profileCombo->currentIndex() - 1;
                    if (pIdx >= 0 && pIdx < m_profileManager.profiles().size()) {
                        BuilderProfile &p = m_profileManager.profiles()[pIdx];
                        const int pi = p.projectPaths.indexOf(op.path);
                        if (pi >= 0) p.projectPaths[pi] = newPath;
                        m_profileManager.save();
                    }
                    op.path = newPath;
                }
            }
            QString err;
            op.manifest.save(op.path, &err);
            op.dirty = false;
        }
        refreshSidebar();
    }
}

void StudioMainWindow::onCopyProject(const QString &path)
{
    // Find the source project (must be a saved project — empty path not copyable yet)
    const OpenProject *src = nullptr;
    for (const OpenProject &op : m_openProjects) {
        if (op.path == path) { src = &op; break; }
    }
    if (!src) return;

    bool ok;
    const QString newName = QInputDialog::getText(
        this, "Copy Project",
        "Name for the copied project:",
        QLineEdit::Normal,
        src->manifest.app.name + " Copy",
        &ok);
    if (!ok || newName.trimmed().isEmpty()) return;

    OpenProject copy;
    copy.manifest           = src->manifest;   // deep copy
    copy.manifest.app.name  = newName.trimmed();
    copy.tempId             = QUuid::createUuid().toString(QUuid::WithoutBraces);
    copy.dirty              = true;

    autoSaveActive();
    m_openProjects.append(copy);
    m_activeIdx = m_openProjects.size() - 1;
    loadEditors();
    updateWindowTitle();
    updateProjectInfoLabel();
    m_editorTabs->setCurrentWidget(m_appInfo);
    m_autoSaveTimer->start();   // will create the .mis file on first auto-save
    m_eventLog->appendInfo(
        QString("Copied project: \"%1\" \u2192 \"%2\"")
            .arg(src->manifest.app.name, newName.trimmed()));
}

void StudioMainWindow::onDeleteProject(const QString &path)
{
    if (path.isEmpty()) return;

    // First close from session (handles active index adjustment + creates blank if last)
    onCloseProject(path);

    // Then delete the file from disk
    if (QFile::exists(path)) {
        QFile::remove(path);
        // Remove from any profile that tracks it
        const int pIdx = m_profileCombo->currentIndex() - 1;
        if (pIdx >= 0 && pIdx < m_profileManager.profiles().size()) {
            m_profileManager.profiles()[pIdx].projectPaths.removeAll(path);
            m_profileManager.save();
        }
        m_eventLog->appendInfo(
            QString("Deleted project: %1").arg(QFileInfo(path).fileName()));
    }
}

void StudioMainWindow::onDeleteApplication(const QString &appGroupId)
{
    if (!hasActive()) return;
    Manifest &m = active().manifest;
    for (int i = 0; i < m.appGroups.size(); ++i) {
        if (m.appGroups[i].id == appGroupId) {
            const QString name = m.appGroups[i].name;
            m.appGroups.removeAt(i);
            // Clear active app if it was the deleted one
            if (active().activeAppGroupId == appGroupId)
                active().activeAppGroupId.clear();
            loadEditors();
            onProjectModified();
            m_eventLog->appendInfo(QString("Deleted application: %1").arg(name));
            return;
        }
    }
}

// ── Profile–project association helpers ──────────────────────────────────────

void StudioMainWindow::associateProjectWithProfile(const QString &path)
{
    if (path.isEmpty()) return;
    const int pIdx = m_profileCombo->currentIndex() - 1;
    if (pIdx < 0 || pIdx >= m_profileManager.profiles().size()) return;
    BuilderProfile &p = m_profileManager.profiles()[pIdx];
    if (!p.projectPaths.contains(path)) {
        p.projectPaths.append(path);
        m_profileManager.save();
        refreshSidebar();
    }
}

void StudioMainWindow::refreshSidebar()
{
    if (!m_sidebar) return;

    // Build the sidebar tree directly from the in-memory open project list.
    // All projects are fully loaded — no disk reads needed.
    QList<ProjectSidebar::ProjectEntry> entries;
    for (int i = 0; i < m_openProjects.size(); ++i) {
        const OpenProject &op = m_openProjects[i];
        ProjectSidebar::ProjectEntry e;
        e.path            = op.path;
        e.isActive        = (i == m_activeIdx);
        e.activeAppGroupId = op.activeAppGroupId;
        e.name     = op.manifest.app.name.isEmpty()
                     ? (op.path.isEmpty() ? "New Project" : QFileInfo(op.path).baseName())
                     : op.manifest.app.name;
        for (const AppGroup &g : op.manifest.appGroups) {
            e.appGroupIds   << g.id;
            e.appGroupNames << (g.name.isEmpty() ? g.id : g.name);
        }
        entries.append(e);
    }

    m_sidebar->refresh(entries);
}

void StudioMainWindow::onToggleHelpPanel()
{
    if (!m_helpDock) return;
    const bool show = !m_helpDock->isVisible();
    m_helpDock->setVisible(show);
    if (show) {
        m_helpDock->raise();
        m_helpDock->setFocus();
    }
}

QString StudioMainWindow::sanitizeFilename(const QString &name)
{
    QString s = name.trimmed();
    static const QRegularExpression kBad(R"([\\/:*?"<>|])");
    s.replace(kBad, "_");
    if (s.isEmpty()) s = "NewProject";
    return s;
}
