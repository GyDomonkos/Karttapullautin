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


// --------------------------------------------------------------------------
// File-level helper: extracts the key name from a commented line.
//
// Handles both formats found in pullauta.ini:
//   # key=value          (plain comment)
//   %23%20key=value      (percent-encoded comment written by old QSettings)
//
// Returns an empty string if the line is not a commented key=value pair.
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
{
    QTabWidget* tabs = new QTabWidget();

    // ---- Tab 0: Run ------------------------------------------------------
    QWidget*     runTab    = new QWidget();
    QVBoxLayout* runLayout = new QVBoxLayout(runTab);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputEdit = new QLineEdit();
    QPushButton* browseInputBtn = new QPushButton("Browse LAS");
    inputLayout->addWidget(new QLabel("Input LAS Folder:"));
    inputLayout->addWidget(inputEdit);
    inputLayout->addWidget(browseInputBtn);

    QHBoxLayout* outputLayout = new QHBoxLayout();
    outputEdit = new QLineEdit();
    QPushButton* browseOutputBtn = new QPushButton("Browse Output");
    outputLayout->addWidget(new QLabel("Output Folder:"));
    outputLayout->addWidget(outputEdit);
    outputLayout->addWidget(browseOutputBtn);

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

    tabs->addTab(runTab, "Run");

    // ---- Tab 1: Settings ------------------------------------------------
    settingsPanel = new SettingsPanel();
    tabs->addTab(settingsPanel, "Settings");

    const QString iniPath =
        QCoreApplication::applicationDirPath() + "/karttapullautin/pullauta.ini";
    settingsPanel->loadFromIni(iniPath);

    // ---- Preview panel --------------------------------------------------
    previewPanel = new PreviewPanel();

    // ---- Splitter -------------------------------------------------------
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(tabs);
    splitter->addWidget(previewPanel);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    setCentralWidget(splitter);
    resize(1280, 750);

    // ---- Connections ----------------------------------------------------
    runner = new KarttaRunner(this);

    connect(browseInputBtn,  &QPushButton::clicked, this, &MainWindow::browseInput);
    connect(browseOutputBtn, &QPushButton::clicked, this, &MainWindow::browseOutput);
    connect(runButton,       &QPushButton::clicked, this, &MainWindow::startKarttapullautin);
    connect(cancelButton,    &QPushButton::clicked, this, &MainWindow::cancelKarttapullautin);
    connect(runner, &KarttaRunner::outputReceived,  this, &MainWindow::handleOutput);
    connect(runner, &KarttaRunner::finished,        this, &MainWindow::handleFinished);
}

// ==========================================================================
// Slots
// ==========================================================================
void MainWindow::browseInput()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "Select Folder Containing LAS/LAZ Files");
    if (dir.isEmpty()) return;

    if (!QDir(dir).exists()) {
        outputText->append("Selected input folder does not exist.");
        return;
    }
    QStringList lazFilters = { "*.las", "*.laz", "*.LAS", "*.LAZ" };
    if (QDir(dir).entryList(lazFilters, QDir::Files).isEmpty())
        outputText->append("Warning: no LAS/LAZ files found in selected folder.");

    inputEdit->setText(dir);
}

void MainWindow::browseOutput()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory");
    if (!dir.isEmpty()) outputEdit->setText(dir);
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

    inputFolder = QDir::fromNativeSeparators(inputFolder);
    outputPath  = QDir::fromNativeSeparators(outputPath);

    const QString baseDir = QCoreApplication::applicationDirPath() + "/karttapullautin";
    const QString iniPath = baseDir + "/pullauta.ini";

    // Settings panel provides the full parameter set;
    // run-time values (paths, batch) are added on top.
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

    int done = text.count("All done!")
             + text.count("exists already in output folder");
    if (done > 0) {
        completedTiles = qMin(completedTiles + done, totalTiles);
        progressBar->setValue(completedTiles);
    }
}

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

// ==========================================================================
// Private: writeIniValues
//
// Line-by-line INI rewriter. Three cases per line:
//   1. Active key in `values`  → overwrite (or comment out if in toComment)
//   2. Active key in `toComment` only → comment it out
//   3. Commented key whose name is in `values` → uncomment and write new value
//   4. Everything else → preserve verbatim
//
// Keys in `values` that were not found anywhere are appended at the end.
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
        QString line    = in.readLine();
        QString trimmed = line.trimmed();
        bool    replaced = false;

        // ---- Active (non-commented) key=value lines ----------------------
        if (!trimmed.isEmpty() && !trimmed.startsWith('#') &&
            !trimmed.startsWith('%') && !trimmed.startsWith('['))
        {
            // Case 1: key is in values
            for (auto it = values.constBegin(); it != values.constEnd(); ++it)
            {
                if (trimmed.startsWith(it.key() + "="))
                {
                    if (toComment.contains(it.key()))
                        lines << "# " + it.key() + "=" + it.value();
                    else
                        lines << it.key() + "=" + it.value();
                    handled.insert(it.key());
                    replaced = true;
                    break;
                }
            }
            // Case 2: key should be commented out but isn't in values
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

        // ---- Commented lines that should be activated --------------------
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

    // Append active keys that weren't present in the file at all
    for (auto it = values.constBegin(); it != values.constEnd(); ++it)
    {
        if (!handled.contains(it.key()) && !toComment.contains(it.key()))
            lines << it.key() + "=" + it.value();
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;

    QTextStream out(&file);
    for (const QString& line : lines)
        out << line << "\n";

    return true;
}