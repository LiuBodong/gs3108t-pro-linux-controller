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
class QMovie;
class QPlainTextEdit;
class QProgressBar;
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
    QPushButton *m_applyButton = nullptr;
    QVBoxLayout *m_presetList = nullptr;
};

class UserLightPage : public PageScroll {
public:
    explicit UserLightPage(KeyboardController *controller, QWidget *parent = nullptr);

private:
    void buildUi();
    void updateColorUi();
    void applyHex();
    void switchSlot(int slot);
    void refreshSelection();
    void applyToKeyboard();
    void clearBoard();
    void loadSlots();
    void saveSlots();
    void rebuildSlotTabs();
    void addSlot();
    void removeSlot();

    KeyboardController *m_controller = nullptr;
    int m_slot = 0;
    QColor m_color{0xFF, 0x3B, 0x30};
    QVector<QVector<QColor>> m_slots;
    QStringList m_slotNames;
    PerKeyBoard *m_board = nullptr;
    QHBoxLayout *m_slotTabs = nullptr;
    ColorWheel *m_wheel = nullptr;
    QLabel *m_swatch = nullptr;
    QLineEdit *m_hex = nullptr;
    QLabel *m_selection = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_apply = nullptr;
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

class GifPage : public PageScroll {
public:
    explicit GifPage(KeyboardController *controller, QWidget *parent = nullptr);

private:
    void buildUi();
    void chooseFile();
    void loadPreview();
    void upload();
    void refreshAvailability();

    KeyboardController *m_controller = nullptr;
    QLabel *m_preview = nullptr;
    QLabel *m_fileLabel = nullptr;
    QLabel *m_infoLabel = nullptr;
    QLabel *m_availability = nullptr;
    QPushButton *m_choose = nullptr;
    QPushButton *m_upload = nullptr;
    QProgressBar *m_progress = nullptr;
    QMovie *m_movie = nullptr;
    QString m_path;
};

class SystemPage : public PageScroll {
public:
    explicit SystemPage(KeyboardController *controller, QWidget *parent = nullptr);

private:
    void buildUi();
    void loadSettings();
    void saveSettings();
    void refreshAvailability();
    void apply();
    void reset(bool factory);

    KeyboardController *m_controller = nullptr;
    SegmentedControl *m_response = nullptr;
    SegmentedControl *m_sleep = nullptr;
    ToggleSwitch *m_win = nullptr;
    ToggleSwitch *m_altf4 = nullptr;
    ToggleSwitch *m_alttab = nullptr;
    QPushButton *m_apply = nullptr;
    QPushButton *m_reset = nullptr;
    QPushButton *m_factoryReset = nullptr;
    QLabel *m_availability = nullptr;
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
