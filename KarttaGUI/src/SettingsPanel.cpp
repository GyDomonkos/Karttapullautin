#include "SettingsPanel.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QFont>
#include <QFile>
#include <QTextStream>
#include <QFrame>


SettingsPanel::SettingsPanel(QWidget* parent)
    : QWidget(parent)
{
    // Outer scroll area so all groups fit even on small screens
    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget*     content = new QWidget();
    QVBoxLayout* vbox    = new QVBoxLayout(content);
    vbox->setSpacing(10);
    vbox->setContentsMargins(6, 6, 6, 6);

    // ==========================================================================
    // GROUP: Contours
    // ==========================================================================
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
            "Larger values smooth contours more. Default 1.0; range 0.5–3.0");
        form->addRow("Smoothing:", smoothing);

        curviness = makeDouble(0.5, 2.0, 0.1, 1, 1.0,
            "Larger values exaggerate reentrants and spurs. Default 1.0");
        form->addRow("Curviness:", curviness);

        knolls = makeDouble(0.0, 1.0, 0.05, 2, 0.8,
            "Higher values produce fewer but more distinct knolls. Default 0.8");
        form->addRow("Knoll sensitivity:", knolls);

        indexContours = makeDouble(0.0, 50.0, 0.5, 1, 12.5,
            "Interval for index (thick) contour lines. 0 = none");
        form->addRow("Index contour interval (m):", indexContours);

        vbox->addWidget(group);
    }

    // ==========================================================================
    // GROUP: Vegetation
    // ==========================================================================
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
            "Height threshold above which vegetation is classified as high (m)");
        form->addRow("Green high (m):", greenhigh);

        yellowheight = makeDouble(0.0, 5.0, 0.1, 1, 0.9,
            "Height threshold for rough open (yellow) ground cover");
        form->addRow("Yellow height (m):", yellowheight);

        yellowthresold = makeDouble(0.0, 5.0, 0.05, 2, 0.9,
            "Density threshold for yellow (rough open) ground classification");
        form->addRow("Yellow threshold:", yellowthresold);

        // ---- Green Shades sub-section ------------------------------------
        QLabel* shadesHeader = new QLabel("Green shades — vegetation density thresholds:");
        {
            QFont f = shadesHeader->font();
            f.setBold(true);
            shadesHeader->setFont(f);
        }
        shadesHeader->setToolTip(
            "11 density thresholds mapping LiDAR point density to map green shades.\n"
            "Values increase from open ground (low) to impassable forest (high).\n"
            "Use 99 to disable a shade level. Typical: 0.2|0.35|0.5|0.7|1.3|2.6|4|99|99|99|99"
        );
        gLayout->addWidget(shadesHeader);

        // Two-column layout: Levels 1–6 on the left, 7–11 on the right
        const double shadeDefaults[11] = {
            0.2, 0.35, 0.5, 0.7, 1.3, 2.6, 4.0, 99.0, 99.0, 99.0, 99.0
        };

        QHBoxLayout* shadesRow  = new QHBoxLayout();
        QFormLayout* leftShades = new QFormLayout();
        QFormLayout* rightShades= new QFormLayout();

        for (int i = 0; i < 11; ++i)
        {
            greenShadeBoxes[i] = makeDouble(0.0, 99.0, 0.05, 2, shadeDefaults[i],
                QString("Density threshold for green shade level %1 (99 = disabled)").arg(i + 1));
            QFormLayout* target = (i < 6) ? leftShades : rightShades;
            target->addRow(QString("Level %1:").arg(i + 1), greenShadeBoxes[i]);
        }

        shadesRow->addLayout(leftShades);
        shadesRow->addLayout(rightShades);
        gLayout->addLayout(shadesRow);

        vbox->addWidget(group);
    }

    // ==========================================================================
    // GROUP: Cliffs
    // ==========================================================================
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
            "Factor weighting how steeply a slope must rise to count as a cliff");
        form->addRow("Steep factor:", cliffsteepfactor);

        cliffflatplace = makeDouble(0.5, 10.0, 0.5, 1, 3.5,
            "Flat-area radius threshold for cliff suppression");
        form->addRow("Flat place threshold:", cliffflatplace);

        cliffnosmallciffs = makeDouble(0.0, 20.0, 0.5, 1, 5.5,
            "Minimum cliff length in metres; shorter features are removed");
        form->addRow("Min cliff length (m):", cliffnosmallciffs);

        vbox->addWidget(group);
    }

    // ==========================================================================
    // GROUP: Processing
    // ==========================================================================
    {
        QGroupBox*   group = new QGroupBox("Processing");
        QFormLayout* form  = new QFormLayout(group);

        processes = makeInt(1, 32, 2,
            "Number of tiles processed simultaneously");
        form->addRow("Parallel processes:", processes);

        northlinesangle = makeDouble(-360.0, 360.0, 1.0, 1, 0.0,
            "Angle in degrees for north lines overlay. 0 = no north lines");
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
        outputDxf->setToolTip("Also write raw DXF contours alongside the PNG map");
        form->addRow("DXF output:", outputDxf);

        vbox->addWidget(group);
    }

    vbox->addStretch();

    scroll->setWidget(content);

    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scroll);
}

// --------------------------------------------------------------------------
// Public: loadFromIni
// --------------------------------------------------------------------------
void SettingsPanel::loadFromIni(const QString& iniPath)
{
    QFile file(iniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // Parse all key=value pairs, skipping comments and section headers
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

    // Lambdas for safe assignment
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
    setDbl(smoothing,      "smoothing");
    setDbl(curviness,      "curviness");
    setDbl(knolls,         "knolls");
    setDbl(indexContours,  "indexcontours");

    // Vegetation
    setDbl(undergrowth,    "undergrowth");
    setDbl(undergrowth2,   "undergrowth2");
    setDbl(greenground,    "greenground");
    setDbl(greenhigh,      "greenhigh");
    setDbl(yellowheight,   "yellowheight");
    setDbl(yellowthresold, "yellowthresold");

    if (ini.contains("greenshades"))
    {
        const QStringList parts = ini["greenshades"].split("|");
        for (int i = 0; i < (int)greenShadeBoxes.size() && i < parts.size(); ++i)
        {
            bool ok; double v = parts[i].toDouble(&ok);
            if (ok) greenShadeBoxes[i]->setValue(v);
        }
    }

    // Cliffs
    setDbl(cliff1,           "cliff1");
    setDbl(cliff2,           "cliff2");
    setDbl(cliffsteepfactor, "cliffsteepfactor");
    setDbl(cliffflatplace,   "cliffflatplace");
    setDbl(cliffnosmallciffs,"cliffnosmallciffs");

    // Processing
    setInt(processes,       "processes");
    setDbl(northlinesangle, "northlinesangle");
    setInt(northlineswidth, "northlineswidth");
    setDbl(scalefactor,     "scalefactor");
    setDbl(zoffset,         "zoffset");
    if (ini.contains("output_dxf"))
        outputDxf->setChecked(ini["output_dxf"] == "1");
}

// --------------------------------------------------------------------------
// Public: values
// --------------------------------------------------------------------------
QMap<QString, QString> SettingsPanel::values() const
{
    // 'g' format gives compact representation: 0.35 not 0.350000, 5 not 5.0
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
    v["undergrowth"]      = d(undergrowth->value());
    v["undergrowth2"]     = d(undergrowth2->value());
    v["greenground"]      = d(greenground->value());
    v["greenhigh"]        = d(greenhigh->value());
    v["yellowheight"]     = d(yellowheight->value());
    v["yellowthresold"]   = d(yellowthresold->value());

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
    v["processes"]        = QString::number(processes->value());
    v["northlinesangle"]  = d(northlinesangle->value());
    v["northlineswidth"]  = QString::number(northlineswidth->value());
    v["scalefactor"]      = d(scalefactor->value());
    v["zoffset"]          = d(zoffset->value());
    v["output_dxf"]       = outputDxf->isChecked() ? "1" : "0";

    return v;
}

// --------------------------------------------------------------------------
// Private helpers
// --------------------------------------------------------------------------
QDoubleSpinBox* SettingsPanel::makeDouble(double min, double max, double step,
                                          int decimals, double defaultVal,
                                          const QString& tooltip)
{
    auto* sb = new QDoubleSpinBox();
    sb->setRange(min, max);
    sb->setSingleStep(step);
    sb->setDecimals(decimals);
    sb->setValue(defaultVal);
    if (!tooltip.isEmpty())
        sb->setToolTip(tooltip);
    return sb;
}

QSpinBox* SettingsPanel::makeInt(int min, int max, int defaultVal,
                                 const QString& tooltip)
{
    auto* sb = new QSpinBox();
    sb->setRange(min, max);
    sb->setValue(defaultVal);
    if (!tooltip.isEmpty())
        sb->setToolTip(tooltip);
    return sb;
}
