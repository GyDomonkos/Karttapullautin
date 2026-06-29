#pragma once

#include <QWidget>
#include <QSet>
#include <QString>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QScrollArea;
class QFileSystemWatcher;
class QStackedWidget;

class PreviewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewPanel(QWidget* parent = nullptr);

    void setOutputFolder(const QString& folderPath);
    void clear();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onDirectoryChanged(const QString& path);
    void onItemClicked(QListWidgetItem* item);

private:
    void scanFolder();
    void addImageFile(const QString& filePath);
    void showImage(const QString& filePath);
    void showPlaceholder();

    QListWidget*        thumbnailList;
    QLabel*             imageTitle;

    // Right-side content is swapped via a QStackedWidget:
    //   page 0 = placeholderLabel (no scroll area, no artifacts)
    //   page 1 = previewScroll + previewLabel
    QStackedWidget*     stack;
    QLabel*             placeholderLabel;
    QScrollArea*        previewScroll;
    QLabel*             previewLabel;

    QFileSystemWatcher* watcher;

    QSet<QString> loadedFiles;
    QString       outputFolder;
    QString       currentImagePath;
};