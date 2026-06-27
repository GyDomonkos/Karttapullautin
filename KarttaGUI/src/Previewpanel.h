#pragma once

#include <QWidget>
#include <QSet>
#include <QString>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QScrollArea;
class QFileSystemWatcher;

// --------------------------------------------------------------------------
// PreviewPanel
//
// Shows a scrollable column of PNG thumbnails on the left and a full-size
// (fit-to-width) view of the selected image on the right.
//
// Call setOutputFolder() once per run to point it at the output directory.
// It scans for existing PNGs immediately, then watches the folder for new
// ones as Karttapullautin produces them.
// --------------------------------------------------------------------------
class PreviewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewPanel(QWidget* parent = nullptr);

    // Clears all thumbnails, scans folderPath for existing PNGs, and starts
    // watching for new ones. Safe to call multiple times.
    void setOutputFolder(const QString& folderPath);

    // Clears all thumbnails and stops watching.
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
    QLabel*             previewLabel;
    QScrollArea*        previewScroll;
    QFileSystemWatcher* watcher;

    QSet<QString> loadedFiles;    // absolute paths already added as thumbnails
    QString       outputFolder;
    QString       currentImagePath;
};