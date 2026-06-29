#include "PreviewPanel.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QStackedWidget>
#include <QFileSystemWatcher>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QPixmap>
#include <QResizeEvent>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QScrollBar>
#include <QFrame>
#include <QPoint>
#include <QEvent>


PreviewPanel::PreviewPanel(QWidget* parent)
    : QWidget(parent)
    , scene(new QGraphicsScene(this))
    , graphicsView(new QGraphicsView(scene, this))
    , pixmapItem(new QGraphicsPixmapItem())
    , currentScale(1.0)
    , minScale(0.01)
    , maxScale(10.0)
    , isPanning(false)
{
    // ---- Thumbnail list (left sidebar) ----------------------------------
    thumbnailList = new QListWidget();
    thumbnailList->setViewMode(QListView::IconMode);
    thumbnailList->setFlow(QListView::TopToBottom);
    thumbnailList->setWrapping(false);
    thumbnailList->setIconSize(QSize(120, 120));
    thumbnailList->setGridSize(QSize(140, 150));
    thumbnailList->setMovement(QListView::Static);
    thumbnailList->setResizeMode(QListView::Adjust);
    thumbnailList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    thumbnailList->setFixedWidth(160);
    thumbnailList->setSpacing(4);

    QLabel* sidebarTitle = new QLabel("Generated Maps");
    QFont boldFont = sidebarTitle->font();
    boldFont.setBold(true);
    sidebarTitle->setFont(boldFont);
    sidebarTitle->setAlignment(Qt::AlignCenter);

    QVBoxLayout* sidebarLayout = new QVBoxLayout();
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(4);
    sidebarLayout->addWidget(sidebarTitle);
    sidebarLayout->addWidget(thumbnailList);

    QWidget* sidebar = new QWidget();
    sidebar->setLayout(sidebarLayout);
    sidebar->setFixedWidth(160);

    // ---- Right side: title + stacked content ----------------------------
    imageTitle = new QLabel();
    imageTitle->setAlignment(Qt::AlignCenter);
    imageTitle->setWordWrap(true);
    imageTitle->setVisible(false);   // hidden until an image is selected

    // Page 0: placeholder  a plain label, no scroll area, no artifacts
    placeholderLabel = new QLabel(
        "No maps yet.\n\n"
        "Select an output folder and\n"
        "press Run  maps will appear\n"
        "here as each tile completes."
    );
    placeholderLabel->setAlignment(Qt::AlignCenter);

    // Setup Graphics View for zoom/pan
    setupGraphicsView();

    stack = new QStackedWidget();
    stack->addWidget(placeholderLabel);   // index 0
    stack->addWidget(graphicsView);      // index 1
    stack->setCurrentIndex(0);            // start on placeholder

    // Permanent header  always visible regardless of whether an image is loaded
    QLabel* previewHeader = new QLabel("Map Preview (Use mouse wheel to zoom, drag to pan)");
    QFont headerFont = previewHeader->font();
    headerFont.setBold(true);
    previewHeader->setFont(headerFont);
    previewHeader->setAlignment(Qt::AlignCenter);

    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(4, 0, 0, 0);
    rightLayout->setSpacing(4);
    rightLayout->addWidget(previewHeader);
    rightLayout->addWidget(imageTitle);   // filename  empty until a map is selected
    rightLayout->addWidget(stack, 1);

    QWidget* rightPane = new QWidget();
    rightPane->setLayout(rightLayout);

    // ---- Main layout ----------------------------------------------------
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(6);
    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(rightPane, 1);

    // ---- File system watcher --------------------------------------------
    watcher = new QFileSystemWatcher(this);

    connect(watcher, &QFileSystemWatcher::directoryChanged,
            this, &PreviewPanel::onDirectoryChanged);

    connect(thumbnailList, &QListWidget::itemClicked,
            this, &PreviewPanel::onItemClicked);

    // Install event filter for mouse events on graphics view
    graphicsView->viewport()->installEventFilter(this);
    graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);
    graphicsView->setRenderHint(QPainter::Antialiasing);
}

// --------------------------------------------------------------------------
// Private: setupGraphicsView
// --------------------------------------------------------------------------
void PreviewPanel::setupGraphicsView()
{
    // Configure the graphics view
    graphicsView->setScene(scene);
    graphicsView->setAlignment(Qt::AlignCenter);
    graphicsView->setFrameShape(QFrame::NoFrame);
    graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    graphicsView->setDragMode(QGraphicsView::ScrollHandDrag); // Enable drag to pan
    graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    graphicsView->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    
    // Add the pixmap item to the scene
    scene->addItem(pixmapItem);
    pixmapItem->setTransformationMode(Qt::SmoothTransformation);
}

// --------------------------------------------------------------------------
// Protected: eventFilter
// Handles mouse wheel for zooming and mouse press/move for panning
// --------------------------------------------------------------------------
bool PreviewPanel::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == graphicsView->viewport())
    {
        if (event->type() == QEvent::Wheel)
        {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            
            // Zoom in/out based on wheel delta
            const qreal scaleFactor = 1.15; // Zoom factor per wheel step
            
            if (wheelEvent->angleDelta().y() > 0)
            {
                // Zoom in
                if (currentScale < maxScale)
                {
                    currentScale *= scaleFactor;
                    if (currentScale > maxScale) currentScale = maxScale;
                    graphicsView->scale(scaleFactor, scaleFactor);
                }
            }
            else if (wheelEvent->angleDelta().y() < 0)
            {
                // Zoom out
                if (currentScale > minScale)
                {
                    currentScale /= scaleFactor;
                    if (currentScale < minScale) currentScale = minScale;
                    graphicsView->scale(1.0 / scaleFactor, 1.0 / scaleFactor);
                }
            }
            
            return true; // Event handled
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::MiddleButton)
            {
                // Start panning with middle mouse button
                lastPanPoint = mouseEvent->globalPosition().toPoint();
                isPanning = true;
                graphicsView->setCursor(Qt::ClosedHandCursor);
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::MiddleButton)
            {
                // Stop panning
                isPanning = false;
                graphicsView->setCursor(Qt::ArrowCursor);
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (isPanning)
            {
                // Calculate pan distance
                QPoint currentPos = mouseEvent->globalPosition().toPoint();
                QPoint delta = currentPos - lastPanPoint;
                lastPanPoint = currentPos;
                
                // Move the scene
                QScrollBar* hBar = graphicsView->horizontalScrollBar();
                QScrollBar* vBar = graphicsView->verticalScrollBar();
                
                hBar->setValue(hBar->value() - delta.x());
                vBar->setValue(vBar->value() - delta.y());
                
                return true;
            }
        }
    }
    
    return QWidget::eventFilter(obj, event);
}

// --------------------------------------------------------------------------
// Public: setOutputFolder
// --------------------------------------------------------------------------
void PreviewPanel::setOutputFolder(const QString& folderPath)
{
    if (!outputFolder.isEmpty() && watcher->directories().contains(outputFolder))
        watcher->removePath(outputFolder);

    clear();
    outputFolder = folderPath;

    if (!QDir(folderPath).exists())
        return;

    watcher->addPath(folderPath);
    scanFolder();
}

// --------------------------------------------------------------------------
// Public: clear
// --------------------------------------------------------------------------
void PreviewPanel::clear()
{
    thumbnailList->clear();
    loadedFiles.clear();
    currentImagePath.clear();
    showPlaceholder();
    
    // Reset graphics view
    scene->clear();
    pixmapItem = new QGraphicsPixmapItem();
    scene->addItem(pixmapItem);
    pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    currentScale = 1.0;
    graphicsView->resetTransform();
}

// --------------------------------------------------------------------------
// Protected: resizeEvent
// --------------------------------------------------------------------------
void PreviewPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (!currentImagePath.isEmpty())
        showImage(currentImagePath);
}

// --------------------------------------------------------------------------
// Private slot: onDirectoryChanged
// --------------------------------------------------------------------------
void PreviewPanel::onDirectoryChanged(const QString& /*path*/)
{
    scanFolder();
}

// --------------------------------------------------------------------------
// Private slot: onItemClicked
// --------------------------------------------------------------------------
void PreviewPanel::onItemClicked(QListWidgetItem* item)
{
    if (!item)
        return;

    QString filePath = item->data(Qt::UserRole).toString();
    if (!filePath.isEmpty())
        showImage(filePath);
}

// --------------------------------------------------------------------------
// Private: scanFolder
// --------------------------------------------------------------------------
void PreviewPanel::scanFolder()
{
    if (outputFolder.isEmpty())
        return;

    QDir dir(outputFolder);
    const QStringList pngFiles = dir.entryList({ "*.png", "*.PNG" }, QDir::Files,
                                               QDir::Name);

    for (const QString& filename : pngFiles)
    {
        const QString fullPath = dir.absoluteFilePath(filename);
        if (!loadedFiles.contains(fullPath))
            addImageFile(fullPath);
    }
}

// --------------------------------------------------------------------------
// Private: addImageFile
// --------------------------------------------------------------------------
void PreviewPanel::addImageFile(const QString& filePath)
{
    QImageReader reader(filePath);
    if (!reader.canRead())
        return;

    QSize origSize = reader.size();
    if (origSize.isEmpty())
        return;

    QSize thumbSize = origSize.scaled(120, 120, Qt::KeepAspectRatio);
    reader.setScaledSize(thumbSize);

    QImage img = reader.read();
    if (img.isNull())
        return;

    QListWidgetItem* item = new QListWidgetItem();
    item->setIcon(QIcon(QPixmap::fromImage(img)));
    item->setText(QFileInfo(filePath).fileName());
    item->setData(Qt::UserRole, filePath);
    item->setToolTip(filePath);

    thumbnailList->addItem(item);
    loadedFiles.insert(filePath);

    // Auto-select and display the first image that arrives
    if (thumbnailList->count() == 1)
    {
        thumbnailList->setCurrentRow(0);
        showImage(filePath);
    }
}

// --------------------------------------------------------------------------
// Private: showImage
// Switches the stack to the graphics view page and loads the image.
// --------------------------------------------------------------------------
void PreviewPanel::showImage(const QString& filePath)
{
    currentImagePath = filePath;

    QImageReader reader(filePath);
    if (!reader.canRead())
        return;

    QImage img = reader.read();
    if (img.isNull())
        return;

    // Reset transform and scale before loading new image
    graphicsView->resetTransform();
    currentScale = 1.0;
    
    // Set the pixmap
    pixmapItem->setPixmap(QPixmap::fromImage(img));
    
    // Fit to window initially
    fitToWindow();

    imageTitle->setText(QFileInfo(filePath).fileName());
    imageTitle->setVisible(true);   // show filename bar once an image is loaded

    // Switch to the image page  placeholder disappears cleanly
    stack->setCurrentIndex(1);
}

// --------------------------------------------------------------------------
// Private: showPlaceholder
// Switches the stack to the plain label page  no scroll area, no artifacts.
// --------------------------------------------------------------------------
void PreviewPanel::showPlaceholder()
{
    imageTitle->clear();
    imageTitle->setVisible(false);  // no empty label hanging above the placeholder
    stack->setCurrentIndex(0);
    
    // Reset graphics view state
    scene->clear();
    pixmapItem = new QGraphicsPixmapItem();
    scene->addItem(pixmapItem);
    pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    currentScale = 1.0;
    graphicsView->resetTransform();
}

// --------------------------------------------------------------------------
// Private: fitToWindow
// Scales the image to fit the current viewport width
// --------------------------------------------------------------------------
void PreviewPanel::fitToWindow()
{
    if (!pixmapItem->pixmap().isNull())
    {
        const int margin = 20; // Small margin around the image
        QRectF viewRect = graphicsView->viewport()->rect();
        viewRect.adjust(margin, margin, -margin, -margin);
        
        if (viewRect.width() > 0 && viewRect.height() > 0)
        {
            QRectF imageRect = pixmapItem->boundingRect();
            qreal scale = viewRect.width() / imageRect.width();
            
            // Don't scale up if image is smaller than viewport
            if (imageRect.width() < viewRect.width() && imageRect.height() < viewRect.height())
            {
                scale = 1.0;
            }
            
            // Reset transform and apply new scale
            graphicsView->resetTransform();
            graphicsView->scale(scale, scale);
            currentScale = scale;
            
            // Center the image
            graphicsView->centerOn(pixmapItem);
        }
    }
}