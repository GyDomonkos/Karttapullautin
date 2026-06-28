#include "MainWindow.h"
#include "KarttaRunner.h"
#include "PreviewPanel.h"
#include "SettingsPanel.h"

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
#include <QTextStream>
#include <QSet>


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , totalTiles(0)
    , completedTiles(0)
    , wasCancelled(false)
{
    // =========================================================================
    // Left panel — tab widget containing Run + Settings
    // =========================================================================
    QTabWidget* tabs = new QTabWidget();

    // ---- Tab 0: Run --------------------------------------------------------
    QWidget*     runTab    = new QWidget();
    QVBoxLayout* runLayout = new QVBoxLayout(runTab);

    // Input selector
    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputEdit = new QLineEdit();
    QPushButton* browseInputBtn = new QPushButton("Browse LAS");
    inputLayout->addWidget(new QLabel("Input LAS Folder:"));
    inputLayout->addWidget(inputEdit);
    inputLayout->addWidget(browseInputBtn);

    // Output selector
    QHBoxLayout* outputLayout = new QHBoxLayout();
    outputEdit = new QLineEdit();
    QPushButton* browseOutputBtn = new QPushButton("Browse Output");
    outputLayout->addWidget(new QLabel("Output Folder:"));
    outputLayout->addWidget(outputEdit);
    outputLayout->addWidget(browseOutputBtn);

    // Run / Cancel buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    runButton    = new QPushButton("Run Karttapullautin");
    cancelButton = new QPushButton("Cancel");
    cancelButton->setEnabled(false);
    buttonLayout->addWidget(runButton);
    buttonLayout->addWidget(cancelButton);

    // Progress bar — hidden until a run starts
    progressBar = new QProgressBar();
    progressBar->setFormat("%v / %m tiles");
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(false);

    // Console output
    outputText = new QTextEdit();
    outputText->setReadOnly(true);

    runLayout->addLayout(inputLayout);
    runLayout->addLayout(outputLayout);
    runLayout->addLayout(buttonLayout);
    runLayout->addWidget(progressBar);
    runLayout->addWidget(outputText);

    tabs->addTab(runTab, "Run");

    // ---- Tab 1: Settings ---------------------------------------------------
    settingsPanel = new SettingsPanel();
    tabs->addTab(settingsPanel, "Settings");

    // Load current INI values into the settings panel at startup
    const QString iniPath =
        QCoreApplication::applicationDirPath() + "/karttapullautin/pullauta.ini";
    settingsPanel->loadFromIni(iniPath);

    // =========================================================================
    // Right panel — map preview
    // =========================================================================
    previewPanel = new PreviewPanel();

    // =========================================================================
    // Splitter: left (tabs) | right (preview)
    // =========================================================================
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(tabs);
    splitter->addWidget(previewPanel);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    setCentralWidget(splitter);
    resize(1280, 750);

    // =========================================================================
    // Connections
    // =========================================================================
    runner = new KarttaRunner(this);

    connect(browseInputBtn,  &QPushButton::clicked,
            this, &MainWindow::browseInput);
    connect(browseOutputBtn, &QPushButton::clicked,
            this, &MainWindow::browseOutput);
    connect(runButton,       &QPushButton::clicked,
            this, &MainWindow::startKarttapullautin);
    connect(cancelButton,    &QPushButton::clicked,
            this, &MainWindow::cancelKarttapullautin);
    connect(runner, &KarttaRunner::outputReceived,
            this, &MainWindow::handleOutput);
    connect(runner, &KarttaRunner::finished,
            this, &MainWindow::handleFinished);
}

// -----------------------------------------------------------------------------
// Slot: browseInput
// -----------------------------------------------------------------------------
void MainWindow::browseInput()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "Select Folder Containing LAS/LAZ Files");

    if (dir.isEmpty()) return;

    if (!QDir(dir).exists())
    {
        outputText->append("Selected input folder does not exist.");
        return;
    }

    QStringList lazFilters = { "*.las", "*.laz", "*.LAS", "*.LAZ" };
    if (QDir(dir).entryList(lazFilters, QDir::Files).isEmpty())
        outputText->append("Warning: no LAS/LAZ files found in selected folder.");

    inputEdit->setText(dir);
}

// -----------------------------------------------------------------------------
// Slot: browseOutput
// -----------------------------------------------------------------------------
void MainWindow::browseOutput()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory");

    if (!dir.isEmpty())
        outputEdit->setText(dir);
}

// -----------------------------------------------------------------------------
// Slot: startKarttapullautin
// -----------------------------------------------------------------------------
void MainWindow::startKarttapullautin()
{
    QString inputFolder = inputEdit->text().trimmed();
    QString outputPath  = outputEdit->text().trimmed();

    if (inputFolder.isEmpty() || outputPath.isEmpty())
    {
        outputText->append("Please select both an input folder and an output folder.");
        return;
    }
    if (!QDir(inputFolder).exists())
    {
        outputText->append("Input folder does not exist: " + inputFolder);
        return;
    }
    if (!QDir(outputPath).exists())
    {
        outputText->append("Output folder does not exist: " + outputPath);
        return;
    }

    // Count tiles for the progress bar
    QStringList lazFilters = { "*.las", "*.laz", "*.LAS", "*.LAZ" };
    totalTiles     = QDir(inputFolder).entryList(lazFilters, QDir::Files).count();
    completedTiles = 0;
    wasCancelled   = false;

    progressBar->setMinimum(0);
    progressBar->setMaximum(totalTiles > 0 ? totalTiles : 1);
    progressBar->setValue(0);
    progressBar->setVisible(true);

    runButton->setEnabled(false);
    cancelButton->setEnabled(true);
    outputText->clear();

    previewPanel->setOutputFolder(outputPath);

    // Forward slashes — works everywhere, no escaping needed in INI
    inputFolder = QDir::fromNativeSeparators(inputFolder);
    outputPath  = QDir::fromNativeSeparators(outputPath);

    const QString baseDir  = QCoreApplication::applicationDirPath() + "/karttapullautin";
    const QString iniPath  = baseDir + "/pullauta.ini";

    // Settings panel values form the base; run-time values are added on top
    QMap<QString, QString> iniValues = settingsPanel->values();
    iniValues["batch"]          = "1";
    iniValues["lazfolder"]      = inputFolder;
    iniValues["batchoutfolder"] = outputPath;

    if (!writeIniValues(iniPath, iniValues))
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

// -----------------------------------------------------------------------------
// Slot: cancelKarttapullautin
// -----------------------------------------------------------------------------
void MainWindow::cancelKarttapullautin()
{
    wasCancelled = true;
    cancelButton->setEnabled(false);
    outputText->append("\nCancelling...");
    runner->cancel();
}

// -----------------------------------------------------------------------------
// Slot: handleOutput
// -----------------------------------------------------------------------------
void MainWindow::handleOutput(const QString& text)
{
    outputText->append(text.trimmed());

    int done = text.count("All done!")
             + text.count("exists already in output folder");
    if (done > 0)
    {
        completedTiles = qMin(completedTiles + done, totalTiles);
        progressBar->setValue(completedTiles);
    }
}

// -----------------------------------------------------------------------------
// Slot: handleFinished
// -----------------------------------------------------------------------------
void MainWindow::handleFinished(int exitCode)
{
    if (wasCancelled)
        outputText->append("\nRun cancelled.");
    else if (exitCode == 0)
        outputText->append("\nAll tiles processed successfully.");
    else
        outputText->append("\nProcess finished with exit code: "
                           + QString::number(exitCode));

    wasCancelled = false;
    runButton->setEnabled(true);
    cancelButton->setEnabled(false);
    progressBar->setVisible(false);
}

// -----------------------------------------------------------------------------
// Private helper: writeIniValues
// -----------------------------------------------------------------------------
bool MainWindow::writeIniValues(const QString& iniPath,
                                const QMap<QString, QString>& values)
{
    QFile file(iniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QStringList   lines;
    QSet<QString> updated;
    QTextStream   in(&file);

    while (!in.atEnd())
    {
        QString line    = in.readLine();
        QString trimmed = line.trimmed();
        bool    replaced = false;

        if (!trimmed.isEmpty() && !trimmed.startsWith('#') && !trimmed.startsWith('%'))
        {
            for (auto it = values.constBegin(); it != values.constEnd(); ++it)
            {
                if (trimmed.startsWith(it.key() + "="))
                {
                    lines << it.key() + "=" + it.value();
                    updated.insert(it.key());
                    replaced = true;
                    break;
                }
            }
        }

        if (!replaced)
            lines << line;
    }
    file.close();

    for (auto it = values.constBegin(); it != values.constEnd(); ++it)
        if (!updated.contains(it.key()))
            lines << it.key() + "=" + it.value();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;

    QTextStream out(&file);
    for (const QString& line : lines)
        out << line << "\n";

    return true;
}
