#pragma once

#include <QWidget>
#include <QMap>
#include <array>

class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QCheckBox;
class QGroupBox;

// --------------------------------------------------------------------------
// SettingsPanel
//
// Exposes the most important pullauta.ini parameters through a scrollable
// form UI. Call loadFromIni() at startup to populate from the current file,
// and values() before each run to get the map to pass to writeIniValues().
//
// batch / lazfolder / batchoutfolder are NOT managed here — MainWindow owns
// those and writes them directly.
// --------------------------------------------------------------------------
class SettingsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPanel(QWidget* parent = nullptr);

    // Reads pullauta.ini and populates all controls.
    // Missing keys are left at their defaults; non-existent file is a no-op.
    void loadFromIni(const QString& iniPath);

    // Returns all parameter values as a string map for writeIniValues().
    QMap<QString, QString> values() const;

private:
    QDoubleSpinBox* makeDouble(double min, double max, double step,
                               int decimals, double defaultVal,
                               const QString& tooltip = {});
    QSpinBox*       makeInt(int min, int max, int defaultVal,
                            const QString& tooltip = {});

    // ---- Contours -------------------------------------------------------
    QDoubleSpinBox* contourInterval;
    QComboBox*      formlineMode;
    QDoubleSpinBox* smoothing;
    QDoubleSpinBox* curviness;
    QDoubleSpinBox* knolls;
    QDoubleSpinBox* indexContours;

    // ---- Vegetation -----------------------------------------------------
    QDoubleSpinBox* undergrowth;
    QDoubleSpinBox* undergrowth2;
    QDoubleSpinBox* greenground;
    QDoubleSpinBox* greenhigh;
    QDoubleSpinBox* yellowheight;
    QDoubleSpinBox* yellowthresold;   // note: typo is in the INI key itself
    std::array<QDoubleSpinBox*, 11> greenShadeBoxes;

    // ---- Cliffs ---------------------------------------------------------
    QDoubleSpinBox* cliff1;
    QDoubleSpinBox* cliff2;
    QDoubleSpinBox* cliffsteepfactor;
    QDoubleSpinBox* cliffflatplace;
    QDoubleSpinBox* cliffnosmallciffs;

    // ---- Processing -----------------------------------------------------
    QSpinBox*       processes;
    QDoubleSpinBox* northlinesangle;
    QSpinBox*       northlineswidth;
    QDoubleSpinBox* scalefactor;
    QDoubleSpinBox* zoffset;
    QCheckBox*      outputDxf;
};
