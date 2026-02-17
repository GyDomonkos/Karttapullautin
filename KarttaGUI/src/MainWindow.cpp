#include "MainWindow.h"

#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow() {
    auto* central = new QWidget;
    auto* layout = new QVBoxLayout;

    runButton = new QPushButton("Run Karttapullautin");
    logBox = new QTextEdit;
    logBox->setReadOnly(true);

    layout->addWidget(runButton);
    layout->addWidget(logBox);

    central->setLayout(layout);
    setCentralWidget(central);

    connect(runButton, &QPushButton::clicked,
            this, &MainWindow::onRunClicked);

    setWindowTitle("KarttaGUI");
    resize(500, 400);
}

void MainWindow::onRunClicked() {
    logBox->append("Run button clicked.");
}
