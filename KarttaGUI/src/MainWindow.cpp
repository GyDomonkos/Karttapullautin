#include "MainWindow.h"
#include "KarttaRunner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QWidget>
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
    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

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

    // Run / Cancel buttons on one row
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

    // Output console
    outputText = new QTextEdit();
    outputText->setReadOnly(true);

    mainLayout->addLayout(inputLayout);
    mainLayout->addLayout(outputLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(outputText);

    setCentralWidget(central);

    runner = new KarttaRunner(this);

    connect(browseInputBtn, &QPushButton::clicked,
            this, &MainWindow::browseInput);

    connect(browseOutputBtn, &QPushButton::clicked,
            this, &MainWindow::browseOutput);

    connect(runButton, &QPushButton::clicked,
            this, &MainWindow::startKarttapullautin);

    connect(cancelButton, &QPushButton::clicked,
            this, &MainWindow::cancelKarttapullautin);

    connect(runner, &KarttaRunner::outputReceived,
            this, &MainWindow::handleOutput);

    connect(runner, &KarttaRunner::finished,
            this, &MainWindow::handleFinished);
}

// ---------------------------------------------------------------------
// Slot: browseInput
// ---------------------------------------------------------------------
void MainWindow::browseInput()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Folder Containing LAS/LAZ Files"
    );

    if (dir.isEmpty())
        return;

    QDir inputDir(dir);
    if (!inputDir.exists())
    {
        outputText->append("Selected input folder does not exist.");
        return;
    }

    QStringList lazFilters = { "*.las", "*.laz", "*.LAS", "*.LAZ" };
    if (inputDir.entryList(lazFilters, QDir::Files).isEmpty())
        outputText->append("Warning: no LAS/LAZ files found in selected folder.");

    inputEdit->setText(dir);
}

// ---------------------------------------------------------------------
// Slot: browseOutput
// ---------------------------------------------------------------------
void MainWindow::browseOutput()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Output Directory"
    );

    if (!dir.isEmpty())
        outputEdit->setText(dir);
}

// ---------------------------------------------------------------------
// Slot: startKarttapullautin
// ---------------------------------------------------------------------
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

    // Count LAZ/LAS tiles to drive the progress bar
    QStringList lazFilters = { "*.las", "*.laz", "*.LAS", "*.LAZ" };
    totalTiles     = QDir(inputFolder).entryList(lazFilters, QDir::Files).count();
    completedTiles = 0;
    wasCancelled   = false;

    // Set up the progress bar
    progressBar->setMinimum(0);
    progressBar->setMaximum(totalTiles > 0 ? totalTiles : 1);
    progressBar->setValue(0);
    progressBar->setVisible(true);

    // Flip button states
    runButton->setEnabled(false);
    cancelButton->setEnabled(true);
    outputText->clear();

    // Use forward slashes — cross-platform and no escaping needed in the INI
    inputFolder = QDir::fromNativeSeparators(inputFolder);
    outputPath  = QDir::fromNativeSeparators(outputPath);

    QString baseDir = QCoreApplication::applicationDirPath() + "/karttapullautin";
    QString iniPath = baseDir + "/pullauta.ini";

    QMap<QString, QString> iniValues;
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

// ---------------------------------------------------------------------
// Slot: cancelKarttapullautin
// ---------------------------------------------------------------------
void MainWindow::cancelKarttapullautin()
{
    wasCancelled = true;
    cancelButton->setEnabled(false);
    outputText->append("\nCancelling...");
    runner->cancel();
}

// ---------------------------------------------------------------------
// Slot: handleOutput
// Count tile-completion events to drive the progress bar.
//
// Two patterns each mean one tile is done:
//   "All done!"                          — tile was processed this run
//   "exists already in output folder"    — tile was skipped (prior run)
// ---------------------------------------------------------------------
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

// ---------------------------------------------------------------------
// Slot: handleFinished
// ---------------------------------------------------------------------
void MainWindow::handleFinished(int exitCode)
{
    if (wasCancelled)
    {
        outputText->append("\nRun cancelled.");
    }
    else if (exitCode == 0)
    {
        outputText->append("\nAll tiles processed successfully.");
    }
    else
    {
        outputText->append(
            "\nProcess finished with exit code: " + QString::number(exitCode));
    }

    // Reset UI
    wasCancelled = false;
    runButton->setEnabled(true);
    cancelButton->setEnabled(false);
    progressBar->setVisible(false);
}

// ---------------------------------------------------------------------
// Private helper: writeIniValues
// ---------------------------------------------------------------------
bool MainWindow::writeIniValues(const QString& iniPath,
                                const QMap<QString, QString>& values)
{
    QFile file(iniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QStringList lines;
    QSet<QString> updated;
    QTextStream in(&file);

    while (!in.atEnd())
    {
        QString line    = in.readLine();
        QString trimmed = line.trimmed();
        bool replaced   = false;

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
    {
        if (!updated.contains(it.key()))
            lines << it.key() + "=" + it.value();
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;

    QTextStream out(&file);
    for (const QString& line : lines)
        out << line << "\n";

    return true;
}