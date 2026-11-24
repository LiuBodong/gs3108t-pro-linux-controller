#pragma once

#include <QScrollArea>
#include <QWidget>
#include <QVector>

#include "presetstore.h"
#include "widgets.h"

class KeyboardController;
class QFrame;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSegmentedControl;
class QTimer;
class QVBoxLayout;

class PageScroll : public QScrollArea {
public:
    explicit PageScroll(QWidget *parent = nullptr);
    QVBoxLayout *contentLayout() const { return m_layout; }

private:
    QVBoxLayout *m_layout = nullptr;
};

class LightPage : public PageScroll {
public:
    explicit LightPage(KeyboardController *controller, QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildUi();
    QColor selectedColor() const;
    void setSelectedMode(int mode);
    void updateColorUi();
    void applyHex();
    void apply();
    void refreshPresetList();
    void saveCurrentAsPreset();
    void applyPreset(const LightPreset &preset);

    KeyboardController *m_controller = nullptr;
    int m_mode = 8;
    qreal m_hue = 0.0;
    qreal m_saturation = 0.92;
    qreal m_value = 1.0;
    int m_brightness = 5;
    int m_speed = 3;
    int m_direction = 0;
    int m_colorful = 0;
    QVector<LightPreset> m_presets;

    QVector<ModeChip *> m_modeChips;
    ColorWheel *m_wheel = nullptr;
    QLabel *m_swatch = nullptr;
    QLineEdit *m_hex = nullptr;
    GlowSlider *m_brightnessSlider = nullptr;
    GlowSlider *m_speedSlider = nullptr;
    SegmentedControl *m_directionControl = nullptr;
    ToggleSwitch *m_colorfulSwitch = nullptr;
    KeyboardPreview *m_preview = nullptr;
    QLabel *m_modeChip = nullptr;
    QLabel *m_hexChip = nullptr;
    QLabel *m_brightnessChip = nullptr;
    QLabel *m_speedChip = nullptr;
    QLabel *m_heroSubtitle = nullptr;
    QPushButton *m_applyButton = nullptr;
    QFrame *m_footerLine = nullptr;
    QLabel *m_footerInfo = nullptr;
    QVBoxLayout *m_presetList = nullptr;
};

class ClockPage : public PageScroll {
public:
    explicit ClockPage(KeyboardController *controller, QWidget *parent = nullptr);

private:
    void buildUi();
    void updateClock();

    KeyboardController *m_controller = nullptr;
    QLabel *m_clock = nullptr;
    QLabel *m_date = nullptr;
    QLabel *m_connChip = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_syncButton = nullptr;
    QTimer *m_timer = nullptr;
};

class DevicePage : public PageScroll {
public:
    explicit DevicePage(KeyboardController *controller, QWidget *parent = nullptr);

private:
    void buildUi();
    void refreshUi();
    void rebuildInterfaceList();

    KeyboardController *m_controller = nullptr;
    QLabel *m_iconBadge = nullptr;
    QLabel *m_connTitle = nullptr;
    QLabel *m_connSub = nullptr;
    SegmentedControl *m_modeControl = nullptr;
    QWidget *m_listContainer = nullptr;
    QVBoxLayout *m_listLayout = nullptr;
    QPlainTextEdit *m_code = nullptr;
    QPushButton *m_refreshButton = nullptr;
};

class AboutPage : public PageScroll {
public:
    explicit AboutPage(QWidget *parent = nullptr);

private:
    void buildUi();
};
