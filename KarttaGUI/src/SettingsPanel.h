#pragma once

#include <QWidget>
#include <QMap>
#include <QSet>
#include <QVector>

class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QCheckBox;
class QVBoxLayout;

class SettingsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPanel(QWidget* parent = nullptr);

    void loadFromIni(const QString& iniPath);
    void resetToDefaults();                    // restores all controls to hardcoded defaults
    QMap<QString, QString> values() const;

    // Keys that should be commented out in the INI (optional features disabled by user)
    QSet<QString> disabledOptionalKeys() const;

private:
    void addShadeLevel(double value);     // appends a row to the shade list
    void clearShadeLevels();              // removes all rows + clears greenShadeBoxes

    QDoubleSpinBox* makeDouble(double min, double max, double step,
                               int dec, double defaultVal,
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
    QDoubleSpinBox* yellowthresold;   // typo matches the INI key

    QVBoxLayout*             shadesLayout;   // holds the dynamic level rows
    QVector<QDoubleSpinBox*> greenShadeBoxes;

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

    // ---- Optional features (enable = uncomment in INI) ------------------
    QCheckBox*      waterclassCheck;
    QSpinBox*       waterclassBox;
    QCheckBox*      waterElevCheck;
    QDoubleSpinBox* waterElevBox;
    QCheckBox*      buildingsclassCheck;
    QSpinBox*       buildingsclassBox;
    QCheckBox*      detectBuildingsCheck;
};