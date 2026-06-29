#include "PreviewPanel.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QScrollArea>
#include <QStackedWidget>
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

    // ---- Right side: title + stacked content ----------------------------
    imageTitle = new QLabel();
    imageTitle->setAlignment(Qt::AlignCenter);
    imageTitle->setWordWrap(true);
    imageTitle->setVisible(false);   // hidden until an image is selected

    // Page 0: placeholder — a plain label, no scroll area, no artifacts
    placeholderLabel = new QLabel(
        "No maps yet.\n\n"
        "Select an output folder and\n"
        "press Run — maps will appear\n"
        "here as each tile completes."
    );
    placeholderLabel->setAlignment(Qt::AlignCenter);

    // Page 1: the actual image inside a scroll area
    previewLabel = new QLabel();
    previewLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    previewLabel->setMinimumSize(1, 1);

    previewScroll = new QScrollArea();
    previewScroll->setWidget(previewLabel);
    previewScroll->setWidgetResizable(false);
    previewScroll->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    // Hide the scroll area frame so it doesn't draw a border
    previewScroll->setFrameShape(QFrame::NoFrame);

    stack = new QStackedWidget();
    stack->addWidget(placeholderLabel);   // index 0
    stack->addWidget(previewScroll);      // index 1
    stack->setCurrentIndex(0);            // start on placeholder

    // Permanent header — always visible regardless of whether an image is loaded
    QLabel* previewHeader = new QLabel("Map Preview");
    QFont headerFont = previewHeader->font();
    headerFont.setBold(true);
    previewHeader->setFont(headerFont);
    previewHeader->setAlignment(Qt::AlignCenter);

    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(4, 0, 0, 0);
    rightLayout->setSpacing(4);
    rightLayout->addWidget(previewHeader);
    rightLayout->addWidget(imageTitle);   // filename — empty until a map is selected
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
// Switches the stack to the scroll area page and loads the image.
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
    imageTitle->setVisible(true);   // show filename bar once an image is loaded

    // Switch to the image page — placeholder disappears cleanly
    stack->setCurrentIndex(1);
}

// --------------------------------------------------------------------------
// Private: showPlaceholder
// Switches the stack to the plain label page — no scroll area, no artifacts.
// --------------------------------------------------------------------------
void PreviewPanel::showPlaceholder()
{
    imageTitle->clear();
    imageTitle->setVisible(false);  // no empty label hanging above the placeholder
    stack->setCurrentIndex(0);
}