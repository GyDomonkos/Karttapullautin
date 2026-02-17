#pragma once

#include <QMainWindow>

class QPushButton;
class QTextEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();

private:
    QPushButton* runButton;
    QTextEdit* logBox;

private slots:
    void onRunClicked();
};
