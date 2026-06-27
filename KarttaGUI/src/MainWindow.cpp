#include "MainWindow.h"
#include "KarttaRunner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
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

    // Run button
    runButton = new QPushButton("Run Karttapullautin");

    // Output console
    outputText = new QTextEdit();
    outputText->setReadOnly(true);

    mainLayout->addLayout(inputLayout);
    mainLayout->addLayout(outputLayout);
    mainLayout->addWidget(runButton);
    mainLayout->addWidget(outputText);

    setCentralWidget(central);

    runner = new KarttaRunner(this);

    connect(browseInputBtn, &QPushButton::clicked,
            this, &MainWindow::browseInput);

    connect(browseOutputBtn, &QPushButton::clicked,
            this, &MainWindow::browseOutput);

    connect(runButton, &QPushButton::clicked,
            this, &MainWindow::startKarttapullautin);

    connect(runner, &KarttaRunner::outputReceived,
            this, &MainWindow::handleOutput);

    connect(runner, &KarttaRunner::finished,
            this, &MainWindow::handleFinished);
}

// ---------------------------------------------------------------------
// Slot: browseInput
// Bug fix 1: guard against cancelled dialog (empty string)
// Bug fix 2: actually store the result in inputEdit
// Bug fix 3: warn if the folder contains no LAS/LAZ files
// ---------------------------------------------------------------------
void MainWindow::browseInput()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Folder Containing LAS/LAZ Files"
    );

    // User cancelled — do nothing
    if (dir.isEmpty())
        return;

    QDir inputDir(dir);
    if (!inputDir.exists())
    {
        outputText->append("Selected input folder does not exist.");
        return;
    }

    // Warn if no LAS/LAZ files are present (non-fatal)
    QStringList lazFilters = { "*.las", "*.laz", "*.LAS", "*.LAZ" };
    if (inputDir.entryList(lazFilters, QDir::Files).isEmpty())
        outputText->append("Warning: no LAS/LAZ files found in selected folder.");

    // Bug fix 2: store the path
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
// Bug fix 4: validate a directory, not a file
// Bug fix 5: replace QSettings with writeIniValues
// Bug fix 6: resolve executable name per platform
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

    // Bug fix 4: validate as directories
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

    runButton->setEnabled(false);
    outputText->clear();

    // Use forward slashes throughout — cross-platform and no escaping needed.
    // Karttapullautin (Rust) accepts forward slashes on Windows.
    inputFolder = QDir::fromNativeSeparators(inputFolder);
    outputPath  = QDir::fromNativeSeparators(outputPath);

    // Locate the bundled karttapullautin directory next to this executable
    QString baseDir = QCoreApplication::applicationDirPath() + "/karttapullautin";
    QString iniPath = baseDir + "/pullauta.ini";

    // Bug fix 5: write INI values safely without QSettings
    QMap<QString, QString> iniValues;
    iniValues["batch"]         = "1";
    iniValues["lazfolder"]     = inputFolder;
    iniValues["batchoutfolder"] = outputPath;

    if (!writeIniValues(iniPath, iniValues))
    {
        outputText->append("Failed to update pullauta.ini at: " + iniPath);
        runButton->setEnabled(true);
        return;
    }

    outputText->append("pullauta.ini updated.");
    outputText->append("Input folder : " + inputFolder);
    outputText->append("Output folder: " + outputPath);
    outputText->append("Launching Karttapullautin...\n");

    // Bug fix 6: platform-aware executable name
#ifdef Q_OS_WIN
    const QString executable = baseDir + "/pullauta.exe";
#else
    const QString executable = baseDir + "/pullauta";
#endif

    runner->run(executable, QStringList());
}

// ---------------------------------------------------------------------
// Private helper: writeIniValues
//
// Reads the INI file line by line, replaces lines whose key matches
// one of the entries in `values`, and writes the file back.
//
// This preserves all comments, ordering, and exotic characters that
// QSettings would otherwise percent-encode or restructure.
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
        QString line     = in.readLine();
        QString trimmed  = line.trimmed();
        bool    replaced = false;

        // Only touch non-commented, non-empty lines
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

    // Append any keys that weren't already present in the file
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

// ---------------------------------------------------------------------
// Slots: process output / finish
// ---------------------------------------------------------------------
void MainWindow::handleOutput(const QString& text)
{
    outputText->append(text.trimmed());
}

void MainWindow::handleFinished(int exitCode)
{
    outputText->append(
        "\nProcess finished with exit code: "
        + QString::number(exitCode)
    );

    runButton->setEnabled(true);
}