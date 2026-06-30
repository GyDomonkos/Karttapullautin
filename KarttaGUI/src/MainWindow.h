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
    void startKarttapullautin();
    void cancelKarttapullautin();
    void handleOutput(const QString& text);
    void handleFinished(int exitCode);

private:
    bool writeIniValues(const QString& iniPath,
                        const QMap<QString, QString>& values,
                        const QSet<QString>& toComment = {});

    void applyInputFolder(const QString& folderPath);   // shared by browse + drop

    KarttaRunner*  runner;
    PreviewPanel*  previewPanel;
    SettingsPanel* settingsPanel;

    QLineEdit*    inputEdit;
    QLineEdit*    outputEdit;
    QTextEdit*    outputText;
    QPushButton*  runButton;
    QPushButton*  cancelButton;
    QPushButton*  openFolderButton;
    QProgressBar* progressBar;

    int  totalTiles;
    int  completedTiles;
    bool wasCancelled;
};