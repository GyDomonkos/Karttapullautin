#pragma once

#include <QWidget>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QJsonObject>

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
    void resetToDefaults();

    // Preset serialisation — used by the preset bar and by MainWindow
    QJsonObject toJson() const;
    void        fromJson(const QJsonObject& obj);

    QMap<QString, QString> values() const;
    QSet<QString>          disabledOptionalKeys() const;

private slots:
    void onSavePreset();
    void onLoadPreset();
    void onDeletePreset();

private:
    void refreshPresetList();
    void addShadeLevel(double value);
    void clearShadeLevels();

    QDoubleSpinBox* makeDouble(double min, double max, double step,
                               int dec, double defaultVal,
                               const QString& tooltip = {});
    QSpinBox*       makeInt(int min, int max, int defaultVal,
                            const QString& tooltip = {});

    // ---- Preset bar -----------------------------------------------------
    QComboBox* presetCombo;

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
    QDoubleSpinBox* yellowthresold;

    QVBoxLayout*             shadesLayout;
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
    QCheckBox*      savetempfiles;
    QCheckBox*      savetempfolders;

    // ---- Optional features ----------------------------------------------
    QCheckBox*      waterclassCheck;
    QSpinBox*       waterclassBox;
    QCheckBox*      waterElevCheck;
    QDoubleSpinBox* waterElevBox;
    QCheckBox*      buildingsclassCheck;
    QSpinBox*       buildingsclassBox;
    QCheckBox*      detectBuildingsCheck;
};