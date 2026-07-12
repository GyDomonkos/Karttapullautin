#include "MainWindow.h"
#include "KarttaRunner.h"
#include "PreviewPanel.h"
#include "SettingsPanel.h"
#include "ToolsPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QWidget>
#include <QSplitter>
#include <QFileDialog>
#include <QLineEdit>
#include <QLabel>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QSet>
#include <QDesktopServices>
#include <QUrl>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMenuBar>
#include <QMessageBox>


// --------------------------------------------------------------------------
// File-level helper: extracts the key name from a commented line.
// --------------------------------------------------------------------------
static QString commentedKey(const QString& trimmed)
{
    QString rest;
    if (trimmed.startsWith("%23"))
    {
        rest = trimmed.mid(3);
        if (rest.startsWith("%20")) rest = rest.mid(3);
        rest = rest.trimmed();
    }
    else if (trimmed.startsWith("#"))
    {
        rest = trimmed.mid(1).trimmed();
    }
    else return {};

    const int eq = rest.indexOf('=');
    if (eq <= 0) return {};
    return rest.left(eq).trimmed();
}


// ==========================================================================
// Constructor
// ==========================================================================
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , totalTiles(0)
    , completedTiles(0)
    , wasCancelled(false)
    , isToolRun(false)
{
    setWindowIcon(QIcon("resources/icon.png"));
    setAcceptDrops(true);   // enable folder / LAS drag-and-drop onto the window

    // FIX: Assigned directly to the class member variable instead of a local variable
    tabWidget = new QTabWidget(this);

    // ---- Tab 0: Run ------------------------------------------------------
    QWidget* runTab    = new QWidget();
    QVBoxLayout* runLayout = new QVBoxLayout(runTab);

    // Input row
    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputEdit = new QLineEdit();
    inputEdit->setPlaceholderText("Drop a folder or LAS/LAZ file here, or use Browse…");
    QPushButton* browseInputBtn = new QPushButton("Browse LAS");
    inputLayout->addWidget(new QLabel("Input LAS Folder:"));
    inputLayout->addWidget(inputEdit);
    inputLayout->addWidget(browseInputBtn);

    // Output row — Browse + Open Folder button
    QHBoxLayout* outputLayout = new QHBoxLayout();
    outputEdit = new QLineEdit();
    QPushButton* browseOutputBtn = new QPushButton("Browse Output");
    openFolderButton = new QPushButton("Open Folder");
    openFolderButton->setToolTip("Open the output folder in Explorer");
    openFolderButton->setEnabled(false);
    outputLayout->addWidget(new QLabel("Output Folder:"));
    outputLayout->addWidget(outputEdit);
    outputLayout->addWidget(browseOutputBtn);
    outputLayout->addWidget(openFolderButton);

    // Run / Cancel
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    runButton    = new QPushButton("Run Karttapullautin");
    cancelButton = new QPushButton("Cancel");
    cancelButton->setEnabled(false);
    buttonLayout->addWidget(runButton);
    buttonLayout->addWidget(cancelButton);

    progressBar = new QProgressBar();
    progressBar->setFormat("%v / %m tiles");
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(false);

    outputText = new QTextEdit();
    outputText->setReadOnly(true);

    runLayout->addLayout(inputLayout);
    runLayout->addLayout(outputLayout);
    runLayout->addLayout(buttonLayout);
    runLayout->addWidget(progressBar);
    runLayout->addWidget(outputText);

    tabWidget->addTab(runTab, "Run");

    // ---- Tab 1: Settings ------------------------------------------------
    settingsPanel = new SettingsPanel();
    tabWidget->addTab(settingsPanel, "Settings");

    const QString iniPath =
        QCoreApplication::applicationDirPath() + "/karttapullautin/pullauta.ini";
    settingsPanel->loadFromIni(iniPath);

    // ---- Tab 2: Tools ---------------------------------------------------
    toolsPanel = new ToolsPanel(this);
    tabWidget->addTab(toolsPanel, "Tools");
    tabWidget->setTabEnabled(2, false); // Keep disabled until an initial successful run occurs

    // Connect the panel's action signal directly into our executor
    connect(toolsPanel, &ToolsPanel::toolActionRequested, this, &MainWindow::runToolCommand);

    // ---- Preview panel --------------------------------------------------
    previewPanel = new PreviewPanel();

    // ---- Splitter -------------------------------------------------------
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(tabWidget);
    splitter->addWidget(previewPanel);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    setCentralWidget(splitter);
    resize(1280, 750);

    // ---- Connections ----------------------------------------------------
    runner = new KarttaRunner(this);

    connect(browseInputBtn,  &QPushButton::clicked, this, &MainWindow::browseInput);
    connect(browseOutputBtn, &QPushButton::clicked, this, &MainWindow::browseOutput);
    connect(openFolderButton,&QPushButton::clicked, this, &MainWindow::openOutputFolder);
    connect(runButton,       &QPushButton::clicked, this, &MainWindow::startKarttapullautin);
    connect(cancelButton,    &QPushButton::clicked, this, &MainWindow::cancelKarttapullautin);
    connect(runner, &KarttaRunner::outputReceived,  this, &MainWindow::handleOutput);
    connect(runner, &KarttaRunner::finished,        this, &MainWindow::handleFinished);

    // Enable the Open Folder button whenever the output path field is non-empty
    connect(outputEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        openFolderButton->setEnabled(!text.trimmed().isEmpty());
    });
}

// ==========================================================================
// Drag & drop
// ==========================================================================

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event->mimeData()->hasUrls()) { event->ignore(); return; }

    for (const QUrl& url : event->mimeData()->urls())
    {
        if (!url.isLocalFile()) continue;
        const QFileInfo info(url.toLocalFile());
        if (info.isDir()) { event->acceptProposedAction(); return; }
        const QString ext = info.suffix().toLower();
        if (ext == "las" || ext == "laz") { event->acceptProposedAction(); return; }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    for (const QUrl& url : event->mimeData()->urls())
    {
        if (!url.isLocalFile()) continue;
        const QFileInfo info(url.toLocalFile());

        QString folder;
        if (info.isDir())
            folder = info.absoluteFilePath();
        else if (info.suffix().toLower() == "las" || info.suffix().toLower() == "laz")
            folder = info.absolutePath();

        if (!folder.isEmpty())
        {
            applyInputFolder(folder);
            event->acceptProposedAction();
            return;
        }
    }
}

// ==========================================================================
// Slots
// ==========================================================================

void MainWindow::browseInput()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, "Select Folder Containing LAS/LAZ Files");
    if (!dir.isEmpty())
        applyInputFolder(dir);
}

void MainWindow::applyInputFolder(const QString& folderPath)
{
    if (!QDir(folderPath).exists())
    {
        outputText->append("Folder does not exist: " + folderPath);
        return;
    }
    const QStringList lazFilters = { "*.las", "*.laz", "*.LAS", "*.LAZ" };
    if (QDir(folderPath).entryList(lazFilters, QDir::Files).isEmpty())
        outputText->append("Warning: no LAS/LAZ files found in: " + folderPath);

    inputEdit->setText(folderPath);
}

void MainWindow::browseOutput()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory");
    if (!dir.isEmpty())
        outputEdit->setText(dir);
}

void MainWindow::openOutputFolder()
{
    const QString path = outputEdit->text().trimmed();
    if (path.isEmpty()) return;

    if (!QDir(path).exists())
    {
        outputText->append("Output folder does not exist: " + path);
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::startKarttapullautin()
{
    QString inputFolder = inputEdit->text().trimmed();
    QString outputPath  = outputEdit->text().trimmed();

    if (inputFolder.isEmpty() || outputPath.isEmpty()) {
        outputText->append("Please select both an input folder and an output folder.");
        return;
    }
    if (!QDir(inputFolder).exists()) {
        outputText->append("Input folder does not exist: " + inputFolder);
        return;
    }
    if (!QDir(outputPath).exists()) {
        outputText->append("Output folder does not exist: " + outputPath);
        return;
    }

    const QStringList lazFilters = { "*.las", "*.laz", "*.LAS", "*.LAZ" };
    totalTiles     = QDir(inputFolder).entryList(lazFilters, QDir::Files).count();
    completedTiles = 0;
    wasCancelled   = false;
    isToolRun      = false;

    progressBar->setMinimum(0);
    progressBar->setMaximum(totalTiles > 0 ? totalTiles : 1);
    progressBar->setValue(0);
    progressBar->setVisible(true);
    runButton->setEnabled(false);
    cancelButton->setEnabled(true);
    outputText->clear();

    previewPanel->setOutputFolder(outputPath);

    inputFolder = QDir::fromNativeSeparators(inputFolder);
    outputPath  = QDir::fromNativeSeparators(outputPath);

    const QString baseDir = QCoreApplication::applicationDirPath() + "/karttapullautin";
    const QString iniPath = baseDir + "/pullauta.ini";

    QMap<QString, QString> iniValues = settingsPanel->values();
    iniValues["batch"]          = "1";
    iniValues["lazfolder"]      = inputFolder;
    iniValues["batchoutfolder"] = outputPath;

    if (!writeIniValues(iniPath, iniValues, settingsPanel->disabledOptionalKeys()))
    {
        outputText->append("Failed to update pullauta.ini at: " + iniPath);
        runButton->setEnabled(true);
        cancelButton->setEnabled(false);
        progressBar->setVisible(false);
        return;
    }

    outputText->append("pullauta.ini updated.");
    outputText->append("Input folder : " + inputFolder);
    outputText->append("Output folder: " + outputPath);
    outputText->append(QString("Tiles found  : %1").arg(totalTiles));
    outputText->append("Launching Karttapullautin...\n");

#ifdef Q_OS_WIN
    const QString executable = baseDir + "/pullauta.exe";
#else
    const QString executable = baseDir + "/pullauta";
#endif

    runner->run(executable, QStringList());
}

void MainWindow::cancelKarttapullautin()
{
    wasCancelled = true;
    cancelButton->setEnabled(false);
    outputText->append("\nCancelling...");
    runner->cancel();
}

void MainWindow::handleOutput(const QString& text)
{
    outputText->append(text.trimmed());

    const int done = text.count("All done!")
                   + text.count("exists already in output folder");
    if (done > 0) {
        completedTiles = qMin(completedTiles + done, totalTiles);
        progressBar->setValue(completedTiles);
    }
}

void MainWindow::handleFinished(int exitCode)
{
    if (wasCancelled) {
        outputText->append("\nRun cancelled.");
    } else if (exitCode == 0) {
        outputText->append("\nProcess finished successfully.");
    } else {
        outputText->append("\nProcess finished with exit code: " + QString::number(exitCode));
    }

    wasCancelled = false;
    runButton->setEnabled(true);
    cancelButton->setEnabled(false);
    progressBar->setVisible(false);

    if (exitCode == 0) {
        if (!isToolRun) {
            // It was a batch run. Unlock the tools!
            tabWidget->setTabEnabled(2, true); 
            outputText->append("\nPost-processing tools are now available in the 'Tools' tab.");
        } else {
            // It was a tool run. Sweep the directory for merged files!
            moveMergedFilesToOutput();
        }
    }
}

// ==========================================================================
// Private: writeIniValues
// ==========================================================================
bool MainWindow::writeIniValues(const QString& iniPath,
                                const QMap<QString, QString>& values,
                                const QSet<QString>& toComment)
{
    QFile file(iniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QStringList   lines;
    QSet<QString> handled;
    QTextStream   in(&file);

    while (!in.atEnd())
    {
        QString line = in.readLine();
        QString trimmed = line.trimmed();
        bool    replaced = false;

        if (!trimmed.isEmpty() && !trimmed.startsWith('#') &&
            !trimmed.startsWith('%') && !trimmed.startsWith('['))
        {
            for (auto it = values.constBegin(); it != values.constEnd(); ++it)
            {
                if (trimmed.startsWith(it.key() + "="))
                {
                    lines << (toComment.contains(it.key())
                              ? "# " + it.key() + "=" + it.value()
                              :        it.key() + "=" + it.value());
                    handled.insert(it.key());
                    replaced = true;
                    break;
                }
            }
            if (!replaced)
            {
                for (const QString& key : toComment)
                {
                    if (trimmed.startsWith(key + "="))
                    {
                        lines << "# " + trimmed;
                        handled.insert(key);
                        replaced = true;
                        break;
                    }
                }
            }
        }

        if (!replaced)
        {
            const QString cKey = commentedKey(trimmed);
            if (!cKey.isEmpty() && values.contains(cKey) && !toComment.contains(cKey))
            {
                lines << cKey + "=" + values[cKey];
                handled.insert(cKey);
                replaced = true;
            }
        }

        if (!replaced)
            lines << line;
    }
    file.close();

    for (auto it = values.constBegin(); it != values.constEnd(); ++it)
        if (!handled.contains(it.key()) && !toComment.contains(it.key()))
            lines << it.key() + "=" + it.value();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;

    QTextStream out(&file);
    for (const QString& line : lines)
        out << line << "\n";

    return true;
}

void MainWindow::runToolCommand(const QStringList& args)
{
    QString outputPath = outputEdit->text().trimmed();
    if (outputPath.isEmpty() || !QDir(outputPath).exists()) {
        outputText->append("\n[Error] Action Aborted: Please select a valid Output Folder first. Tools operate on files existing inside that directory.");
        return;
    }

    // Switch view to the Run/Log tab so the user can watch the output.
    tabWidget->setCurrentIndex(0);

    runButton->setEnabled(false);
    cancelButton->setEnabled(true);
    isToolRun = true;
    outputText->append(QString("\nExecuting tool command: pullauta %1").arg(args.join(" ")));

    QString iniPath = QCoreApplication::applicationDirPath() + "/karttapullautin/pullauta.ini";
    writeIniValues(iniPath, settingsPanel->values(), settingsPanel->disabledOptionalKeys());

    QString executable = QCoreApplication::applicationDirPath() + "/karttapullautin/pullauta";
#if defined(Q_OS_WIN)
    executable += ".exe";
#endif

    runner->run(executable, args);
}


void MainWindow::moveMergedFilesToOutput()
{
    const QString baseDir = QCoreApplication::applicationDirPath() + "/karttapullautin";
    const QString outDir = outputEdit->text().trimmed();
    QDir pullautaDir(baseDir);
    
    // The specific prefixes Karttapullautin generates when merging
    QStringList filters;
    filters << "merged*.png" << "merged*.jpg" 
            << "merged*.pgw" << "merged*.jgw"
            << "vege*.png"   << "vege*.jpg"
            << "*merge*.dxf";
            
    QFileInfoList generatedFiles = pullautaDir.entryInfoList(filters, QDir::Files);
    
    bool movedAny = false;
    for (const QFileInfo& fileInfo : generatedFiles) {
        QString sourcePath = fileInfo.absoluteFilePath();
        QString destPath = outDir + "/" + fileInfo.fileName();
        
        // Remove existing destination file if it exists to allow overwrite
        if (QFile::exists(destPath)) {
            QFile::remove(destPath);
        }
        
        // Move the file
        if (QFile::rename(sourcePath, destPath)) {
            outputText->append("Moved output file to: " + destPath);
            movedAny = true;
        }
    }
    
    if (movedAny) {
        outputText->append("All merged files have been safely moved to your Output Folder.");
    }
}

// ==========================================================================
// Slot: showAbout
// ==========================================================================
void MainWindow::showAbout()
{
    // APP_VERSION is injected by CMake via target_compile_definitions so it
    // always matches the version declared in CMakeLists.txt — no duplication.
#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif

    QMessageBox::about(this,
        "About KarttaGUI",
        "<h3>KarttaGUI v" APP_VERSION "</h3>"
        "<p>A graphical frontend for "
        "<a href='https://github.com/karttapullautin/karttapullautin'>Karttapullautin</a> "
        "— the LiDAR-to-orienteering-map processing engine.</p>"
        "<p>Built with Qt " QT_VERSION_STR " (MinGW, C++17)</p>"
        "<hr>"
        "<p style='font-size:small;color:gray;'>"
        "Copyright &copy; 2026 Domonkos Gyorffy. All Rights Reserved.<br>"
        "Source-available — not open source. See LICENSE for details."
        "</p>"
    );
}