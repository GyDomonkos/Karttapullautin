#include "MainWindow.h"
#include "KarttaRunner.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>
#include <qcoreapplication.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);

    runButton = new QPushButton("Run Karttapullautin");
    outputText = new QTextEdit();
    outputText->setReadOnly(true);

    layout->addWidget(runButton);
    layout->addWidget(outputText);

    setCentralWidget(central);

    runner = new KarttaRunner(this);

    connect(runButton, &QPushButton::clicked,
            this, &MainWindow::startKarttapullautin);

    
    connect(runner, &KarttaRunner::outputReceived,
            this, &MainWindow::handleOutput);

    connect(runner, &KarttaRunner::finished,
            this, &MainWindow::handleFinished);

}

void MainWindow::startKarttapullautin()
{
    runButton->setEnabled(false);

    QString exePath =
        QCoreApplication::applicationDirPath()
        + "/karttapullautin/pullauta.exe";

    QStringList args;
    args << "input.las" << "output_folder";

    runner->run(exePath, args);
}

void MainWindow::handleOutput(const QString& text)
{
    outputText->append(text);
}

void MainWindow::handleFinished(int exitCode)
{
    outputText->append("Finished with exit code: "
                       + QString::number(exitCode));

    runButton->setEnabled(true);
}
