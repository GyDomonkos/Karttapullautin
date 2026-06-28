#pragma once

#include <QMainWindow>
#include <QMap>
#include <QSet>

class KarttaRunner;
class PreviewPanel;
class SettingsPanel;
class QTextEdit;
class QPushButton;
class QLineEdit;
class QProgressBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void browseInput();
    void browseOutput();
    void startKarttapullautin();
    void cancelKarttapullautin();
    void handleOutput(const QString& text);
    void handleFinished(int exitCode);

private:
    // Rewrites matching keys in iniPath, preserving all other content.
    // Keys in toComment are written as "# key=value" (commented out).
    // Commented lines whose key appears in values are uncommented.
    bool writeIniValues(const QString& iniPath,
                        const QMap<QString, QString>& values,
                        const QSet<QString>& toComment = {});

    KarttaRunner*  runner;
    PreviewPanel*  previewPanel;
    SettingsPanel* settingsPanel;

    QLineEdit*    inputEdit;
    QLineEdit*    outputEdit;
    QTextEdit*    outputText;
    QPushButton*  runButton;
    QPushButton*  cancelButton;
    QProgressBar* progressBar;

    int  totalTiles;
    int  completedTiles;
    bool wasCancelled;
};