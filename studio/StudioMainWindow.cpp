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
#include "BuildPanel.h"
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
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QIcon>

// ── Constructor ───────────────────────────────────────────────────────────────
StudioMainWindow::StudioMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Mcaster1 Install Studio");
    setMinimumSize(1100, 700);
    resize(1280, 800);

    buildUi();
    buildMenuBar();
    buildToolBar();

    // Start with a blank project
    m_manifest = Manifest::newProject();
    loadEditors();
    updateWindowTitle();
}

// ── Public ────────────────────────────────────────────────────────────────────
void StudioMainWindow::openProject(const QString &path)
{
    if (!confirmDiscard()) return;

    QString err;
    if (!m_manifest.load(path, &err)) {
        QMessageBox::warning(this, "Open Failed",
            QString("Could not load project:\n%1\n\n%2").arg(path, err));
        return;
    }
    m_projectPath = path;
    m_dirty       = false;
    loadEditors();
    updateWindowTitle();

    statusBar()->showMessage(QString("Opened: %1").arg(path), 4000);
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void StudioMainWindow::onNewProject()
{
    if (!confirmDiscard()) return;
    m_manifest    = Manifest::newProject();
    m_projectPath.clear();
    m_dirty       = false;
    loadEditors();
    updateWindowTitle();
}

void StudioMainWindow::onOpenProject()
{
    if (!confirmDiscard()) return;
    QString path = QFileDialog::getOpenFileName(
        this, "Open Project",
        QDir::homePath(),
        "Mcaster1 Install Spec (*.mis);;All files (*)");
    if (!path.isEmpty())
        openProject(path);
}

void StudioMainWindow::onSaveProject()
{
    if (m_projectPath.isEmpty()) {
        onSaveProjectAs();
        return;
    }
    collectEditors();
    QString err;
    if (!m_manifest.save(m_projectPath, &err)) {
        QMessageBox::critical(this, "Save Failed",
            QString("Could not save:\n%1\n\n%2").arg(m_projectPath, err));
        return;
    }
    m_dirty = false;
    updateWindowTitle();
    statusBar()->showMessage(QString("Saved: %1").arg(m_projectPath), 3000);
}

void StudioMainWindow::onSaveProjectAs()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Save Project As",
        m_projectPath.isEmpty()
            ? QDir::homePath() + "/NewProject.mis"
            : m_projectPath,
        "Mcaster1 Install Spec (*.mis);;All files (*)");

    if (path.isEmpty()) return;
    if (!path.endsWith(".mis", Qt::CaseInsensitive))
        path += ".mis";

    m_projectPath = path;
    onSaveProject();
}

void StudioMainWindow::onBuildStart()
{
    // Make sure we're on the Build tab
    m_editorTabs->setCurrentWidget(m_build);
    collectEditors();
    m_build->startBuild(m_manifest,
                        m_projectPath.isEmpty()
                            ? QDir::homePath()
                            : QFileInfo(m_projectPath).absolutePath());
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
    m_manifest    = imp.manifest();
    m_projectPath.clear();
    m_dirty       = true;
    loadEditors();
    updateWindowTitle();
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
    m_manifest    = imp.manifest();
    m_projectPath.clear();
    m_dirty       = true;
    loadEditors();
    updateWindowTitle();
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

void StudioMainWindow::onTabChanged(int /*index*/)
{
    // Sync sidebar selection to active tab when needed
}

void StudioMainWindow::onBuildLog(const QString &line)
{
    m_logView->appendPlainText(line);
    // Auto-scroll
    QTextCursor c = m_logView->textCursor();
    c.movePosition(QTextCursor::End);
    m_logView->setTextCursor(c);
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
        onBuildLog(QString("\n✓ Build SUCCESS: %1").arg(outputPath));
        statusBar()->showMessage("Build finished successfully.", 8000);
    } else {
        onBuildLog("\n✗ Build FAILED — see log above.");
        statusBar()->showMessage("Build failed.", 8000);
    }
}

// ── Protected ─────────────────────────────────────────────────────────────────
void StudioMainWindow::closeEvent(QCloseEvent *event)
{
    if (confirmDiscard())
        event->accept();
    else
        event->ignore();
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
    m_splitter->setStretchFactor(0, 0);   // sidebar: fixed
    m_splitter->setStretchFactor(1, 1);   // editor: stretch
    m_splitter->setSizes({240, 1040});

    setCentralWidget(m_splitter);

    // ── Editor tabs ──────────────────────────────────────────────────────
    m_appInfo    = new AppInfoEditor(m_editorTabs);
    m_files      = new FilesEditor(m_editorTabs);
    m_components = new ComponentsEditor(m_editorTabs);
    m_shortcuts  = new ShortcutsEditor(m_editorTabs);
    m_registry   = new RegistryEditor(m_editorTabs);
    m_security   = new SecurityEditor(m_editorTabs);
    m_build      = new BuildPanel(m_editorTabs);

    m_editorTabs->addTab(m_appInfo,    svgIcon(SvgIcons::kPackage), "App Info");
    m_editorTabs->addTab(m_files,      svgIcon(SvgIcons::kOpen),    "Files");
    m_editorTabs->addTab(m_components, svgIcon(SvgIcons::kPackage), "Components");
    m_editorTabs->addTab(m_shortcuts,  svgIcon(SvgIcons::kNew),     "Shortcuts");
    m_editorTabs->addTab(m_registry,   svgIcon(SvgIcons::kSettings),"Registry");
    m_editorTabs->addTab(m_security,   svgIcon(SvgIcons::kSettings),"Security");
    m_editorTabs->addTab(m_build,      svgIcon(SvgIcons::kBuild),   "Build");

    connect(m_editorTabs, &QTabWidget::currentChanged, this, &StudioMainWindow::onTabChanged);

    // ── Connect build signals ────────────────────────────────────────────
    connect(m_build, &BuildPanel::buildLog,      this, &StudioMainWindow::onBuildLog);
    connect(m_build, &BuildPanel::buildProgress, this, &StudioMainWindow::onBuildProgress);
    connect(m_build, &BuildPanel::buildFinished, this, &StudioMainWindow::onBuildFinished);

    // ── Build log dock ───────────────────────────────────────────────────
    m_logDock = new QDockWidget("Build Output", this);
    m_logDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    m_logDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    m_logView = new QPlainTextEdit(m_logDock);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(10000);
    m_logView->setFont(QFont("Menlo, Consolas, monospace", 11));
    m_logDock->setWidget(m_logView);

    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);
    m_logDock->hide();

    // ── Status bar ───────────────────────────────────────────────────────
    m_statusLabel = new QLabel("Ready");
    statusBar()->addPermanentWidget(m_statusLabel);
}

void StudioMainWindow::buildMenuBar()
{
    // Qt6.4+ prefers: new QAction then addAction(action) or connect separately.
    // ── File ─────────────────────────────────────────────────────────────
    QMenu *file = menuBar()->addMenu("&File");

    m_actNew = new QAction(svgIcon(SvgIcons::kNew), "&New Project", this);
    m_actNew->setShortcut(QKeySequence::New);
    connect(m_actNew, &QAction::triggered, this, &StudioMainWindow::onNewProject);
    file->addAction(m_actNew);

    m_actOpen = new QAction(svgIcon(SvgIcons::kOpen), "&Open Project...", this);
    m_actOpen->setShortcut(QKeySequence::Open);
    connect(m_actOpen, &QAction::triggered, this, &StudioMainWindow::onOpenProject);
    file->addAction(m_actOpen);

    file->addSeparator();

    m_actSave = new QAction(svgIcon(SvgIcons::kSave), "&Save", this);
    m_actSave->setShortcut(QKeySequence::Save);
    connect(m_actSave, &QAction::triggered, this, &StudioMainWindow::onSaveProject);
    file->addAction(m_actSave);

    m_actSaveAs = new QAction(svgIcon(SvgIcons::kSave), "Save &As...", this);
    m_actSaveAs->setShortcut(QKeySequence::SaveAs);
    connect(m_actSaveAs, &QAction::triggered, this, &StudioMainWindow::onSaveProjectAs);
    file->addAction(m_actSaveAs);

    file->addSeparator();

    QMenu *importMenu = file->addMenu(svgIcon(SvgIcons::kImport), "&Import");
    m_actImportNsis = new QAction("&NSIS Script (.nsi)...", this);
    connect(m_actImportNsis, &QAction::triggered, this, &StudioMainWindow::onImportNsis);
    importMenu->addAction(m_actImportNsis);

    m_actImportInno = new QAction("&Inno Setup Script (.iss)...", this);
    connect(m_actImportInno, &QAction::triggered, this, &StudioMainWindow::onImportInno);
    importMenu->addAction(m_actImportInno);

    file->addSeparator();

    QAction *quitAct = new QAction("&Quit", this);
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, qApp, &QCoreApplication::quit);
    file->addAction(quitAct);

    // ── Build ─────────────────────────────────────────────────────────────
    QMenu *build = menuBar()->addMenu("&Build");
    m_actBuild = new QAction(svgIcon(SvgIcons::kBuild), "&Build Installer...", this);
    m_actBuild->setShortcut(QKeySequence("Ctrl+B"));
    connect(m_actBuild, &QAction::triggered, this, &StudioMainWindow::onBuildStart);
    build->addAction(m_actBuild);

    // ── Help ──────────────────────────────────────────────────────────────
    QMenu *help = menuBar()->addMenu("&Help");
    QAction *aboutAct = new QAction("&About Mcaster1 Install Studio", this);
    connect(aboutAct, &QAction::triggered, this, &StudioMainWindow::onAbout);
    help->addAction(aboutAct);
}

void StudioMainWindow::buildToolBar()
{
    QToolBar *tb = addToolBar("Main");
    tb->setIconSize(QSize(20, 20));
    tb->setMovable(false);

    // Logo label
    QLabel *logo = new QLabel(tb);
    logo->setPixmap(svgIcon(SvgIcons::kLogo, 32).pixmap(32, 32));
    logo->setContentsMargins(6, 0, 10, 0);
    tb->addWidget(logo);

    tb->addAction(m_actNew);
    tb->addAction(m_actOpen);
    tb->addAction(m_actSave);
    tb->addSeparator();
    tb->addAction(m_actBuild);
    m_actBuild->setObjectName("buildBtn");
}

void StudioMainWindow::updateWindowTitle()
{
    QString name = m_manifest.app.name.isEmpty()
                   ? "Untitled Project"
                   : m_manifest.app.name;

    if (m_dirty) name.prepend("* ");
    if (!m_projectPath.isEmpty())
        name += QString(" — %1").arg(QFileInfo(m_projectPath).fileName());

    setWindowTitle(name + " — Mcaster1 Install Studio");
    m_statusLabel->setText(m_projectPath.isEmpty() ? "New Project" : m_projectPath);
}

void StudioMainWindow::loadEditors()
{
    m_appInfo   ->load(m_manifest);
    m_files     ->load(m_manifest);
    m_components->load(m_manifest);
    m_shortcuts ->load(m_manifest);
    m_registry  ->load(m_manifest);
    m_security  ->load(m_manifest);
    m_sidebar   ->populate(m_manifest);
}

void StudioMainWindow::collectEditors()
{
    m_appInfo   ->save(m_manifest);
    m_files     ->save(m_manifest);
    m_components->save(m_manifest);
    m_shortcuts ->save(m_manifest);
    m_registry  ->save(m_manifest);
    m_security  ->save(m_manifest);
}

bool StudioMainWindow::confirmDiscard()
{
    if (!m_dirty) return true;
    auto ans = QMessageBox::question(
        this, "Unsaved Changes",
        "You have unsaved changes. Discard them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ans == QMessageBox::Save) {
        onSaveProject();
        return !m_dirty;  // if save failed, stay open
    }
    return ans == QMessageBox::Discard;
}

QIcon StudioMainWindow::svgIcon(const char *svgStr, int size)
{
    QByteArray data(svgStr);
    QSvgRenderer renderer(data);
    QPixmap px(size, size);
    px.fill(Qt::transparent);
    QPainter p(&px);
    renderer.render(&p);
    return QIcon(px);
}
