#pragma once

#include <QMainWindow>

class KarttaRunner;
class QTextEdit;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void startKarttapullautin();
    void handleOutput(const QString& text);
    void handleFinished(int exitCode);

private:
    KarttaRunner* runner;
    QTextEdit* outputText;
    QPushButton* runButton;
};
