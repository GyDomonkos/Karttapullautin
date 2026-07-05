#include "ToolsPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>

ToolsPanel::ToolsPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* mainVBox = new QVBoxLayout(this);
    mainVBox->setContentsMargins(10, 10, 10, 10);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    mainVBox->addWidget(scroll);

    auto* content = new QWidget();
    auto* vbox = new QVBoxLayout(content);
    vbox->setSpacing(15);
    vbox->setContentsMargins(0, 0, 0, 0);
    scroll->setWidget(content);

    // =========================================================================
    // Group 1: Iterative / Partial Processing
    // =========================================================================
    {
        auto* group = new QGroupBox("Iterative & Partial Processing", content);
        auto* groupLayout = new QVBoxLayout(group);
        groupLayout->setSpacing(8);

        // Requirement notice
        auto* note = new QLabel(
            "<i>Vegetation and cliff regeneration require intermediate temp data "
            "from the previous run.<br>"
            "Enable <b>Save temp folders</b> in Settings before running the batch.</i>");
        note->setWordWrap(true);
        note->setTextFormat(Qt::RichText);
        groupLayout->addWidget(note);

        // Separator
        auto* sep = new QFrame();
        sep->setFrameShape(QFrame::HLine);
        sep->setFrameShadow(QFrame::Sunken);
        groupLayout->addWidget(sep);

        // Tool 1: Cosmetic Re-render
        auto* rerenderBtn = new QPushButton("Quick Re-render Map Images", group);
        rerenderBtn->setToolTip(
            "Re-renders the PNG map images using the current settings without\n"
            "reprocessing the point cloud. Useful for tweaking north lines,\n"
            "contour appearance, and other cosmetic parameters.\n\n"
            "Requires that the temp folder still contains data from the last run.");
        connect(rerenderBtn, &QPushButton::clicked, this, [this]() {
            emit toolActionRequested(QStringList());
        });
        groupLayout->addWidget(rerenderBtn);

        // Tool 2: Make Vege
        auto* vegeBtn = new QPushButton("Regenerate Vegetation Layers Only", group);
        vegeBtn->setToolTip(
            "Re-calculates green and yellow vegetation without reprocessing\n"
            "the full point cloud. Useful for iterating undergrowth and\n"
            "greenshade thresholds.\n\n"
            "Requires: Save temp folders = ON in Settings before the batch run.\n"
            "After this runs, press Quick Re-render to update the PNG output.");
        connect(vegeBtn, &QPushButton::clicked, this, [this]() {
            emit toolActionRequested(QStringList() << "makevege");
        });
        groupLayout->addWidget(vegeBtn);

        // Tool 3: Make Cliffs
        auto* cliffSubGroup = new QWidget(group);
        auto* cliffForm = new QFormLayout(cliffSubGroup);
        cliffForm->setContentsMargins(0, 5, 0, 0);

        cliffSmoothBox = new QDoubleSpinBox(cliffSubGroup);
        cliffSmoothBox->setRange(0.1, 10.0);
        cliffSmoothBox->setSingleStep(0.05);
        cliffSmoothBox->setValue(1.0);
        cliffForm->addRow("Cliff Smoothing:", cliffSmoothBox);

        cliffSteepBox = new QDoubleSpinBox(cliffSubGroup);
        cliffSteepBox->setRange(0.1, 10.0);
        cliffSteepBox->setSingleStep(0.05);
        cliffSteepBox->setValue(1.15);
        cliffForm->addRow("Cliff Steepness Factor:", cliffSteepBox);

        auto* cliffBtn = new QPushButton("Regenerate Cliffs Only", cliffSubGroup);
        cliffBtn->setToolTip(
            "Re-calculates cliff features from intermediate point data\n"
            "using the smoothing and steepness values above.\n\n"
            "Requires: Save temp folders = ON in Settings before the batch run.\n"
            "Command: pullauta makecliffs xyztemp.xyz <smooth> <steep>\n"
            "After this runs, press Quick Re-render to update the PNG output.");
        connect(cliffBtn, &QPushButton::clicked, this, [this]() {
            QStringList args;
            args << "makecliffs" << "xyztemp.xyz"
                 << QString::number(cliffSmoothBox->value(), 'f', 2)
                 << QString::number(cliffSteepBox->value(), 'f', 2);
            emit toolActionRequested(args);
        });
        cliffForm->addRow(cliffBtn);

        groupLayout->addWidget(cliffSubGroup);
        vbox->addWidget(group);
    }

    // =========================================================================
    // Group 2: Batch Tile Merging
    // =========================================================================
    {
        auto* group = new QGroupBox("Batch Output Tile Merging", content);
        auto* groupLayout = new QVBoxLayout(group);
        groupLayout->setSpacing(8);

        // Requirement notice
        auto* note = new QLabel(
            "<i>Merge tools combine per-tile output files into a single image.\n"
            "PNG/DXF merge requires <b>Save temp files</b> to be enabled in Settings\n"
            "before the batch run so per-tile files are copied to the output folder.</i>");
        note->setWordWrap(true);
        note->setTextFormat(Qt::RichText);
        groupLayout->addWidget(note);

        auto* sep = new QFrame();
        sep->setFrameShape(QFrame::HLine);
        sep->setFrameShadow(QFrame::Sunken);
        groupLayout->addWidget(sep);

        // Shared scale multiplier
        auto* scaleWidget = new QWidget(group);
        auto* scaleForm = new QFormLayout(scaleWidget);
        scaleForm->setContentsMargins(0, 0, 0, 5);

        mergeScaleBox = new QSpinBox(scaleWidget);
        mergeScaleBox->setRange(1, 50);
        mergeScaleBox->setValue(1);
        mergeScaleBox->setToolTip(
            "1 = full resolution (large files).\n"
            "2 = 50% size,  4 = 25% size,  10 = 10% size.\n"
            "Start with a higher value for a quick preview.");
        scaleForm->addRow("Downscale factor (1 = max res):", mergeScaleBox);
        groupLayout->addWidget(scaleWidget);

        // Merge Standard
        auto* mergeStdBtn = new QPushButton("Merge Standard Raster Maps", group);
        mergeStdBtn->setToolTip(
            "Stitches all per-tile PNG map files into a single output image.\n"
            "Produces merged.png / merged.jpg + world files.\n"
            "No extra setting required — tile PNGs are always in the output folder.");
        connect(mergeStdBtn, &QPushButton::clicked, this, [this]() {
            emit toolActionRequested(QStringList() << "pngmerge"
                                                   << QString::number(mergeScaleBox->value()));
        });
        groupLayout->addWidget(mergeStdBtn);

        // Merge Depression
        auto* mergeDeprBtn = new QPushButton("Merge Depression Raster Maps", group);
        mergeDeprBtn->setToolTip(
            "Stitches the depression-variant PNG tiles into a single image.\n"
            "Produces merged_depr.png / merged_depr.jpg + world files.\n"
            "No extra setting required.");
        connect(mergeDeprBtn, &QPushButton::clicked, this, [this]() {
            emit toolActionRequested(QStringList() << "pngmergedepr"
                                                   << QString::number(mergeScaleBox->value()));
        });
        groupLayout->addWidget(mergeDeprBtn);

        // Merge Vege — NOTE: no scale argument (not supported by pullauta)
        auto* mergeVegeBtn = new QPushButton("Merge Vegetation Backgrounds", group);
        mergeVegeBtn->setToolTip(
            "Stitches per-tile vegetation PNG backgrounds into one image.\n"
            "Requires: Save temp files = ON in Settings before the batch run\n"
            "so that _vege.png files are copied to the output folder.\n\n"
            "Note: scale factor does not apply to this command.");
        connect(mergeVegeBtn, &QPushButton::clicked, this, [this]() {
            // pngmergevege takes no scale argument — passing one is ignored at best,
            // may cause unexpected behaviour at worst.
            emit toolActionRequested(QStringList() << "pngmergevege");
        });
        groupLayout->addWidget(mergeVegeBtn);

        // Merge DXF
        auto* mergeDxfBtn = new QPushButton("Merge DXF Vector Outputs", group);
        mergeDxfBtn->setToolTip(
            "Combines per-tile DXF contour/cliff files into unified DXF outputs.\n"
            "Requires two things to be enabled in Settings before the batch run:\n"
            "  • DXF output = ON  (writes .dxf files during processing)\n"
            "  • Save temp files = ON  (copies them to the output folder)");
        connect(mergeDxfBtn, &QPushButton::clicked, this, [this]() {
            emit toolActionRequested(QStringList() << "dxfmerge");
        });
        groupLayout->addWidget(mergeDxfBtn);

        vbox->addWidget(group);
    }

    vbox->addStretch();
}