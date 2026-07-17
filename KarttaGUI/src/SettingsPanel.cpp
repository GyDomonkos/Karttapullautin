#include "SettingsPanel.h"
#include "KarttaRunner.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QFile>
#include <QTextStream>
#include <QFrame>
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonArray>
#include <QRegularExpression>
#include "PresetManager.h"
#include <qcoreapplication.h>


SettingsPanel::SettingsPanel(QWidget* parent)
    : QWidget(parent)
{
    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget*     content = new QWidget();
    QVBoxLayout* vbox    = new QVBoxLayout(content);
    vbox->setSpacing(10);
    vbox->setContentsMargins(6, 6, 6, 6);

    // =========================================================================
    // Contours
    // =========================================================================
    {
        QGroupBox*   group = new QGroupBox("Contours");
        QFormLayout* form  = new QFormLayout(group);

        contourInterval = makeDouble(0.5, 20.0, 0.5, 1, 5.0,
            "Distance between main contours in metres");
        form->addRow("Contour interval (m):", contourInterval);

        formlineMode = new QComboBox();
        formlineMode->addItem("0 — 2.5 m, no form lines");
        formlineMode->addItem("1 — 2.5 m, every second line thick");
        formlineMode->addItem("2 — 5 m, dashed form lines (recommended)");
        formlineMode->setCurrentIndex(2);
        form->addRow("Form lines:", formlineMode);

        smoothing = makeDouble(0.5, 3.0, 0.1, 1, 1.0,
            "Larger values smooth contours more. Default 1.0");
        form->addRow("Smoothing:", smoothing);

        curviness = makeDouble(0.5, 2.0, 0.1, 1, 1.0,
            "Larger values exaggerate reentrants and spurs. Default 1.0");
        form->addRow("Curviness:", curviness);

        knolls = makeDouble(0.0, 1.0, 0.05, 2, 0.8,
            "Higher values produce fewer but more distinct knolls. Default 0.8");
        form->addRow("Knoll sensitivity:", knolls);

        indexContours = makeDouble(0.0, 50.0, 0.5, 1, 12.5,
            "Interval for index (thick) contours. 0 = none");
        form->addRow("Index contour interval (m):", indexContours);

        vbox->addWidget(group);
    }

    // =========================================================================
    // Vegetation
    // =========================================================================
    {
        QGroupBox*   group   = new QGroupBox("Vegetation");
        QVBoxLayout* gLayout = new QVBoxLayout(group);

        QFormLayout* form = new QFormLayout();
        gLayout->addLayout(form);

        undergrowth = makeDouble(0.0, 5.0, 0.05, 2, 0.35,
            "LiDAR density threshold for slow-run (light green) vegetation");
        form->addRow("Undergrowth — slow run:", undergrowth);

        undergrowth2 = makeDouble(0.0, 5.0, 0.05, 2, 0.56,
            "LiDAR density threshold for walk (medium green) vegetation");
        form->addRow("Undergrowth — walk:", undergrowth2);

        greenground = makeDouble(0.0, 5.0, 0.05, 2, 0.9,
            "Density threshold for ground-level vegetation");
        form->addRow("Green ground:", greenground);

        greenhigh = makeDouble(0.0, 10.0, 0.1, 1, 2.0,
            "Height above which vegetation is classified as high (m)");
        form->addRow("Green high (m):", greenhigh);

        yellowheight = makeDouble(0.0, 5.0, 0.1, 1, 0.9,
            "Height threshold for rough open (yellow) ground");
        form->addRow("Yellow height (m):", yellowheight);

        yellowthresold = makeDouble(0.0, 5.0, 0.05, 2, 0.9,
            "Density threshold for yellow ground classification");
        form->addRow("Yellow threshold:", yellowthresold);

        // ---- Green shade levels (dynamic) --------------------------------
        QLabel* shadesHeader = new QLabel("Green shade thresholds:");
        {
            QFont f = shadesHeader->font();
            f.setBold(true);
            shadesHeader->setFont(f);
            shadesHeader->setToolTip(
                "Ordered density thresholds that map LiDAR point density to map\n"
                "green shades. Each value is the lower bound for that shade level.\n"
                "Standard OMaps use 3 levels (slow run, walk, fight).");
        }
        gLayout->addWidget(shadesHeader);

        // Container for dynamic rows
        QWidget* shadesContainer = new QWidget();
        shadesLayout = new QVBoxLayout(shadesContainer);
        shadesLayout->setContentsMargins(0, 0, 0, 0);
        shadesLayout->setSpacing(2);
        gLayout->addWidget(shadesContainer);

        // Add Level button
        QPushButton* addBtn = new QPushButton("+ Add Shade Level");
        addBtn->setToolTip("Append another green shade density threshold");
        gLayout->addWidget(addBtn);

        connect(addBtn, &QPushButton::clicked, this, [this]() {
            addShadeLevel(1.0);
        });

        // Default 3 levels — proven values for standard OMaps
        addShadeLevel(0.8);
        addShadeLevel(1.3);
        addShadeLevel(2.0);

        vbox->addWidget(group);
    }

    // =========================================================================
    // Cliffs
    // =========================================================================
    {
        QGroupBox*   group = new QGroupBox("Cliffs");
        QFormLayout* form  = new QFormLayout(group);

        cliff1 = makeDouble(0.5, 5.0, 0.05, 2, 1.15,
            "Steepness threshold for passable (tag) cliffs");
        form->addRow("Cliff 1 — passable:", cliff1);

        cliff2 = makeDouble(0.5, 5.0, 0.05, 2, 2.0,
            "Steepness threshold for impassable cliffs");
        form->addRow("Cliff 2 — impassable:", cliff2);

        cliffsteepfactor = makeDouble(0.0, 1.0, 0.01, 2, 0.38,
            "Factor weighting how steeply a slope must rise to qualify as a cliff");
        form->addRow("Steep factor:", cliffsteepfactor);

        cliffflatplace = makeDouble(0.5, 10.0, 0.5, 1, 3.5,
            "Flat-area radius that suppresses nearby cliffs");
        form->addRow("Flat place threshold:", cliffflatplace);

        cliffnosmallciffs = makeDouble(0.0, 20.0, 0.5, 1, 5.5,
            "Minimum cliff length in metres; shorter features are removed");
        form->addRow("Min cliff length (m):", cliffnosmallciffs);

        vbox->addWidget(group);
    }

    // =========================================================================
    // Processing
    // =========================================================================
    {
        QGroupBox*   group = new QGroupBox("Processing");
        QFormLayout* form  = new QFormLayout(group);

        processes = makeInt(1, 32, 2,
            "Number of tiles processed simultaneously");
        form->addRow("Parallel processes:", processes);

        northlinesangle = makeDouble(-360.0, 360.0, 1.0, 1, 0.0,
            "Angle in degrees for magnetic north lines. 0 = no north lines");
        form->addRow("North lines angle (°):", northlinesangle);

        northlineswidth = makeInt(0, 20, 0,
            "Width of north lines in pixels. 0 = none");
        form->addRow("North lines width (px):", northlineswidth);

        scalefactor = makeDouble(0.1, 4.0, 0.1, 2, 1.0,
            "Output map scale multiplier");
        form->addRow("Scale factor:", scalefactor);

        zoffset = makeDouble(-1000.0, 1000.0, 0.5, 1, 0.0,
            "Vertical offset applied to all elevation values (m)");
        form->addRow("Z offset (m):", zoffset);

        outputDxf = new QCheckBox("Export DXF contour layer");
        outputDxf->setToolTip(
            "Write DXF vector contour/cliff files alongside each PNG tile.\n"
            "Required for the DXF Merge tool in the Tools tab.");
        form->addRow("DXF output:", outputDxf);

        savetempfiles = new QCheckBox("Save vector/vege files per tile to output folder");
        savetempfiles->setToolTip(
            "Copies each tile's DXF and vegetation PNG files into the output folder.\n"
            "Required for the 'Merge DXF' and 'Merge Vegetation' tools.\n"
            "Enable this before running the batch if you plan to use those tools.");
        form->addRow("Save temp files:", savetempfiles);

        savetempfolders = new QCheckBox("Save full temp folder per tile");
        savetempfolders->setToolTip(
            "Keeps the complete intermediate data for every processed tile.\n"
            "Required for 'Regenerate Vegetation' and 'Regenerate Cliffs' tools.\n"
            "Warning: significantly increases disk usage for large batches.");
        form->addRow("Save temp folders:", savetempfolders);

        vbox->addWidget(group);
    }

    // =========================================================================
    // Optional Features
    // Controlled by checkboxes; disabled features are commented out in the INI.
    // =========================================================================
    {
        QGroupBox*   group = new QGroupBox("Optional Features");
        QFormLayout* form  = new QFormLayout(group);

        // Water class
        {
            waterclassCheck = new QCheckBox("Water classification class:");
            waterclassCheck->setToolTip(
                "If your LiDAR has a water classification, enter the class number here.\n"
                "Water points will be rendered in blue on the map.");
            waterclassBox = makeInt(0, 99, 9);
            waterclassBox->setEnabled(false);
            form->addRow(waterclassCheck, waterclassBox);
            connect(waterclassCheck, &QCheckBox::toggled,
                    waterclassBox, &QSpinBox::setEnabled);
        }

        // Water elevation
        {
            waterElevCheck = new QCheckBox("Water elevation threshold (m):");
            waterElevCheck->setToolTip(
                "Elevation below which terrain is assumed to be water and drawn in blue.\n"
                "Useful when water is not separately classified in the LiDAR.");
            waterElevBox = makeDouble(-100.0, 9999.0, 0.05, 2, 0.15);
            waterElevBox->setEnabled(false);
            form->addRow(waterElevCheck, waterElevBox);
            connect(waterElevCheck, &QCheckBox::toggled,
                    waterElevBox, &QDoubleSpinBox::setEnabled);
        }

        // Buildings class
        {
            buildingsclassCheck = new QCheckBox("Buildings classification class:");
            buildingsclassCheck->setToolTip(
                "If your LiDAR has a building classification, enter the class number.\n"
                "Building outlines will be drawn in black on the map.");
            buildingsclassBox = makeInt(0, 99, 6);
            buildingsclassBox->setEnabled(false);
            form->addRow(buildingsclassCheck, buildingsclassBox);
            connect(buildingsclassCheck, &QCheckBox::toggled,
                    buildingsclassBox, &QSpinBox::setEnabled);
        }

        // Detect buildings (always active, just 0/1)
        {
            detectBuildingsCheck = new QCheckBox("Detect buildings automatically");
            detectBuildingsCheck->setToolTip(
                "Experimental: attempts to detect and outline buildings from LiDAR.\n"
                "Drawn in purple with black edges.");
            form->addRow("Building detection:", detectBuildingsCheck);
        }

        vbox->addWidget(group);
    }

    vbox->addStretch();

    // Reset button — always visible at the bottom of the scroll area
    QPushButton* resetBtn = new QPushButton("Reset all settings to defaults");
    resetBtn->setToolTip("Restores every parameter to its hardcoded default value");
    vbox->addWidget(resetBtn);
    connect(resetBtn, &QPushButton::clicked, this, &SettingsPanel::resetToDefaults);

    scroll->setWidget(content);

    // ---- Preset bar (always visible above the scroll area) ---------------
    QWidget*     presetBar    = new QWidget();
    QHBoxLayout* presetLayout = new QHBoxLayout(presetBar);
    presetLayout->setContentsMargins(6, 6, 6, 4);
    presetLayout->setSpacing(6);

    presetCombo = new QComboBox();
    presetCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    presetCombo->setToolTip("Saved settings presets");

    QPushButton* loadPresetBtn   = new QPushButton("Load");
    QPushButton* savePresetBtn   = new QPushButton("Save As…");
    QPushButton* deletePresetBtn = new QPushButton("Delete");
    loadPresetBtn->setFixedWidth(52);
    savePresetBtn->setFixedWidth(80);
    deletePresetBtn->setFixedWidth(60);
    loadPresetBtn->setToolTip("Load the selected preset into the settings form");
    savePresetBtn->setToolTip("Save the current settings as a named preset");
    deletePresetBtn->setToolTip("Permanently delete the selected preset");

    presetLayout->addWidget(new QLabel("Preset:"));
    presetLayout->addWidget(presetCombo, 1);
    presetLayout->addWidget(loadPresetBtn);
    presetLayout->addWidget(savePresetBtn);
    presetLayout->addWidget(deletePresetBtn);

    connect(loadPresetBtn,   &QPushButton::clicked, this, &SettingsPanel::onLoadPreset);
    connect(savePresetBtn,   &QPushButton::clicked, this, &SettingsPanel::onSavePreset);
    connect(deletePresetBtn, &QPushButton::clicked, this, &SettingsPanel::onDeletePreset);

    // Separator line between preset bar and scroll area
    QFrame* sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);

    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    outerLayout->addWidget(presetBar);
    outerLayout->addWidget(sep);
    outerLayout->addWidget(scroll, 1);

    refreshPresetList();
}

// --------------------------------------------------------------------------
// Public: resetToDefaults
// --------------------------------------------------------------------------
void SettingsPanel::resetToDefaults()
{
    // Contours
    contourInterval->setValue(5.0);
    formlineMode->setCurrentIndex(2);
    smoothing->setValue(1.0);
    curviness->setValue(1.0);
    knolls->setValue(0.8);
    indexContours->setValue(12.5);

    // Vegetation
    undergrowth->setValue(0.35);
    undergrowth2->setValue(0.56);
    greenground->setValue(0.9);
    greenhigh->setValue(2.0);
    yellowheight->setValue(0.9);
    yellowthresold->setValue(0.9);

    clearShadeLevels();
    addShadeLevel(0.8);
    addShadeLevel(1.3);
    addShadeLevel(2.0);

    // Cliffs
    cliff1->setValue(1.15);
    cliff2->setValue(2.0);
    cliffsteepfactor->setValue(0.38);
    cliffflatplace->setValue(3.5);
    cliffnosmallciffs->setValue(5.5);

    // Processing
    processes->setValue(2);
    northlinesangle->setValue(0.0);
    northlineswidth->setValue(0);
    scalefactor->setValue(1.0);
    zoffset->setValue(0.0);
    outputDxf->setChecked(false);
    savetempfiles->setChecked(false);
    savetempfolders->setChecked(false);

    // Optional features — all off by default
    waterclassCheck->setChecked(false);
    waterclassBox->setValue(9);
    waterElevCheck->setChecked(false);
    waterElevBox->setValue(0.15);
    buildingsclassCheck->setChecked(false);
    buildingsclassBox->setValue(6);
    detectBuildingsCheck->setChecked(false);
}

// --------------------------------------------------------------------------
// Public: loadFromIni
// --------------------------------------------------------------------------
void SettingsPanel::loadFromIni(const QString& iniPath)
{
    // If the INI file doesn't exist, try to run the Karttapullautin executable to generate it
    QFile file(iniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        const QString baseDir = QCoreApplication::applicationDirPath() + "/karttapullautin";
        const QString iniPath = baseDir + "/pullauta.ini";
        KarttaRunner *runner = new KarttaRunner(this);
        #ifdef Q_OS_WIN
            const QString executable = baseDir + "/pullauta.exe";
        #else
            const QString executable = baseDir + "/pullauta";
        #endif

        runner->run(executable, QStringList());
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QMap<QString, QString> ini;
    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('[') ||
            line.startsWith('#') || line.startsWith('%'))
            continue;
        const int eq = line.indexOf('=');
        if (eq < 0) continue;
        ini[line.left(eq).trimmed()] = line.mid(eq + 1).trimmed();
    }

    auto setDbl = [&](QDoubleSpinBox* sb, const QString& key) {
        if (!ini.contains(key)) return;
        bool ok; double v = ini[key].toDouble(&ok);
        if (ok) sb->setValue(v);
    };
    auto setInt = [&](QSpinBox* sb, const QString& key) {
        if (!ini.contains(key)) return;
        bool ok; int v = ini[key].toInt(&ok);
        if (ok) sb->setValue(v);
    };

    // Contours
    setDbl(contourInterval, "contour_interval");
    if (ini.contains("formline")) {
        bool ok; int v = ini["formline"].toInt(&ok);
        if (ok && v >= 0 && v <= 2) formlineMode->setCurrentIndex(v);
    }
    setDbl(smoothing,     "smoothing");
    setDbl(curviness,     "curviness");
    setDbl(knolls,        "knolls");
    setDbl(indexContours, "indexcontours");

    // Vegetation
    setDbl(undergrowth,    "undergrowth");
    setDbl(undergrowth2,   "undergrowth2");
    setDbl(greenground,    "greenground");
    setDbl(greenhigh,      "greenhigh");
    setDbl(yellowheight,   "yellowheight");
    setDbl(yellowthresold, "yellowthresold");
    // greenshades is intentionally NOT loaded from the INI.
    // The defaults (0.8 | 1.3 | 2.0) are the user's preferred values and
    // must not be overwritten by whatever the INI happens to contain.

    // Cliffs
    setDbl(cliff1,            "cliff1");
    setDbl(cliff2,            "cliff2");
    setDbl(cliffsteepfactor,  "cliffsteepfactor");
    setDbl(cliffflatplace,    "cliffflatplace");
    setDbl(cliffnosmallciffs, "cliffnosmallciffs");

    // Processing
    setInt(processes,       "processes");
    setDbl(northlinesangle, "northlinesangle");
    setInt(northlineswidth, "northlineswidth");
    setDbl(scalefactor,     "scalefactor");
    setDbl(zoffset,         "zoffset");
    if (ini.contains("output_dxf"))
        outputDxf->setChecked(ini["output_dxf"] == "1");
    if (ini.contains("savetempfiles"))
        savetempfiles->setChecked(ini["savetempfiles"] == "1");
    if (ini.contains("savetempfolders"))
        savetempfolders->setChecked(ini["savetempfolders"] == "1");

    // Optional features — only enabled if the key is present and active in the INI
    if (ini.contains("waterclass")) {
        waterclassCheck->setChecked(true);
        setInt(waterclassBox, "waterclass");
    }
    if (ini.contains("waterelevation")) {
        waterElevCheck->setChecked(true);
        setDbl(waterElevBox, "waterelevation");
    }
    if (ini.contains("buildingsclass")) {
        buildingsclassCheck->setChecked(true);
        setInt(buildingsclassBox, "buildingsclass");
    }
    if (ini.contains("detectbuildings"))
        detectBuildingsCheck->setChecked(ini["detectbuildings"] == "1");
}

// --------------------------------------------------------------------------
// Public: values
// --------------------------------------------------------------------------
QMap<QString, QString> SettingsPanel::values() const
{
    auto d = [](double v) { return QString::number(v, 'g', 6); };

    QMap<QString, QString> v;

    // Contours
    v["contour_interval"] = d(contourInterval->value());
    v["formline"]         = QString::number(formlineMode->currentIndex());
    v["smoothing"]        = d(smoothing->value());
    v["curviness"]        = d(curviness->value());
    v["knolls"]           = d(knolls->value());
    v["indexcontours"]    = d(indexContours->value());

    // Vegetation
    v["undergrowth"]    = d(undergrowth->value());
    v["undergrowth2"]   = d(undergrowth2->value());
    v["greenground"]    = d(greenground->value());
    v["greenhigh"]      = d(greenhigh->value());
    v["yellowheight"]   = d(yellowheight->value());
    v["yellowthresold"] = d(yellowthresold->value());

    QStringList shades;
    for (auto* sb : greenShadeBoxes)
        shades << d(sb->value());
    v["greenshades"] = shades.join("|");

    // Cliffs
    v["cliff1"]            = d(cliff1->value());
    v["cliff2"]            = d(cliff2->value());
    v["cliffsteepfactor"]  = d(cliffsteepfactor->value());
    v["cliffflatplace"]    = d(cliffflatplace->value());
    v["cliffnosmallciffs"] = d(cliffnosmallciffs->value());

    // Processing
    v["processes"]       = QString::number(processes->value());
    v["northlinesangle"] = d(northlinesangle->value());
    v["northlineswidth"] = QString::number(northlineswidth->value());
    v["scalefactor"]     = d(scalefactor->value());
    v["zoffset"]         = d(zoffset->value());
    v["output_dxf"]      = outputDxf->isChecked() ? "1" : "0";
    v["savetempfiles"]   = savetempfiles->isChecked() ? "1" : "0";
    v["savetempfolders"] = savetempfolders->isChecked() ? "1" : "0";

    // Optional features — only add active ones; disabled ones go via disabledOptionalKeys()
    if (waterclassCheck->isChecked())
        v["waterclass"]    = QString::number(waterclassBox->value());
    if (waterElevCheck->isChecked())
        v["waterelevation"] = d(waterElevBox->value());
    if (buildingsclassCheck->isChecked())
        v["buildingsclass"] = QString::number(buildingsclassBox->value());
    v["detectbuildings"] = detectBuildingsCheck->isChecked() ? "1" : "0";

    return v;
}

// --------------------------------------------------------------------------
// Public: disabledOptionalKeys
// --------------------------------------------------------------------------
QSet<QString> SettingsPanel::disabledOptionalKeys() const
{
    QSet<QString> keys;
    if (!waterclassCheck->isChecked())     keys.insert("waterclass");
    if (!waterElevCheck->isChecked())      keys.insert("waterelevation");
    if (!buildingsclassCheck->isChecked()) keys.insert("buildingsclass");
    return keys;
}

// --------------------------------------------------------------------------
// Private: addShadeLevel
// --------------------------------------------------------------------------
void SettingsPanel::addShadeLevel(double value)
{
    QWidget*     row    = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* sb = makeDouble(0.0, 99.0, 0.05, 2, value,
        "Vegetation density threshold for this green shade level (99 = disabled)");

    QPushButton* removeBtn = new QPushButton("×");
    removeBtn->setFixedWidth(28);
    removeBtn->setToolTip("Remove this shade level");

    layout->addWidget(sb, 1);
    layout->addWidget(removeBtn);

    greenShadeBoxes.append(sb);
    shadesLayout->addWidget(row);

    connect(removeBtn, &QPushButton::clicked, this, [this, row, sb]() {
        greenShadeBoxes.removeOne(sb);
        row->deleteLater();
    });
}

// --------------------------------------------------------------------------
// Private: clearShadeLevels
// --------------------------------------------------------------------------
void SettingsPanel::clearShadeLevels()
{
    while (shadesLayout->count() > 0)
    {
        QLayoutItem* item = shadesLayout->takeAt(0);
        if (QWidget* w = item->widget())
            delete w;
        delete item;
    }
    greenShadeBoxes.clear();
}

// --------------------------------------------------------------------------
// Private helpers
// --------------------------------------------------------------------------
QDoubleSpinBox* SettingsPanel::makeDouble(double min, double max, double step,
                                          int dec, double defaultVal,
                                          const QString& tooltip)
{
    auto* sb = new QDoubleSpinBox();
    sb->setRange(min, max);
    sb->setSingleStep(step);
    sb->setDecimals(dec);
    sb->setValue(defaultVal);
    if (!tooltip.isEmpty()) sb->setToolTip(tooltip);
    return sb;
}

QSpinBox* SettingsPanel::makeInt(int min, int max, int defaultVal,
                                 const QString& tooltip)
{
    auto* sb = new QSpinBox();
    sb->setRange(min, max);
    sb->setValue(defaultVal);
    if (!tooltip.isEmpty()) sb->setToolTip(tooltip);
    return sb;
}

// --------------------------------------------------------------------------
// Private: refreshPresetList
// --------------------------------------------------------------------------
void SettingsPanel::refreshPresetList()
{
    const QString current = presetCombo->currentIndex() > 0
                          ? presetCombo->currentText() : QString();

    presetCombo->clear();
    presetCombo->addItem("— select preset —");
    for (const QString& name : PresetManager::list())
        presetCombo->addItem(name);

    // Reselect the previously selected preset if it still exists
    if (!current.isEmpty())
    {
        const int idx = presetCombo->findText(current);
        if (idx >= 0) presetCombo->setCurrentIndex(idx);
    }
}

// --------------------------------------------------------------------------
// Private slot: onSavePreset
// --------------------------------------------------------------------------
void SettingsPanel::onSavePreset()
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, "Save Preset",
        "Preset name:",
        QLineEdit::Normal, {}, &ok).trimmed();

    if (!ok || name.isEmpty()) return;

    // Warn if overwriting an existing preset
    if (PresetManager::list().contains(name))
    {
        const auto btn = QMessageBox::question(
            this, "Overwrite Preset",
            QString("A preset named \"%1\" already exists.\nOverwrite it?").arg(name));
        if (btn != QMessageBox::Yes) return;
    }

    if (PresetManager::save(name, toJson()))
    {
        refreshPresetList();
        presetCombo->setCurrentText(name);
    }
    else
    {
        QMessageBox::warning(this, "Save Failed",
            "Could not write the preset file.\nCheck that the presets folder is writable.");
    }
}

// --------------------------------------------------------------------------
// Private slot: onLoadPreset
// --------------------------------------------------------------------------
void SettingsPanel::onLoadPreset()
{
    if (presetCombo->currentIndex() <= 0) return;   // placeholder selected

    const QString name = presetCombo->currentText();
    const QJsonObject obj = PresetManager::load(name);

    if (obj.isEmpty())
    {
        QMessageBox::warning(this, "Load Failed",
            QString("Could not read preset \"%1\".").arg(name));
        return;
    }

    fromJson(obj);
}

// --------------------------------------------------------------------------
// Private slot: onDeletePreset
// --------------------------------------------------------------------------
void SettingsPanel::onDeletePreset()
{
    if (presetCombo->currentIndex() <= 0) return;

    const QString name = presetCombo->currentText();
    const auto btn = QMessageBox::question(
        this, "Delete Preset",
        QString("Permanently delete preset \"%1\"?").arg(name));

    if (btn != QMessageBox::Yes) return;

    PresetManager::remove(name);
    refreshPresetList();
}

// --------------------------------------------------------------------------
// Public: toJson
// --------------------------------------------------------------------------
QJsonObject SettingsPanel::toJson() const
{
    auto d = [](double v) -> double { return v; };   // keep full precision in JSON
    QJsonObject obj;
    obj["version"] = 1;

    // Contours
    obj["contour_interval"] = d(contourInterval->value());
    obj["formline"]         = formlineMode->currentIndex();
    obj["smoothing"]        = d(smoothing->value());
    obj["curviness"]        = d(curviness->value());
    obj["knolls"]           = d(knolls->value());
    obj["indexcontours"]    = d(indexContours->value());

    // Vegetation
    obj["undergrowth"]    = d(undergrowth->value());
    obj["undergrowth2"]   = d(undergrowth2->value());
    obj["greenground"]    = d(greenground->value());
    obj["greenhigh"]      = d(greenhigh->value());
    obj["yellowheight"]   = d(yellowheight->value());
    obj["yellowthresold"] = d(yellowthresold->value());

    // Greenshades as a proper JSON array
    QJsonArray shadesArr;
    for (auto* sb : greenShadeBoxes)
        shadesArr.append(sb->value());
    obj["greenshades"] = shadesArr;

    // Cliffs
    obj["cliff1"]            = d(cliff1->value());
    obj["cliff2"]            = d(cliff2->value());
    obj["cliffsteepfactor"]  = d(cliffsteepfactor->value());
    obj["cliffflatplace"]    = d(cliffflatplace->value());
    obj["cliffnosmallciffs"] = d(cliffnosmallciffs->value());

    // Processing
    obj["processes"]       = processes->value();
    obj["northlinesangle"] = d(northlinesangle->value());
    obj["northlineswidth"] = northlineswidth->value();
    obj["scalefactor"]     = d(scalefactor->value());
    obj["zoffset"]         = d(zoffset->value());
    obj["output_dxf"]      = outputDxf->isChecked();
    obj["savetempfiles"]   = savetempfiles->isChecked();
    obj["savetempfolders"] = savetempfolders->isChecked();

    // Optional features — save both the enabled flag and the value so the
    // user can toggle them off without losing the configured value
    obj["waterclass_enabled"]    = waterclassCheck->isChecked();
    obj["waterclass"]            = waterclassBox->value();
    obj["waterelevation_enabled"]= waterElevCheck->isChecked();
    obj["waterelevation"]        = d(waterElevBox->value());
    obj["buildingsclass_enabled"]= buildingsclassCheck->isChecked();
    obj["buildingsclass"]        = buildingsclassBox->value();
    obj["detectbuildings"]       = detectBuildingsCheck->isChecked();

    return obj;
}

// --------------------------------------------------------------------------
// Public: fromJson
// Applies a preset JSON object to all controls.
// Unlike loadFromIni, this DOES overwrite the greenshades list because a
// preset is an explicit user save/load action.
// --------------------------------------------------------------------------
void SettingsPanel::fromJson(const QJsonObject& obj)
{
    auto dbl = [&](const QString& key, QDoubleSpinBox* sb) {
        if (obj.contains(key)) sb->setValue(obj[key].toDouble());
    };
    auto intVal = [&](const QString& key, QSpinBox* sb) {
        if (obj.contains(key)) sb->setValue(obj[key].toInt());
    };

    // Contours
    dbl("contour_interval", contourInterval);
    if (obj.contains("formline")) {
        const int v = obj["formline"].toInt();
        if (v >= 0 && v <= 2) formlineMode->setCurrentIndex(v);
    }
    dbl("smoothing",     smoothing);
    dbl("curviness",     curviness);
    dbl("knolls",        knolls);
    dbl("indexcontours", indexContours);

    // Vegetation
    dbl("undergrowth",    undergrowth);
    dbl("undergrowth2",   undergrowth2);
    dbl("greenground",    greenground);
    dbl("greenhigh",      greenhigh);
    dbl("yellowheight",   yellowheight);
    dbl("yellowthresold", yellowthresold);

    // Greenshades — explicit user action, so we replace the current list
    if (obj.contains("greenshades") && obj["greenshades"].isArray())
    {
        const QJsonArray arr = obj["greenshades"].toArray();
        if (!arr.isEmpty())
        {
            clearShadeLevels();
            for (const QJsonValue& v : arr)
                addShadeLevel(v.toDouble());
        }
    }

    // Cliffs
    dbl("cliff1",            cliff1);
    dbl("cliff2",            cliff2);
    dbl("cliffsteepfactor",  cliffsteepfactor);
    dbl("cliffflatplace",    cliffflatplace);
    dbl("cliffnosmallciffs", cliffnosmallciffs);

    // Processing
    intVal("processes",       processes);
    dbl("northlinesangle",    northlinesangle);
    intVal("northlineswidth", northlineswidth);
    dbl("scalefactor",        scalefactor);
    dbl("zoffset",            zoffset);
    if (obj.contains("output_dxf"))
        outputDxf->setChecked(obj["output_dxf"].toBool());
    if (obj.contains("savetempfiles"))
        savetempfiles->setChecked(obj["savetempfiles"].toBool());
    if (obj.contains("savetempfolders"))
        savetempfolders->setChecked(obj["savetempfolders"].toBool());

    // Optional features
    if (obj.contains("waterclass_enabled")) {
        waterclassCheck->setChecked(obj["waterclass_enabled"].toBool());
        intVal("waterclass", waterclassBox);
    }
    if (obj.contains("waterelevation_enabled")) {
        waterElevCheck->setChecked(obj["waterelevation_enabled"].toBool());
        dbl("waterelevation", waterElevBox);
    }
    if (obj.contains("buildingsclass_enabled")) {
        buildingsclassCheck->setChecked(obj["buildingsclass_enabled"].toBool());
        intVal("buildingsclass", buildingsclassBox);
    }
    if (obj.contains("detectbuildings"))
        detectBuildingsCheck->setChecked(obj["detectbuildings"].toBool());
}