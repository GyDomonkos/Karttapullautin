#pragma once

#include <QWidget>
#include <QSet>
#include <QString>
#include <QGraphicsView>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QFileSystemWatcher;
class QStackedWidget;
class QGraphicsScene;
class QGraphicsPixmapItem;

class PreviewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewPanel(QWidget* parent = nullptr);

    void setOutputFolder(const QString& folderPath);
    void clear();

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onDirectoryChanged(const QString& path);
    void onItemClicked(QListWidgetItem* item);

private:
    void scanFolder();
    void addImageFile(const QString& filePath);
    void showImage(const QString& filePath);
    void showPlaceholder();
    void setupGraphicsView();
    void fitToWindow();
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void updateZoomLabel();

    QListWidget*        thumbnailList;
    QLabel*             imageTitle;

    // Right-side content is swapped via a QStackedWidget:
    //   page 0 = placeholderLabel (no scroll area, no artifacts)
    //   page 1 = graphicsView
    QStackedWidget*     stack;
    QLabel*             placeholderLabel;
    
    // Graphics View components for zoom/pan
    QGraphicsScene*     scene;
    QGraphicsView*      graphicsView;
    QGraphicsPixmapItem* pixmapItem;

    QFileSystemWatcher* watcher;

    QSet<QString> loadedFiles;
    QString       outputFolder;
    QString       currentImagePath;
    
    // Zoom state
    qreal currentScale;
    qreal minScale;
    qreal maxScale;
    
    // Zoom level display
    QLabel*             zoomLabel;
    
    // Mouse interaction state
    QPoint lastPanPoint;
    bool isPanning;
};