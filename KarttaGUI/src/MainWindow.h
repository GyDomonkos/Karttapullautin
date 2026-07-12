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
class QDragEnterEvent;
class QDropEvent;
class ToolsPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event)            override;

private slots:
    void browseInput();
    void browseOutput();
    void openOutputFolder();
    void showAbout();
    void startKarttapullautin();
    void cancelKarttapullautin();
    void handleOutput(const QString& text);
    void handleFinished(int exitCode);
    void runToolCommand(const QStringList& args);

private:
    bool writeIniValues(const QString& iniPath,
                        const QMap<QString, QString>& values,
                        const QSet<QString>& toComment = {});

    void applyInputFolder(const QString& folderPath);   // shared by browse + drop

    void moveMergedFilesToOutput();

    KarttaRunner*  runner;
    PreviewPanel*  previewPanel;
    SettingsPanel* settingsPanel;
    ToolsPanel* toolsPanel;

    QLineEdit*    inputEdit;
    QLineEdit*    outputEdit;
    QTextEdit*    outputText;
    QPushButton*  runButton;
    QPushButton*  cancelButton;
    QPushButton*  openFolderButton;
    QProgressBar* progressBar;
    QTabWidget* tabWidget; 

    int  totalTiles;
    int  completedTiles;
    bool wasCancelled;
    bool isToolRun; // Tracks if we are running a tool or the main batch
};