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
        groupLayout->setSpacing(10);

        // Tool 1: Cosmetic Re-render
        auto* rerenderBtn = new QPushButton("Quick Re-render Map Images", group);
        rerenderBtn->setToolTip("Re-runs the engine with no file arguments. Updates cosmetic choices (like north lines) instantly without parsing heavy point clouds again.");
        connect(rerenderBtn, &QPushButton::clicked, this, [this]() {
            emit toolActionRequested(QStringList()); // Empty arguments triggers standard re-render
        });
        groupLayout->addWidget(rerenderBtn);

        // Tool 2: Make Vege
        auto* vegeBtn = new QPushButton("Regenerate Vegetation Layers Only", group);
        vegeBtn->setToolTip("Re-calculates the green and yellow vegetation classifications using updated parameters.");
        connect(vegeBtn, &QPushButton::clicked, this, [this]() {
            emit toolActionRequested(QStringList() << "makevege");
        });
        groupLayout->addWidget(vegeBtn);

        // Tool 3: Make Cliffs (Requires inputs)
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
        cliffBtn->setToolTip("Re-calculates cliffs from the intermediate data utilizing custom smoothing and steepness rules.");
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
        groupLayout->setSpacing(10);

        // Shared Scale Multiplier Row
        auto* scaleWidget = new QWidget(group);
        auto* scaleForm = new QFormLayout(scaleWidget);
        scaleForm->setContentsMargins(0, 0, 0, 5);

        mergeScaleBox = new QSpinBox(scaleWidget);
        mergeScaleBox->setRange(1, 50);
        mergeScaleBox->setValue(1);
        mergeScaleBox->setToolTip("1 = Full scale (Large files!). 2 = 50% size reduction. 4 = 25% size reduction.");
        scaleForm->addRow("Downscale Factor (1 = Max Res):", mergeScaleBox);
        groupLayout->addWidget(scaleWidget);

        // Merge Actions
        auto* mergeStdBtn = new QPushButton("Merge Standard Raster Maps", group);
        connect(mergeStdBtn, &QPushButton::clicked, this, [this]() {
            emit toolActionRequested(QStringList() << "pngmerge" << QString::number(mergeScaleBox->value()));
        });
        groupLayout->addWidget(mergeStdBtn);

        auto* mergeDeprBtn = new QPushButton("Merge Depression Raster Maps", group);
        connect(mergeDeprBtn, &QPushButton::clicked, this, [this]() {
            emit toolActionRequested(QStringList() << "pngmergedepr" << QString::number(mergeScaleBox->value()));
        });
        groupLayout->addWidget(mergeDeprBtn);

        auto* mergeVegeBtn = new QPushButton("Merge Vegetation Backgrounds", group);
        connect(mergeVegeBtn, &QPushButton::clicked, this, [this]() {
            emit toolActionRequested(QStringList() << "pngmergevege" << QString::number(mergeScaleBox->value()));
        });
        groupLayout->addWidget(mergeVegeBtn);

        auto* mergeDxfBtn = new QPushButton("Merge DXF Vector Outputs", group);
        mergeDxfBtn->setToolTip("Combines all exported contour and cliff vector segments into unified DXF files.");
        connect(mergeDxfBtn, &QPushButton::clicked, this, [this]() {
            emit toolActionRequested(QStringList() << "dxfmerge");
        });
        groupLayout->addWidget(mergeDxfBtn);

        vbox->addWidget(group);
    }

    vbox->addStretch();
}