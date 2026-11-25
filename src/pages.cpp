#include "pages.h"

#include "keyboardcontroller.h"
#include "lightmodes.h"
#include "theme.h"

#include <QClipboard>
#include <QDate>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>

namespace {

QLabel *makeLabel(const QString &text, const QString &objectName = {}, int pixelSize = 0,
                  const QColor &color = {})
{
    auto *label = new QLabel(text);
    if (!objectName.isEmpty())
        label->setObjectName(objectName);
    if (pixelSize > 0)
        label->setFont(Theme::uiFont(pixelSize));
    if (color.isValid())
        label->setStyleSheet(QStringLiteral("color: %1;").arg(color.name()));
    label->setTextFormat(Qt::PlainText);
    return label;
}

QLabel *makeChip(const QString &text)
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("chip"));
    label->setAlignment(Qt::AlignCenter);
    return label;
}

QFrame *makeDivider()
{
    auto *line = new QFrame;
    line->setFrameShape(QFrame::NoFrame);
    line->setFixedHeight(1);
    line->setStyleSheet(QStringLiteral("background-color: #1C2331;"));
    return line;
}

QString modeName(int mode)
{
    return LightModes::name(mode);
}

} // namespace

PageScroll::PageScroll(QWidget *parent)
    : QScrollArea(parent)
{
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto *content = new QWidget;
    content->setObjectName(QStringLiteral("pageContent"));
    content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_layout = new QVBoxLayout(content);
    m_layout->setContentsMargins(0, 0, 10, 8);
    m_layout->setSpacing(18);
    setWidget(content);
}

LightPage::LightPage(KeyboardController *controller, QWidget *parent)
    : PageScroll(parent)
    , m_controller(controller)
{
    setViewportMargins(0, 0, 0, 64);
    buildUi();
    updateColorUi();
}

QColor LightPage::selectedColor() const
{
    qreal hue = std::fmod(m_hue, 1.0);
    if (hue < 0.0)
        hue += 1.0;
    return QColor::fromHsvF(hue, m_saturation, m_value);
}

void LightPage::buildUi()
{
    auto *hero = new CardFrame;
    hero->setObjectName(QStringLiteral("hero"));
    hero->setMinimumHeight(200);

    auto *heroRow = new QHBoxLayout;
    heroRow->setSpacing(18);

    auto *left = new QVBoxLayout;
    left->setSpacing(8);
    left->setContentsMargins(0, 0, 0, 0);

    auto *chips = new QHBoxLayout;
    chips->setSpacing(8);
    m_modeChip = makeChip(QStringLiteral("MODE %1 · %2")
                              .arg(m_mode)
                              .arg(LightModes::name(m_mode)));
    m_hexChip = makeChip(QStringLiteral("#FF3B30"));
    m_brightnessChip = makeChip(QStringLiteral("亮度 5"));
    m_speedChip = makeChip(QStringLiteral("速度 3"));
    chips->addWidget(m_modeChip);
    chips->addWidget(m_hexChip);
    chips->addWidget(m_brightnessChip);
    chips->addWidget(m_speedChip);
    chips->addStretch();
    left->addLayout(chips);

    auto *title = makeLabel(QStringLiteral("让每一次敲击\n都有颜色"),
                            QStringLiteral("heroTitle"));
    title->setFont(Theme::displayFont(32, QFont::DemiBold));
    title->setMinimumHeight(84);
    left->addWidget(title);

    m_heroSubtitle = makeLabel(QStringLiteral("当前：呼吸 · #FF3B30"),
                               QStringLiteral("muted"));
    left->addWidget(m_heroSubtitle);
    left->addStretch();

    auto *leftWidget = new QWidget;
    leftWidget->setLayout(left);
    leftWidget->setMinimumWidth(360);
    leftWidget->setMaximumWidth(450);

    m_preview = new KeyboardPreview;
    m_preview->setMinimumSize(360, 150);
    m_preview->setColor(selectedColor());
    m_preview->setBrightness(m_brightness);
    m_preview->setSpeed(m_speed);

    heroRow->addWidget(leftWidget, 0);
    heroRow->addWidget(m_preview, 1);
    hero->body()->addLayout(heroRow);

    auto *modeCard = new CardFrame;
    auto *modeHeader = new QHBoxLayout;
    modeHeader->addWidget(makeLabel(QStringLiteral("灯光模式"), QStringLiteral("cardTitle")));
    modeHeader->addStretch();
    modeHeader->addWidget(makeLabel(QStringLiteral("点击选择，应用后生效"),
                                    QStringLiteral("muted")));
    modeCard->body()->addLayout(modeHeader);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);
    for (int mode = LightModes::kFirst; mode <= LightModes::kLast; ++mode) {
        auto *chip = new ModeChip(mode, LightModes::name(mode));
        chip->setAccentColor(selectedColor());
        connect(chip, &ModeChip::clicked, this, [this](int clicked) {
            setSelectedMode(clicked);
        });
        const int index = mode - LightModes::kFirst;
        grid->addWidget(chip, index / 5, index % 5);
        grid->setColumnStretch(index % 5, 1);
        m_modeChips.append(chip);
    }
    modeCard->body()->addLayout(grid);
    modeCard->body()->addWidget(makeDivider());

    auto *note = makeLabel(
        QStringLiteral("编号即固件模式号，已逐项实测校准（11 随波逐流、8 光谱循环、2 单点亮）。开关类模式对颜色不敏感。"),
        QStringLiteral("tiny"));
    note->setWordWrap(true);
    modeCard->body()->addWidget(note);
    modeCard->body()->addStretch();

    auto *controlsCard = new CardFrame;
    controlsCard->body()->addWidget(makeLabel(QStringLiteral("颜色与参数"),
                                              QStringLiteral("cardTitle")));

    m_wheel = new ColorWheel;
    m_wheel->setFixedSize(200, 200);
    auto *wheelRow = new QHBoxLayout;
    wheelRow->addStretch();
    wheelRow->addWidget(m_wheel);
    wheelRow->addStretch();
    controlsCard->body()->addLayout(wheelRow);
    connect(m_wheel, &ColorWheel::hsvChanged, this, [this](qreal hue, qreal saturation) {
        m_hue = hue;
        m_saturation = saturation;
        updateColorUi();
    });

    auto *hexRow = new QHBoxLayout;
    hexRow->setSpacing(10);
    m_swatch = new QLabel;
    m_swatch->setFixedSize(36, 36);
    hexRow->addWidget(m_swatch);
    m_hex = new QLineEdit;
    m_hex->setPlaceholderText(QStringLiteral("#RRGGBB"));
    hexRow->addWidget(m_hex, 1);
    connect(m_hex, &QLineEdit::editingFinished, this, &LightPage::applyHex);
    controlsCard->body()->addLayout(hexRow);

    m_brightnessSlider = new GlowSlider(QStringLiteral("亮度"), 0, 5, m_brightness);
    connect(m_brightnessSlider, &GlowSlider::valueChanged, this, [this](int value) {
        m_brightness = value;
        m_preview->setBrightness(value);
        updateColorUi();
    });
    controlsCard->body()->addWidget(m_brightnessSlider);

    m_speedSlider = new GlowSlider(QStringLiteral("速度"), 0, 5, m_speed);
    connect(m_speedSlider, &GlowSlider::valueChanged, this, [this](int value) {
        m_speed = value;
        m_preview->setSpeed(value);
        updateColorUi();
    });
    controlsCard->body()->addWidget(m_speedSlider);

    auto *directionRow = new QHBoxLayout;
    directionRow->setSpacing(12);
    auto *directionLabel = makeLabel(QStringLiteral("方向"), QStringLiteral("muted"));
    directionLabel->setFixedWidth(34);
    directionRow->addWidget(directionLabel);
    m_directionControl = new SegmentedControl(
        {QStringLiteral("左"), QStringLiteral("右"), QStringLiteral("上"), QStringLiteral("下")});
    connect(m_directionControl, &SegmentedControl::currentChanged, this, [this](int index) {
        m_direction = index;
    });
    directionRow->addWidget(m_directionControl, 1);
    controlsCard->body()->addLayout(directionRow);

    auto *colorfulRow = new QHBoxLayout;
    colorfulRow->setSpacing(12);
    colorfulRow->addWidget(makeLabel(QStringLiteral("多彩"), QStringLiteral("muted")), 1);
    m_colorfulSwitch = new ToggleSwitch;
    connect(m_colorfulSwitch, &ToggleSwitch::toggled, this, [this](bool checked) {
        m_colorful = checked ? 1 : 0;
    });
    colorfulRow->addWidget(m_colorfulSwitch, 0, Qt::AlignRight);
    controlsCard->body()->addLayout(colorfulRow);

    controlsCard->body()->addStretch();

    auto *hint = makeLabel(QStringLiteral("USB 与 2.4G 灯效均已实测通过。"),
                           QStringLiteral("tiny"));
    hint->setWordWrap(true);
    controlsCard->body()->addWidget(hint);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(18);
    columns->addWidget(modeCard, 3);
    columns->addWidget(controlsCard, 2);

    auto *presetCard = new CardFrame;
    auto *presetHeader = new QHBoxLayout;
    presetHeader->addWidget(makeLabel(QStringLiteral("我的专属配置"),
                                      QStringLiteral("cardTitle")));
    presetHeader->addStretch();
    auto *savePresetButton = new QPushButton(QStringLiteral("保存当前设置"), presetCard);
    savePresetButton->setObjectName(QStringLiteral("ghost"));
    savePresetButton->setCursor(Qt::PointingHandCursor);
    connect(savePresetButton, &QPushButton::clicked, this, &LightPage::saveCurrentAsPreset);
    presetHeader->addWidget(savePresetButton);
    presetCard->body()->addLayout(presetHeader);

    m_presetList = new QVBoxLayout;
    m_presetList->setSpacing(8);
    presetCard->body()->addLayout(m_presetList);

    contentLayout()->addWidget(hero);
    contentLayout()->addLayout(columns);
    contentLayout()->addWidget(presetCard);
    contentLayout()->addStretch();

    m_presets = PresetStore::load();
    refreshPresetList();

    m_applyButton = new QPushButton(QStringLiteral("应用到键盘"), this);
    m_applyButton->setObjectName(QStringLiteral("primary"));
    m_applyButton->setFixedSize(180, 48);
    m_applyButton->setCursor(Qt::PointingHandCursor);
    connect(m_applyButton, &QPushButton::clicked, this, &LightPage::apply);
    auto *glow = new QGraphicsDropShadowEffect(m_applyButton);
    glow->setBlurRadius(28);
    glow->setOffset(0, 0);
    glow->setColor(QColor(0x6E, 0x8B, 0xFF, 110));
    m_applyButton->setGraphicsEffect(glow);
    m_applyButton->raise();

    m_footerLine = new QFrame(this);
    m_footerLine->setFrameShape(QFrame::NoFrame);
    m_footerLine->setStyleSheet(QStringLiteral("background-color: #1C2331;"));
    m_footerInfo = new QLabel(this);
    m_footerInfo->setFont(Theme::monoFont(11));
    m_footerInfo->setStyleSheet(QStringLiteral("color: #5E697C;"));
    m_footerInfo->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_footerLine->raise();
    m_footerInfo->raise();

    auto updateEnabled = [this]() {
        const bool enabled = m_controller->connected() && !m_controller->busy();
        m_applyButton->setEnabled(enabled);
        m_applyButton->setText(m_controller->busy() ? QStringLiteral("正在发送...")
                                                    : QStringLiteral("应用到键盘"));
    };
    connect(m_controller, &KeyboardController::stateChanged, this, updateEnabled);
    updateEnabled();
}

void LightPage::resizeEvent(QResizeEvent *event)
{
    PageScroll::resizeEvent(event);
    if (m_applyButton) {
        if (m_footerLine)
            m_footerLine->setGeometry(0, height() - 64, width() - 12, 1);
        if (m_footerInfo)
            m_footerInfo->setGeometry(4, height() - 56, width() - 230, 48);
        m_applyButton->move(width() - m_applyButton->width() - 26,
                            height() - 56);
        if (m_footerLine)
            m_footerLine->raise();
        if (m_footerInfo)
            m_footerInfo->raise();
        m_applyButton->raise();
    }
}

void LightPage::setSelectedMode(int mode)
{
    m_mode = qBound(LightModes::kFirst, mode, LightModes::kLast);
    for (ModeChip *chip : m_modeChips)
        chip->setSelected(chip->mode() == m_mode);
    updateColorUi();
}

void LightPage::updateColorUi()
{
    const QColor color = selectedColor();

    m_preview->setColor(color);
    m_preview->setBrightness(m_brightness);
    m_preview->setSpeed(m_speed);
    m_wheel->setHsv(m_hue, m_saturation, m_value);
    m_brightnessSlider->setAccentColor(color);
    m_speedSlider->setAccentColor(color);

    for (ModeChip *chip : m_modeChips) {
        chip->setAccentColor(color);
        chip->setSelected(chip->mode() == m_mode);
    }

    m_swatch->setStyleSheet(
        QStringLiteral("background-color: %1; border: 1px solid rgba(255, 255, 255, 45); "
                       "border-radius: 10px;")
            .arg(Theme::hex(color)));

    if (!m_hex->hasFocus()) {
        const QSignalBlocker blocker(m_hex);
        m_hex->setText(Theme::hex(color));
    }

    m_modeChip->setText(QStringLiteral("MODE %1 · %2")
                            .arg(m_mode)
                            .arg(LightModes::name(m_mode)));
    m_modeChip->setStyleSheet(
        QStringLiteral("background-color: %1; border: 1px solid %2; border-radius: 12px; "
                       "padding: 4px 10px; color: %3; font-family: \"Maple Mono NF CN\"; "
                       "font-size: 11px;")
            .arg(Theme::rgba(color, 36), Theme::hex(color), Theme::hex(color)));
    m_hexChip->setText(Theme::hex(color));
    m_brightnessChip->setText(QStringLiteral("亮度 %1").arg(m_brightness));
    m_speedChip->setText(QStringLiteral("速度 %1").arg(m_speed));
    m_heroSubtitle->setText(QStringLiteral("当前：%1 · %2")
                                .arg(modeName(m_mode), Theme::hex(color)));
    if (m_footerInfo) {
        m_footerInfo->setText(
            QStringLiteral("MODE %1 %2  ·  %3  ·  亮度 %4  ·  速度 %5")
                .arg(m_mode)
                .arg(modeName(m_mode))
                .arg(Theme::hex(color))
                .arg(m_brightness)
                .arg(m_speed));
    }
}

void LightPage::applyHex()
{
    QString text = m_hex->text().trimmed();
    if (!text.startsWith(QLatin1Char('#')) && text.size() == 6)
        text.prepend(QLatin1Char('#'));

    const QColor color(text);
    if (color.isValid()) {
        m_hue = color.hsvHueF();
        if (m_hue < 0.0)
            m_hue = 0.0;
        m_saturation = color.hsvSaturationF();
        m_value = color.valueF();
    }
    m_hex->clearFocus();
    updateColorUi();
}

void LightPage::apply()
{
    const QColor color = selectedColor();
    m_controller->applyLight(m_mode, color.red(), color.green(), color.blue(),
                             m_brightness, m_speed, m_direction, m_colorful);
}

void LightPage::refreshPresetList()
{
    while (QLayoutItem *item = m_presetList->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }

    if (m_presets.isEmpty()) {
        auto *empty = makeLabel(
            QStringLiteral("还没有保存的配置。调好模式、颜色和参数后，点右上角「保存当前设置」。"),
            QStringLiteral("muted"));
        empty->setWordWrap(true);
        m_presetList->addWidget(empty);
        return;
    }

    for (const LightPreset &preset : m_presets) {
        auto *row = new QWidget;
        row->setObjectName(QStringLiteral("presetRow"));

        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(12, 8, 12, 8);
        layout->setSpacing(10);

        auto *dot = new QLabel;
        dot->setFixedSize(12, 12);
        dot->setStyleSheet(
            QStringLiteral("background-color: %1; border-radius: 6px;")
                .arg(preset.color.name()));
        layout->addWidget(dot, 0, Qt::AlignVCenter);

        auto *texts = new QVBoxLayout;
        texts->setSpacing(1);
        texts->addWidget(makeLabel(preset.name, QStringLiteral("presetTitle")));
        texts->addWidget(makeLabel(
            QStringLiteral("MODE %1 %2  ·  亮度 %3  ·  速度 %4%5")
                .arg(preset.mode)
                .arg(LightModes::name(preset.mode))
                .arg(preset.brightness)
                .arg(preset.speed)
                .arg(preset.colorful ? QStringLiteral("  ·  多彩") : QString()),
            QStringLiteral("tiny")));
        layout->addLayout(texts, 1);

        auto *applyButton = new QPushButton(QStringLiteral("应用"), row);
        applyButton->setObjectName(QStringLiteral("ghost"));
        applyButton->setCursor(Qt::PointingHandCursor);
        connect(applyButton, &QPushButton::clicked, this, [this, preset]() {
            applyPreset(preset);
        });
        layout->addWidget(applyButton, 0, Qt::AlignVCenter);

        auto *deleteButton = new QPushButton(QStringLiteral("删除"), row);
        deleteButton->setObjectName(QStringLiteral("ghost"));
        deleteButton->setCursor(Qt::PointingHandCursor);
        connect(deleteButton, &QPushButton::clicked, this, [this, name = preset.name]() {
            const bool confirmed = AppDialog::confirm(
                this, QStringLiteral("删除配置"),
                QStringLiteral("确定删除「%1」？").arg(name),
                QStringLiteral("删除"), QStringLiteral("取消"));
            if (!confirmed)
                return;
            for (int i = 0; i < m_presets.size(); ++i) {
                if (m_presets.at(i).name == name) {
                    m_presets.removeAt(i);
                    break;
                }
            }
            PresetStore::save(m_presets);
            refreshPresetList();
        });
        layout->addWidget(deleteButton, 0, Qt::AlignVCenter);

        m_presetList->addWidget(row);
    }
}

void LightPage::saveCurrentAsPreset()
{
    bool ok = false;
    const QString input = AppDialog::prompt(
        this, QStringLiteral("保存配置"), QStringLiteral("配置名称"),
        QStringLiteral("配置 %1").arg(m_presets.size() + 1), &ok);
    if (!ok)
        return;

    const QString name = input.trimmed();
    if (name.isEmpty())
        return;

    LightPreset preset;
    preset.name = name;
    preset.mode = m_mode;
    preset.color = selectedColor();
    preset.brightness = m_brightness;
    preset.speed = m_speed;
    preset.direction = m_direction;
    preset.colorful = m_colorful;

    bool replaced = false;
    for (LightPreset &existing : m_presets) {
        if (existing.name == name) {
            existing = preset;
            replaced = true;
            break;
        }
    }
    if (!replaced)
        m_presets.append(preset);

    PresetStore::save(m_presets);
    refreshPresetList();
}

void LightPage::applyPreset(const LightPreset &preset)
{
    m_mode = qBound(LightModes::kFirst, preset.mode, LightModes::kLast);

    const QColor color = preset.color;
    m_hue = color.hsvHueF();
    if (m_hue < 0.0)
        m_hue = 0.0;
    m_saturation = color.hsvSaturationF();
    m_value = color.valueF();

    m_brightness = qBound(0, preset.brightness, 5);
    m_speed = qBound(0, preset.speed, 5);
    m_direction = qBound(0, preset.direction, 3);
    m_colorful = preset.colorful ? 1 : 0;

    {
        const QSignalBlocker blockBrightness(m_brightnessSlider);
        m_brightnessSlider->setValue(m_brightness);
        const QSignalBlocker blockSpeed(m_speedSlider);
        m_speedSlider->setValue(m_speed);
        const QSignalBlocker blockDirection(m_directionControl);
        m_directionControl->setCurrentIndex(m_direction);
        const QSignalBlocker blockColorful(m_colorfulSwitch);
        m_colorfulSwitch->setChecked(m_colorful != 0, false);
    }

    updateColorUi();
    apply();
}

ClockPage::ClockPage(KeyboardController *controller, QWidget *parent)
    : PageScroll(parent)
    , m_controller(controller)
{
    buildUi();
    updateClock();

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &ClockPage::updateClock);
    m_timer->start();
}

void ClockPage::buildUi()
{
    auto *hero = new CardFrame;
    hero->setObjectName(QStringLiteral("hero"));
    hero->setMinimumHeight(230);

    m_clock = new QLabel(QStringLiteral("00:00:00"));
    m_clock->setFont(Theme::monoFont(64, QFont::DemiBold));
    m_clock->setAlignment(Qt::AlignCenter);
    m_clock->setStyleSheet(QStringLiteral("color: #F4F7FC;"));

    m_date = new QLabel;
    m_date->setFont(Theme::uiFont(14));
    m_date->setAlignment(Qt::AlignCenter);
    m_date->setStyleSheet(QStringLiteral("color: #8B95A9;"));

    auto *chips = new QHBoxLayout;
    chips->setSpacing(8);
    chips->addStretch();
    chips->addWidget(makeChip(QStringLiteral("TFT RTC")));
    m_connChip = makeChip(m_controller->connectionLabel());
    chips->addWidget(m_connChip);
    chips->addStretch();

    hero->body()->addStretch();
    hero->body()->addWidget(m_clock);
    hero->body()->addWidget(m_date);
    hero->body()->addLayout(chips);
    hero->body()->addStretch();

    auto *syncCard = new CardFrame;
    syncCard->body()->addWidget(makeLabel(QStringLiteral("同步到键盘"),
                                          QStringLiteral("cardTitle")));
    auto *description = makeLabel(
        QStringLiteral("把当前系统时间写入键盘 TFT 屏的 RTC。USB 与 2.4G 都支持，发送后键盘屏幕上的时间会立即更新。"),
        QStringLiteral("muted"));
    description->setWordWrap(true);
    syncCard->body()->addWidget(description);

    m_syncButton = new QPushButton(QStringLiteral("同步当前时间"));
    m_syncButton->setObjectName(QStringLiteral("sync"));
    m_syncButton->setMinimumHeight(46);
    m_syncButton->setCursor(Qt::PointingHandCursor);
    connect(m_syncButton, &QPushButton::clicked, m_controller, &KeyboardController::syncClock);
    syncCard->body()->addWidget(m_syncButton);

    m_status = makeLabel(QStringLiteral("提示：首次使用建议先确认 /dev/hidraw 权限。"),
                         QStringLiteral("tiny"));
    m_status->setWordWrap(true);
    syncCard->body()->addWidget(m_status);
    syncCard->body()->addStretch();

    auto *protocolCard = new CardFrame;
    protocolCard->body()->addWidget(makeLabel(QStringLiteral("协议说明"),
                                              QStringLiteral("cardTitle")));
    auto *protocol = makeLabel(
        QStringLiteral("USB：4 个 65 字节 Feature 包（04 18 → 04 28 → 时间 → 04 02）。\n"
                       "2.4G：1 个 33 字节 Output 包（0C 10 + 时间 + AA55 + 校验和）。"),
        QStringLiteral("muted"));
    protocol->setWordWrap(true);
    protocolCard->body()->addWidget(protocol);
    protocolCard->body()->addWidget(makeDivider());
    auto *verified = makeLabel(QStringLiteral("校时协议已在 USB 与 2.4G 两种模式下实测通过。"),
                               {}, 11, Theme::Success);
    verified->setWordWrap(true);
    protocolCard->body()->addWidget(verified);
    protocolCard->body()->addStretch();

    auto *columns = new QHBoxLayout;
    columns->setSpacing(18);
    columns->addWidget(syncCard, 3);
    columns->addWidget(protocolCard, 2);

    contentLayout()->addWidget(hero);
    contentLayout()->addLayout(columns);
    contentLayout()->addStretch();

    auto updateState = [this]() {
        m_connChip->setText(m_controller->connectionLabel());
        m_syncButton->setEnabled(m_controller->connected() && !m_controller->busy());
        m_syncButton->setText(m_controller->busy() ? QStringLiteral("正在同步...")
                                                   : QStringLiteral("同步当前时间"));
    };
    connect(m_controller, &KeyboardController::stateChanged, this, updateState);
    connect(m_controller, &KeyboardController::clockSynced, this,
            [this](bool ok, const QString &message) {
                m_status->setText(message);
                m_status->setStyleSheet(
                    QStringLiteral("color: %1;").arg((ok ? Theme::Success : Theme::Warning).name()));
            });
    updateState();
}

void ClockPage::updateClock()
{
    m_clock->setText(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")));

    const QDate date = QDate::currentDate();
    const QStringList weekdays = {
        QStringLiteral("周日"), QStringLiteral("周一"), QStringLiteral("周二"),
        QStringLiteral("周三"), QStringLiteral("周四"), QStringLiteral("周五"),
        QStringLiteral("周六")
    };
    const int index = date.dayOfWeek() % 7;
    m_date->setText(date.toString(QStringLiteral("yyyy年M月d日")) + QStringLiteral(" · ")
                    + weekdays.at(index));
}

SystemPage::SystemPage(KeyboardController *controller, QWidget *parent)
    : PageScroll(parent)
    , m_controller(controller)
{
    buildUi();
    loadSettings();
    refreshAvailability();
    connect(m_controller, &KeyboardController::stateChanged, this,
            &SystemPage::refreshAvailability);
}

void SystemPage::buildUi()
{
    auto *behaviourCard = new CardFrame;
    behaviourCard->body()->addWidget(makeLabel(QStringLiteral("按键行为"),
                                               QStringLiteral("cardTitle")));
    behaviourCard->body()->addWidget(makeLabel(
        QStringLiteral("响应档位越高，键盘去抖时间越短；休眠时间到点后键盘进入省电状态。"),
        QStringLiteral("tiny")));
    behaviourCard->body()->addWidget(makeDivider());

    auto *responseRow = new QHBoxLayout;
    responseRow->setSpacing(14);
    auto *responseTexts = new QVBoxLayout;
    responseTexts->setSpacing(4);
    responseTexts->addWidget(makeLabel(QStringLiteral("按键响应时间"), {}, 13, Theme::Text));
    responseTexts->addWidget(makeLabel(QStringLiteral("1 档最稳，5 档最快（约 17ms）。"),
                                       QStringLiteral("tiny")));
    responseRow->addLayout(responseTexts, 1);
    m_response = new SegmentedControl({QStringLiteral("1"), QStringLiteral("2"),
                                       QStringLiteral("3"), QStringLiteral("4"),
                                       QStringLiteral("5")});
    m_response->setFixedWidth(300);
    responseRow->addWidget(m_response, 0, Qt::AlignVCenter);
    behaviourCard->body()->addLayout(responseRow);
    behaviourCard->body()->addWidget(makeDivider());

    auto *sleepRow = new QHBoxLayout;
    sleepRow->setSpacing(14);
    auto *sleepTexts = new QVBoxLayout;
    sleepTexts->setSpacing(4);
    sleepTexts->addWidget(makeLabel(QStringLiteral("键盘休眠时间"), {}, 13, Theme::Text));
    sleepTexts->addWidget(makeLabel(QStringLiteral("无线模式下建议不要设为「不休眠」，会影响续航。"),
                                    QStringLiteral("tiny")));
    sleepRow->addLayout(sleepTexts, 1);
    m_sleep = new SegmentedControl({QStringLiteral("不休眠"), QStringLiteral("1 分钟"),
                                    QStringLiteral("5 分钟"), QStringLiteral("30 分钟")});
    m_sleep->setFixedWidth(360);
    sleepRow->addWidget(m_sleep, 0, Qt::AlignVCenter);
    behaviourCard->body()->addLayout(sleepRow);
    contentLayout()->addWidget(behaviourCard);

    auto *disableCard = new CardFrame;
    disableCard->body()->addWidget(makeLabel(QStringLiteral("禁用按键"),
                                             QStringLiteral("cardTitle")));
    disableCard->body()->addWidget(makeLabel(
        QStringLiteral("直接写进键盘固件，换电脑或换系统后依然生效。"),
        QStringLiteral("tiny")));
    disableCard->body()->addWidget(makeDivider());

    auto addSwitchRow = [&](const QString &title, const QString &subtitle,
                            ToggleSwitch **target) {
        auto *row = new QHBoxLayout;
        row->setSpacing(14);
        auto *texts = new QVBoxLayout;
        texts->setSpacing(4);
        texts->addWidget(makeLabel(title, {}, 13, Theme::Text));
        texts->addWidget(makeLabel(subtitle, QStringLiteral("tiny")));
        row->addLayout(texts, 1);
        auto *toggle = new ToggleSwitch;
        *target = toggle;
        row->addWidget(toggle, 0, Qt::AlignVCenter);
        disableCard->body()->addLayout(row);
        return row;
    };

    addSwitchRow(QStringLiteral("禁用 Windows 键"),
                 QStringLiteral("避免游戏中误触 Win 键弹出开始菜单。"), &m_win);
    disableCard->body()->addWidget(makeDivider());
    addSwitchRow(QStringLiteral("禁用 ALT+F4"),
                 QStringLiteral("阻止用 ALT+F4 关闭当前窗口。"), &m_altf4);
    disableCard->body()->addWidget(makeDivider());
    addSwitchRow(QStringLiteral("禁用 ALT+TAB"),
                 QStringLiteral("阻止用 ALT+TAB 切换窗口。"), &m_alttab);
    contentLayout()->addWidget(disableCard);

    auto *applyCard = new CardFrame;
    applyCard->body()->addWidget(makeLabel(QStringLiteral("应用到键盘"),
                                           QStringLiteral("cardTitle")));
    m_availability = makeLabel(QString(), QStringLiteral("tiny"));
    m_availability->setWordWrap(true);
    applyCard->body()->addWidget(m_availability);

    auto *applyRow = new QHBoxLayout;
    applyRow->setSpacing(14);
    auto *note = makeLabel(QStringLiteral("写入后立即生效，不需要额外保存。"),
                           QStringLiteral("tiny"));
    note->setWordWrap(true);
    applyRow->addWidget(note, 1);
    m_apply = new QPushButton(QStringLiteral("应用到键盘"));
    m_apply->setObjectName(QStringLiteral("primary"));
    m_apply->setFixedSize(180, 48);
    m_apply->setCursor(Qt::PointingHandCursor);
    connect(m_apply, &QPushButton::clicked, this, &SystemPage::apply);
    applyRow->addWidget(m_apply, 0, Qt::AlignVCenter);
    applyCard->body()->addLayout(applyRow);
    contentLayout()->addWidget(applyCard);

    auto *dangerCard = new CardFrame;
    dangerCard->body()->addWidget(makeLabel(QStringLiteral("危险操作"),
                                            QStringLiteral("cardTitle")));
    auto *dangerHint = makeLabel(
        QStringLiteral("「重置键盘」会清空键盘上的用户灯光和自定义灯光数据；"
                       "「恢复出厂」还会清掉逐键灯光，并把灯效和系统配置恢复默认。"),
        QStringLiteral("tiny"));
    dangerHint->setWordWrap(true);
    dangerCard->body()->addWidget(dangerHint);

    auto *dangerRow = new QHBoxLayout;
    dangerRow->setSpacing(10);
    dangerRow->addStretch();

    m_reset = new QPushButton(QStringLiteral("重置键盘"));
    m_reset->setObjectName(QStringLiteral("ghost"));
    m_reset->setMinimumHeight(40);
    m_reset->setCursor(Qt::PointingHandCursor);
    connect(m_reset, &QPushButton::clicked, this, [this]() { reset(false); });
    dangerRow->addWidget(m_reset);

    m_factoryReset = new QPushButton(QStringLiteral("恢复出厂"));
    m_factoryReset->setObjectName(QStringLiteral("danger"));
    m_factoryReset->setMinimumHeight(40);
    m_factoryReset->setCursor(Qt::PointingHandCursor);
    connect(m_factoryReset, &QPushButton::clicked, this, [this]() { reset(true); });
    dangerRow->addWidget(m_factoryReset);

    dangerCard->body()->addLayout(dangerRow);
    contentLayout()->addWidget(dangerCard);

    contentLayout()->addStretch();
}

void SystemPage::loadSettings()
{
    QSettings settings;
    const int response = qBound(1, settings.value(QStringLiteral("system/response"), 2).toInt(), 5);
    const int sleep = qBound(0, settings.value(QStringLiteral("system/sleep"), 2).toInt(), 3);
    m_response->setCurrentIndex(response - 1);
    m_sleep->setCurrentIndex(sleep);
    m_win->setChecked(settings.value(QStringLiteral("system/disableWin"), false).toBool(), false);
    m_altf4->setChecked(settings.value(QStringLiteral("system/disableAltF4"), false).toBool(), false);
    m_alttab->setChecked(settings.value(QStringLiteral("system/disableAltTab"), false).toBool(), false);
}

void SystemPage::saveSettings()
{
    QSettings settings;
    settings.setValue(QStringLiteral("system/response"), m_response->currentIndex() + 1);
    settings.setValue(QStringLiteral("system/sleep"), m_sleep->currentIndex());
    settings.setValue(QStringLiteral("system/disableWin"), m_win->isChecked());
    settings.setValue(QStringLiteral("system/disableAltF4"), m_altf4->isChecked());
    settings.setValue(QStringLiteral("system/disableAltTab"), m_alttab->isChecked());
}

void SystemPage::refreshAvailability()
{
    const bool connected = m_controller->connected();
    const bool wireless = m_controller->connectionType() == QStringLiteral("wireless");
    const bool canWrite = connected && !wireless && !m_controller->busy();
    m_apply->setEnabled(canWrite);
    m_reset->setEnabled(canWrite);
    m_factoryReset->setEnabled(canWrite);

    if (!connected) {
        m_availability->setText(QStringLiteral("当前未连接键盘。请插上 USB 线，或插入 2.4G 接收器。"));
        m_availability->setStyleSheet(QStringLiteral("color: #F0A23C;"));
    } else if (wireless) {
        m_availability->setText(QStringLiteral(
            "当前连接：2.4G 无线。系统配置暂时只能通过 USB 有线写入——请插上 USB 线，"
            "并确认键盘处于有线模式。灯光和校时在 2.4G 下仍然可用。"));
        m_availability->setStyleSheet(QStringLiteral("color: #F0A23C;"));
    } else {
        m_availability->setText(QStringLiteral("当前连接：USB 有线。配置会直接写入键盘固件。"));
        m_availability->setStyleSheet(QStringLiteral("color: #5FD0A0;"));
    }
}

void SystemPage::apply()
{
    const int response = m_response->currentIndex() + 1;
    const int sleep = m_sleep->currentIndex();
    const bool ok = m_controller->applySystemConfig(response, sleep,
                                                    m_win->isChecked(),
                                                    m_altf4->isChecked(),
                                                    m_alttab->isChecked());
    if (ok)
        saveSettings();
}

void SystemPage::reset(bool factory)
{
    const QString title = factory ? QStringLiteral("恢复出厂设置")
                                  : QStringLiteral("重置键盘");
    const QString text = factory
        ? QStringLiteral("会清空键盘上的所有灯光数据，并把灯效和系统配置恢复默认。确定继续？")
        : QStringLiteral("会清空键盘上的用户灯光和自定义灯光数据。确定继续？");
    const QString accept = factory ? QStringLiteral("恢复出厂")
                                   : QStringLiteral("重置");
    if (!AppDialog::confirm(this, title, text, accept, QStringLiteral("取消")))
        return;

    if (!m_controller->resetKeyboard(factory))
        return;

    if (factory) {
        m_response->setCurrentIndex(1);
        m_sleep->setCurrentIndex(2);
        m_win->setChecked(false, false);
        m_altf4->setChecked(false, false);
        m_alttab->setChecked(false, false);
        saveSettings();
    }
}

DevicePage::DevicePage(KeyboardController *controller, QWidget *parent)
    : PageScroll(parent)
    , m_controller(controller)
{
    buildUi();
    refreshUi();

    connect(m_controller, &KeyboardController::stateChanged, this, &DevicePage::refreshUi);
}

void DevicePage::buildUi()
{
    auto *headerCard = new CardFrame;
    auto *headerRow = new QHBoxLayout;
    headerRow->setSpacing(14);

    m_iconBadge = new QLabel;
    m_iconBadge->setFixedSize(44, 44);
    m_iconBadge->setAlignment(Qt::AlignCenter);
    headerRow->addWidget(m_iconBadge);

    auto *titles = new QVBoxLayout;
    titles->setSpacing(2);
    m_connTitle = makeLabel(QStringLiteral("未检测到键盘"));
    m_connTitle->setFont(Theme::uiFont(16, QFont::DemiBold));
    m_connSub = makeLabel(QString(), QStringLiteral("mono"));
    titles->addWidget(m_connTitle);
    titles->addWidget(m_connSub);
    headerRow->addLayout(titles, 1);

    m_refreshButton = new QPushButton(QStringLiteral("刷新设备"));
    m_refreshButton->setObjectName(QStringLiteral("ghost"));
    m_refreshButton->setMinimumHeight(38);
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    connect(m_refreshButton, &QPushButton::clicked, m_controller, &KeyboardController::refresh);
    headerRow->addWidget(m_refreshButton);

    headerCard->body()->addLayout(headerRow);
    headerCard->body()->addWidget(makeDivider());

    auto *modeRow = new QHBoxLayout;
    modeRow->setSpacing(14);
    auto *modeTexts = new QVBoxLayout;
    modeTexts->setSpacing(4);
    modeTexts->addWidget(makeLabel(QStringLiteral("连接方式"), {}, 13, Theme::Text));
    modeTexts->addWidget(makeLabel(QStringLiteral("自动会优先使用 USB；也可以手动指定。"),
                                   QStringLiteral("tiny")));
    modeRow->addLayout(modeTexts, 1);

    m_modeControl = new SegmentedControl(
        {QStringLiteral("自动"), QStringLiteral("USB 有线"), QStringLiteral("2.4G 无线")});
    m_modeControl->setFixedWidth(300);
    connect(m_modeControl, &SegmentedControl::currentChanged, this, [this](int index) {
        if (index == 1)
            m_controller->setPreferredMode(QStringLiteral("usb"));
        else if (index == 2)
            m_controller->setPreferredMode(QStringLiteral("wireless"));
        else
            m_controller->setPreferredMode(QStringLiteral("auto"));
    });
    modeRow->addWidget(m_modeControl);
    headerCard->body()->addLayout(modeRow);

    auto *wirelessHint = makeLabel(
        QStringLiteral("2.4G 模式请先按 FN+4 把键盘切到 2.4G，再按一下按键唤醒；蓝牙模式（FN+5/6/7）暂不支持设置。"),
        QStringLiteral("tiny"));
    wirelessHint->setWordWrap(true);
    headerCard->body()->addWidget(wirelessHint);

    contentLayout()->addWidget(headerCard);

    auto *listCard = new CardFrame;
    listCard->body()->addWidget(makeLabel(QStringLiteral("检测到的接口"),
                                          QStringLiteral("cardTitle")));
    m_listContainer = new QWidget;
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(10);
    listCard->body()->addWidget(m_listContainer);
    contentLayout()->addWidget(listCard);

    auto *permissionCard = new CardFrame;
    permissionCard->body()->addWidget(makeLabel(QStringLiteral("权限"),
                                                QStringLiteral("cardTitle")));
    auto *permissionText = makeLabel(
        QStringLiteral("普通用户需要能读写 /dev/hidraw*。项目里已经带了一份 udev 规则，安装后重新插拔键盘即可："),
        QStringLiteral("muted"));
    permissionText->setWordWrap(true);
    permissionCard->body()->addWidget(permissionText);

    m_code = new QPlainTextEdit;
    m_code->setObjectName(QStringLiteral("code"));
    m_code->setReadOnly(true);
    m_code->setFixedHeight(100);
    m_code->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_code->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_code->setPlainText(
        QStringLiteral("pkexec install -m 644 gs3108_analysis/70-gs3108t-pro.rules /etc/udev/rules.d/70-gs3108t-pro.rules\n"
                       "pkexec udevadm control --reload-rules\n"
                       "pkexec udevadm trigger --subsystem-match=hidraw --action=change"));
    permissionCard->body()->addWidget(m_code);

    auto *copyButton = new QPushButton(QStringLiteral("复制命令"));
    copyButton->setObjectName(QStringLiteral("ghost"));
    copyButton->setCursor(Qt::PointingHandCursor);
    connect(copyButton, &QPushButton::clicked, this, [this]() {
        QGuiApplication::clipboard()->setText(m_code->toPlainText());
    });
    auto *copyRow = new QHBoxLayout;
    copyRow->addWidget(copyButton, 0, Qt::AlignLeft);
    copyRow->addStretch();
    permissionCard->body()->addLayout(copyRow);

    auto *verify = makeLabel(QStringLiteral("验证：getfacl /dev/hidrawN 应能看到当前用户有 rw- 权限。"),
                             QStringLiteral("tiny"));
    verify->setWordWrap(true);
    permissionCard->body()->addWidget(verify);

    contentLayout()->addWidget(permissionCard);
    contentLayout()->addStretch();
}

void DevicePage::refreshUi()
{
    const bool connected = m_controller->connected();
    const QString type = m_controller->connectionType();

    m_iconBadge->setPixmap(Theme::tintedIcon(
        type == QStringLiteral("wireless") ? QStringLiteral(":/assets/icons/wireless.svg")
                                           : QStringLiteral(":/assets/icons/usb.svg"),
        connected ? Theme::Success : Theme::Danger, 21));
    m_iconBadge->setStyleSheet(
        QStringLiteral("background-color: %1; border: 1px solid %2; border-radius: 12px;")
            .arg(connected ? QStringLiteral("#173328") : QStringLiteral("#241B1B"),
                 connected ? QStringLiteral("#2C6A50") : QStringLiteral("#5A2B33")));

    m_connTitle->setText(connected ? m_controller->connectionLabel()
                                    : QStringLiteral("未检测到键盘"));
    m_connSub->setText(connected ? m_controller->deviceNode() + QStringLiteral("  ·  ")
                                       + m_controller->deviceName()
                                 : QStringLiteral("请检查 USB 线或 2.4G 接收器"));

    const QString preferred = m_controller->preferredMode();
    m_modeControl->setCurrentIndex(preferred == QStringLiteral("usb") ? 1
                                   : preferred == QStringLiteral("wireless") ? 2
                                                                             : 0);
    rebuildInterfaceList();
}

void DevicePage::rebuildInterfaceList()
{
    while (QLayoutItem *item = m_listLayout->takeAt(0)) {
        if (item->widget())
            delete item->widget();
        delete item;
    }

    const QVariantList devices = m_controller->devices();
    QVariantList shown;
    for (const QVariant &entry : devices) {
        const QVariantMap device = entry.toMap();
        if (device.value(QStringLiteral("type")).toString() != QStringLiteral("未知接口"))
            shown.append(entry);
    }
    if (shown.isEmpty())
        shown = devices;

    if (shown.isEmpty()) {
        m_listLayout->addWidget(makeLabel(QStringLiteral("没有匹配到 05ac:024f 的 hidraw 接口。"),
                                          QStringLiteral("muted")));
        m_listLayout->addStretch();
        return;
    }

    for (const QVariant &entry : shown) {
        const QVariantMap device = entry.toMap();
        const bool active = device.value(QStringLiteral("active")).toBool();

        auto *row = new QFrame;
        row->setObjectName(active ? QStringLiteral("deviceRowActive")
                                  : QStringLiteral("deviceRow"));
        row->setAttribute(Qt::WA_StyledBackground, true);

        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(14, 12, 14, 12);
        layout->setSpacing(12);

        auto *dot = new QLabel;
        dot->setFixedSize(8, 8);
        dot->setStyleSheet(
            QStringLiteral("background-color: %1; border-radius: 4px;")
                .arg(active ? Theme::Success.name() : Theme::TextMuted.name()));
        layout->addWidget(dot, 0, Qt::AlignVCenter);

        auto *texts = new QVBoxLayout;
        texts->setSpacing(2);
        auto *title = makeLabel(device.value(QStringLiteral("type")).toString()
                                    + QStringLiteral("  ·  ")
                                    + device.value(QStringLiteral("node")).toString());
        title->setFont(Theme::uiFont(12, QFont::DemiBold));
        texts->addWidget(title);
        texts->addWidget(makeLabel(
            QStringLiteral("usage ") + device.value(QStringLiteral("usagePage")).toString()
                + QStringLiteral("   feature ")
                + device.value(QStringLiteral("feature")).toString()
                + QStringLiteral("   output ")
                + device.value(QStringLiteral("output")).toString(),
            QStringLiteral("mono")));
        layout->addLayout(texts, 1);

        if (active) {
            auto *badge = new QLabel(QStringLiteral("当前"));
            badge->setAlignment(Qt::AlignCenter);
            badge->setFixedHeight(24);
            badge->setStyleSheet(
                QStringLiteral("background-color: #173328; border: 1px solid #2C6A50; "
                               "border-radius: 12px; color: #3DDC97; padding: 0 10px; "
                               "font-size: 10px;"));
            layout->addWidget(badge, 0, Qt::AlignVCenter);
        }

        m_listLayout->addWidget(row);
    }
    m_listLayout->addStretch();
}

AboutPage::AboutPage(QWidget *parent)
    : PageScroll(parent)
{
    buildUi();
}

void AboutPage::buildUi()
{
    auto *hero = new CardFrame;
    hero->setObjectName(QStringLiteral("hero"));
    hero->setMinimumHeight(190);

    auto *row = new QHBoxLayout;
    row->setSpacing(18);

    auto *logo = new QLabel;
    logo->setFixedSize(78, 78);
    logo->setPixmap(Theme::logoPixmap(78));
    row->addWidget(logo, 0, Qt::AlignVCenter);

    auto *texts = new QVBoxLayout;
    texts->setSpacing(4);
    auto *title = makeLabel(QStringLiteral("GS3108T PRO 控制中心"));
    title->setFont(Theme::displayFont(26, QFont::DemiBold));
    texts->addWidget(title);
    texts->addWidget(makeLabel(
        QStringLiteral("Linux 下的 GANSS GS3108T-PRO 配置工具 · Qt 6 Widgets / C++26"),
        QStringLiteral("muted")));
    texts->addWidget(makeLabel(
        QStringLiteral("v0.1.0 · 协议来自官方 Windows 驱动的静态逆向"),
        QStringLiteral("tiny")));
    texts->addStretch();
    row->addLayout(texts, 1);

    hero->body()->addStretch();
    hero->body()->addLayout(row);
    hero->body()->addStretch();
    contentLayout()->addWidget(hero);

    auto *features = new CardFrame;
    features->body()->addWidget(makeLabel(QStringLiteral("功能进度"),
                                          QStringLiteral("cardTitle")));

    struct Feature {
        QString name;
        QString state;
        bool ok;
    };
    const QVector<Feature> items = {
        {QStringLiteral("灯光模式（模式 / 颜色 / 亮度 / 速度 / 方向 / 多彩）"),
         QStringLiteral("已支持"), true},
        {QStringLiteral("我的专属配置（本地预设，一键应用）"), QStringLiteral("已支持"), true},
        {QStringLiteral("TFT 时钟同步"), QStringLiteral("已支持"), true},
        {QStringLiteral("系统配置（响应 / 休眠 / 禁用按键）"), QStringLiteral("USB 已验证"), true},
        {QStringLiteral("重置键盘 / 恢复出厂"), QStringLiteral("USB 已验证"), true},
        {QStringLiteral("USB 有线 / 2.4G 无线自动识别"), QStringLiteral("已支持"), true},
        {QStringLiteral("2.4G 灯效写入"), QStringLiteral("实测通过"), true},
        {QStringLiteral("用户灯光 / 自定义灯光（逐键 RGB）"), QStringLiteral("逆向中"), false},
        {QStringLiteral("TFT GIF"), QStringLiteral("待开始"), false}
    };

    for (const Feature &feature : items) {
        auto *line = new QFrame;
        line->setObjectName(QStringLiteral("deviceRow"));
        line->setAttribute(Qt::WA_StyledBackground, true);
        line->setFixedHeight(44);

        auto *layout = new QHBoxLayout(line);
        layout->setContentsMargins(12, 0, 12, 0);
        layout->setSpacing(10);

        auto *dot = new QLabel;
        dot->setFixedSize(7, 7);
        dot->setStyleSheet(
            QStringLiteral("background-color: %1; border-radius: 3px;")
                .arg(feature.ok ? Theme::Success.name() : Theme::Warning.name()));
        layout->addWidget(dot);
        layout->addWidget(makeLabel(feature.name), 1);
        auto *state = makeLabel(feature.state, {}, 11,
                                feature.ok ? Theme::Success : Theme::Warning);
        layout->addWidget(state);

        features->body()->addWidget(line);
    }
    contentLayout()->addWidget(features);

    auto *protocol = new CardFrame;
    protocol->body()->addWidget(makeLabel(QStringLiteral("协议摘要"),
                                          QStringLiteral("cardTitle")));
    auto *protocolText = makeLabel(
        QStringLiteral("USB：usage page 0xFF13，64 字节 Feature。灯效为 5 个 65 字节包（04 18 → 04 13 → 数据 → 04 02 → 04 F0）。\n"
                       "2.4G：usage page 0xFF60，32 字节 Output。灯效为 1 个 33 字节包（05 10 + 数据 + AA55 + 校验和）。"),
        QStringLiteral("muted"));
    protocolText->setWordWrap(true);
    protocol->body()->addWidget(protocolText);
    protocol->body()->addWidget(makeDivider());
    auto *files = makeLabel(
        QStringLiteral("相关文件：GS3108T-PRO-时间同步-协议与实现.md、HID设备驱动逆向方法论-交接文档.md、gs3108_analysis/gskbd-time.py、gs3108_analysis/gskbd-light.py"),
        QStringLiteral("tiny"));
    files->setWordWrap(true);
    protocol->body()->addWidget(files);
    contentLayout()->addWidget(protocol);

    contentLayout()->addStretch();
}
