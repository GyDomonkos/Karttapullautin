#pragma once

#include <QWidget>
#include <QStringList>

class QSpinBox;
class QDoubleSpinBox;

class ToolsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ToolsPanel(QWidget* parent = nullptr);

signals:
    // Emitted whenever any button is clicked, sending the matching CLI arguments
    void toolActionRequested(const QStringList& arguments);

private:
    QSpinBox* mergeScaleBox;
    QDoubleSpinBox* cliffSmoothBox;
    QDoubleSpinBox* cliffSteepBox;
};