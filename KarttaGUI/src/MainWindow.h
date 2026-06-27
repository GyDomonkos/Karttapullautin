#pragma once

#include <QMainWindow>
#include <QMap>

class KarttaRunner;
class QTextEdit;
class QPushButton;
class QLineEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void browseInput();
    void browseOutput();
    void startKarttapullautin();
    void handleOutput(const QString& text);
    void handleFinished(int exitCode);

private:
    // Reads iniPath line-by-line, replaces matching keys, writes back.
    // Avoids QSettings which percent-encodes keys and restructures sections.
    bool writeIniValues(const QString& iniPath,
                        const QMap<QString, QString>& values);

    KarttaRunner* runner;

    QLineEdit* inputEdit;
    QLineEdit* outputEdit;
    QTextEdit* outputText;
    QPushButton* runButton;
};