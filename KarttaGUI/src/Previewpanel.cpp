#include "PreviewPanel.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QScrollArea>
#include <QFileSystemWatcher>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QPixmap>
#include <QResizeEvent>
#include <QFont>


PreviewPanel::PreviewPanel(QWidget* parent)
    : QWidget(parent)
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

    // ---- Large image view (right area) ----------------------------------
    previewLabel = new QLabel();
    previewLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    previewLabel->setMinimumSize(1, 1);

    previewScroll = new QScrollArea();
    previewScroll->setWidget(previewLabel);
    previewScroll->setWidgetResizable(false);
    previewScroll->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    imageTitle = new QLabel();
    imageTitle->setAlignment(Qt::AlignCenter);
    imageTitle->setWordWrap(true);

    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(4, 0, 0, 0);
    rightLayout->setSpacing(4);
    rightLayout->addWidget(imageTitle);
    rightLayout->addWidget(previewScroll, 1);

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

    showPlaceholder();
}

// --------------------------------------------------------------------------
// Public: setOutputFolder
// --------------------------------------------------------------------------
void PreviewPanel::setOutputFolder(const QString& folderPath)
{
    // Stop watching the old folder
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
}

// --------------------------------------------------------------------------
// Protected: resizeEvent
// Refits the current image to the (possibly new) scroll area width.
// --------------------------------------------------------------------------
void PreviewPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (!currentImagePath.isEmpty())
        showImage(currentImagePath);
}

// --------------------------------------------------------------------------
// Private slot: onDirectoryChanged
// Fires whenever a file is added or removed in the watched folder.
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
// Finds all PNG files not yet shown and adds them.
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
// Loads a scaled thumbnail and adds it to the list.
// Uses QImageReader::setScaledSize() to avoid loading the full image.
// --------------------------------------------------------------------------
void PreviewPanel::addImageFile(const QString& filePath)
{
    QImageReader reader(filePath);
    if (!reader.canRead())
        return;

    // Scale to thumbnail size while preserving aspect ratio
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

    // Auto-select the first image that arrives
    if (thumbnailList->count() == 1)
    {
        thumbnailList->setCurrentRow(0);
        showImage(filePath);
    }
}

// --------------------------------------------------------------------------
// Private: showImage
// Loads and scales the image to fit the scroll area's current viewport width.
// --------------------------------------------------------------------------
void PreviewPanel::showImage(const QString& filePath)
{
    currentImagePath = filePath;

    const int maxWidth = previewScroll->viewport()->width();
    if (maxWidth <= 0)
        return;

    QImageReader reader(filePath);
    if (!reader.canRead())
        return;

    QSize origSize = reader.size();
    if (origSize.isEmpty())
        return;

    // Only downscale — never upscale beyond original size
    if (origSize.width() > maxWidth)
    {
        QSize scaled = origSize.scaled(maxWidth, INT_MAX, Qt::KeepAspectRatio);
        reader.setScaledSize(scaled);
    }

    QImage img = reader.read();
    if (img.isNull())
        return;

    previewLabel->setPixmap(QPixmap::fromImage(img));
    previewLabel->adjustSize();

    imageTitle->setText(QFileInfo(filePath).fileName());
}

// --------------------------------------------------------------------------
// Private: showPlaceholder
// --------------------------------------------------------------------------
void PreviewPanel::showPlaceholder()
{
    imageTitle->clear();
    previewLabel->setPixmap(QPixmap());
    previewLabel->setText(
        "No maps yet.\n\n"
        "Select an output folder and\n"
        "press Run — maps will appear\n"
        "here as each tile completes."
    );
    previewLabel->setAlignment(Qt::AlignCenter);
}