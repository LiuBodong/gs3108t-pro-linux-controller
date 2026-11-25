#include "pages.h"

#include "keyboardcontroller.h"
#include "lightmodes.h"
#include "theme.h"

#include <QClipboard>
#include <QDate>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QMovie>
#include <QProgressBar>
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

UserLightPage::UserLightPage(KeyboardController *controller, QWidget *parent)
    : PageScroll(parent)
    , m_controller(controller)
{
    buildUi();
    loadSlots();
    m_board->setColors(m_slots[m_slot]);
}

void UserLightPage::buildUi()
{
    auto *hero = new CardFrame;
    hero->setObjectName(QStringLiteral("hero"));
    hero->setMinimumHeight(180);

    auto *heroRow = new QHBoxLayout;
    heroRow->setSpacing(18);

    auto *texts = new QVBoxLayout;
    texts->setSpacing(8);
    auto *chips = new QHBoxLayout;
    chips->setSpacing(8);
    chips->addWidget(makeChip(QStringLiteral("PER-KEY RGB")));
    chips->addWidget(makeChip(QStringLiteral("112 键")));
    chips->addStretch();
    texts->addLayout(chips);

    auto *title = makeLabel(QStringLiteral("给每一颗键\n单独上色"), QStringLiteral("heroTitle"));
    title->setFont(Theme::displayFont(32, QFont::DemiBold));
    title->setMinimumHeight(84);
    texts->addWidget(title);

    auto *subtitle = makeLabel(
        QStringLiteral("点击或框选键盘上的按键，选好颜色后写入键盘。"
                       "写入后键盘进入实时灯光模式；按 FN+(-) 或 FN+(+) 会回到键盘自己的灯效，"
                       "再点一次「写入键盘」即可恢复。"),
        QStringLiteral("muted"));
    subtitle->setWordWrap(true);
    texts->addWidget(subtitle);
    texts->addStretch();

    auto *left = new QWidget;
    left->setLayout(texts);
    left->setMinimumWidth(360);
    left->setMaximumWidth(560);

    auto *art = new QLabel;
    art->setAlignment(Qt::AlignCenter);
    art->setPixmap(Theme::tintedIcon(QStringLiteral(":/assets/icons/keyboard.svg"), m_color, 110));
    art->setMinimumWidth(140);

    heroRow->addWidget(left, 1);
    heroRow->addWidget(art, 0);
    hero->body()->addLayout(heroRow);
    contentLayout()->addWidget(hero);

    auto *boardCard = new CardFrame;
    auto *boardHeader = new QHBoxLayout;
    boardHeader->setSpacing(10);
    boardHeader->addWidget(makeLabel(QStringLiteral("逐键颜色"), QStringLiteral("cardTitle")));
    boardHeader->addSpacing(6);
    boardHeader->addStretch();

    auto *selectAllButton = new QPushButton(QStringLiteral("全选"), this);
    selectAllButton->setObjectName(QStringLiteral("ghost"));
    selectAllButton->setCursor(Qt::PointingHandCursor);
    connect(selectAllButton, &QPushButton::clicked, this, [this]() {
        m_board->selectAll();
    });
    boardHeader->addWidget(selectAllButton);

    auto *clearSelectionButton = new QPushButton(QStringLiteral("清空选择"), this);
    clearSelectionButton->setObjectName(QStringLiteral("ghost"));
    clearSelectionButton->setCursor(Qt::PointingHandCursor);
    connect(clearSelectionButton, &QPushButton::clicked, this, [this]() {
        m_board->clearSelection();
    });
    boardHeader->addWidget(clearSelectionButton);
    boardCard->body()->addLayout(boardHeader);

    auto *slotRow = new QHBoxLayout;
    slotRow->setSpacing(6);
    m_slotTabs = new QHBoxLayout;
    m_slotTabs->setSpacing(6);
    slotRow->addLayout(m_slotTabs);
    slotRow->addStretch();

    auto *addSlotButton = new QPushButton(QStringLiteral("+ 新建方案"), this);
    addSlotButton->setObjectName(QStringLiteral("ghost"));
    addSlotButton->setCursor(Qt::PointingHandCursor);
    connect(addSlotButton, &QPushButton::clicked, this, &UserLightPage::addSlot);
    slotRow->addWidget(addSlotButton);

    auto *removeSlotButton = new QPushButton(QStringLiteral("删除方案"), this);
    removeSlotButton->setObjectName(QStringLiteral("ghost"));
    removeSlotButton->setCursor(Qt::PointingHandCursor);
    connect(removeSlotButton, &QPushButton::clicked, this, &UserLightPage::removeSlot);
    slotRow->addWidget(removeSlotButton);
    boardCard->body()->addLayout(slotRow);

    m_board = new PerKeyBoard;
    connect(m_board, &PerKeyBoard::selectionChanged,
            this, &UserLightPage::refreshSelection);
    boardCard->body()->addWidget(m_board);

    auto *boardFooter = new QHBoxLayout;
    m_selection = makeLabel(QStringLiteral("未选择按键"), QStringLiteral("muted"));
    boardFooter->addWidget(m_selection, 1);
    auto *hint = makeLabel(
        QStringLiteral("拖动可框选；按住 Ctrl 点击可多选。每个方案是一组本地配置，写入时只支持 USB 有线。"),
        QStringLiteral("tiny"));
    hint->setWordWrap(true);
    boardFooter->addWidget(hint, 0, Qt::AlignRight);
    boardCard->body()->addLayout(boardFooter);
    contentLayout()->addWidget(boardCard);

    auto *colorCard = new CardFrame;
    auto *colorHeader = new QHBoxLayout;
    colorHeader->addWidget(makeLabel(QStringLiteral("颜色"), QStringLiteral("cardTitle")));
    colorHeader->addStretch();
    colorHeader->addWidget(makeLabel(QStringLiteral("选色后自动刷到选中的按键"),
                                     QStringLiteral("muted")));
    colorCard->body()->addLayout(colorHeader);

    auto *colorRow = new QHBoxLayout;
    colorRow->setSpacing(24);

    m_wheel = new ColorWheel;
    m_wheel->setFixedSize(190, 190);
    colorRow->addWidget(m_wheel, 0, Qt::AlignTop);

    auto *colorTexts = new QVBoxLayout;
    colorTexts->setSpacing(10);
    auto *hexRow = new QHBoxLayout;
    hexRow->setSpacing(10);
    m_swatch = new QLabel;
    m_swatch->setFixedSize(36, 36);
    hexRow->addWidget(m_swatch);
    m_hex = new QLineEdit;
    m_hex->setPlaceholderText(QStringLiteral("#RRGGBB"));
    hexRow->addWidget(m_hex, 1);
    connect(m_hex, &QLineEdit::editingFinished, this, &UserLightPage::applyHex);
    colorTexts->addLayout(hexRow);

    auto *quickRow = new QHBoxLayout;
    quickRow->setSpacing(6);
    const QVector<QColor> quickColors = {
        QColor(0xFF, 0x3B, 0x30), QColor(0xFF, 0x95, 0x00), QColor(0xFF, 0xCC, 0x00),
        QColor(0x34, 0xC7, 0x59), QColor(0x32, 0xAD, 0xE6), QColor(0x0A, 0x84, 0xFF),
        QColor(0xAF, 0x52, 0xDE), QColor(0xFF, 0x2D, 0x55), QColor(0xFF, 0xFF, 0xFF)
    };
    for (const QColor &quick : quickColors) {
        auto *button = new QPushButton(this);
        button->setFixedSize(26, 26);
        button->setCursor(Qt::PointingHandCursor);
        button->setToolTip(Theme::hex(quick));
        button->setStyleSheet(
            QStringLiteral("QPushButton { background-color: %1; border: 1px solid rgba(255, 255, 255, 70); "
                           "border-radius: 13px; } "
                           "QPushButton:hover { border: 2px solid #FFFFFF; }")
                .arg(Theme::hex(quick)));
        connect(button, &QPushButton::clicked, this, [this, quick]() {
            m_color = quick;
            qreal hue = quick.hsvHueF();
            if (hue < 0.0)
                hue = 0.0;
            m_wheel->setHsv(hue, quick.hsvSaturationF(), 1.0);
            if (m_board->hasSelection())
                m_board->paintSelected(m_color);
            updateColorUi();
        });
        quickRow->addWidget(button);
    }
    quickRow->addStretch();
    colorTexts->addLayout(quickRow);

    auto *applySelection = new QPushButton(QStringLiteral("应用到选中的按键"), this);
    applySelection->setObjectName(QStringLiteral("ghost"));
    applySelection->setCursor(Qt::PointingHandCursor);
    connect(applySelection, &QPushButton::clicked, this, [this]() {
        m_board->paintSelected(m_color);
    });
    colorTexts->addWidget(applySelection);

    auto *clearAll = new QPushButton(QStringLiteral("把所有键恢复为不亮"), this);
    clearAll->setObjectName(QStringLiteral("ghost"));
    clearAll->setCursor(Qt::PointingHandCursor);
    connect(clearAll, &QPushButton::clicked, this, &UserLightPage::clearBoard);
    colorTexts->addWidget(clearAll);
    colorTexts->addStretch();
    colorRow->addLayout(colorTexts, 1);
    colorCard->body()->addLayout(colorRow);
    contentLayout()->addWidget(colorCard);

    auto *footer = new QWidget;
    auto *footerRow = new QHBoxLayout(footer);
    footerRow->setContentsMargins(0, 0, 0, 0);
    m_status = makeLabel(QStringLiteral("选好颜色后点右侧按钮写入键盘"), QStringLiteral("muted"));
    footerRow->addWidget(m_status, 1);
    m_apply = new QPushButton(QStringLiteral("写入键盘"), footer);
    m_apply->setObjectName(QStringLiteral("primary"));
    m_apply->setFixedSize(180, 48);
    m_apply->setCursor(Qt::PointingHandCursor);
    connect(m_apply, &QPushButton::clicked, this, &UserLightPage::applyToKeyboard);
    auto *glow = new QGraphicsDropShadowEffect(m_apply);
    glow->setBlurRadius(28);
    glow->setOffset(0, 0);
    glow->setColor(QColor(0x6E, 0x8B, 0xFF, 110));
    m_apply->setGraphicsEffect(glow);
    footerRow->addWidget(m_apply, 0, Qt::AlignRight);
    contentLayout()->addWidget(footer);
    contentLayout()->addStretch();

    connect(m_wheel, &ColorWheel::hsvChanged, this, [this](qreal hue, qreal saturation) {
        m_color = QColor::fromHsvF(hue, saturation, 1.0);
        if (m_board->hasSelection())
            m_board->paintSelected(m_color);
        updateColorUi();
    });

    connect(m_controller, &KeyboardController::perKeyApplied, this,
            [this](bool ok, const QString &message) {
                m_status->setText(message);
                m_status->setStyleSheet(
                    QStringLiteral("color: %1;")
                        .arg(ok ? Theme::Success.name() : Theme::Warning.name()));
            });

    auto updateEnabled = [this]() {
        const bool enabled = m_controller->connected() && !m_controller->busy();
        m_apply->setEnabled(enabled);
        m_apply->setText(m_controller->busy() ? QStringLiteral("正在写入...")
                                             : QStringLiteral("写入键盘"));
    };
    connect(m_controller, &KeyboardController::stateChanged, this, updateEnabled);
    updateEnabled();

    m_wheel->setHsv(m_color.hsvHueF() < 0.0 ? 0.0 : m_color.hsvHueF(),
                    m_color.hsvSaturationF(), 1.0);
    updateColorUi();
}

void UserLightPage::loadSlots()
{
    QSettings settings;
    const int count = qBound(1, settings.value(QStringLiteral("perkey/count"), 2).toInt(), 32);
    m_slots.clear();
    m_slotNames.clear();

    for (int slot = 0; slot < count; ++slot) {
        QString name = settings.value(QStringLiteral("perkey/name%1").arg(slot)).toString();
        if (name.isEmpty())
            name = QStringLiteral("方案 %1").arg(slot + 1);

        QVector<QColor> colors(KeyLayout::kRecordCount);
        const QString text = settings
            .value(QStringLiteral("perkey/slot%1").arg(slot))
            .toString();
        for (const QString &item : text.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
            const QStringList parts = item.split(QLatin1Char(':'));
            if (parts.size() != 2)
                continue;
            bool ok = false;
            const int index = parts[0].toInt(&ok);
            if (!ok || index < 0 || index >= KeyLayout::kRecordCount)
                continue;
            const QColor color(QStringLiteral("#") + parts[1]);
            if (color.isValid())
                colors[index] = color;
        }
        m_slots.append(colors);
        m_slotNames.append(name);
    }

    m_slot = 0;
    rebuildSlotTabs();
}

void UserLightPage::saveSlots()
{
    QSettings settings;
    settings.setValue(QStringLiteral("perkey/count"), m_slots.size());
    for (int slot = 0; slot < m_slots.size(); ++slot) {
        QStringList items;
        const QVector<QColor> &colors = m_slots[slot];
        for (int index = 0; index < colors.size(); ++index) {
            if (colors[index].isValid())
                items << QStringLiteral("%1:%2")
                             .arg(index)
                             .arg(colors[index].name().mid(1));
        }
        settings.setValue(QStringLiteral("perkey/name%1").arg(slot),
                          m_slotNames.value(slot));
        settings.setValue(QStringLiteral("perkey/slot%1").arg(slot),
                          items.join(QLatin1Char(',')));
    }
}

void UserLightPage::updateColorUi()
{
    m_swatch->setStyleSheet(
        QStringLiteral("background-color: %1; border: 1px solid rgba(255, 255, 255, 45); "
                       "border-radius: 10px;")
            .arg(Theme::hex(m_color)));
    if (!m_hex->hasFocus()) {
        const QSignalBlocker blocker(m_hex);
        m_hex->setText(Theme::hex(m_color));
    }
}

void UserLightPage::applyHex()
{
    QString text = m_hex->text().trimmed();
    if (!text.startsWith(QLatin1Char('#')) && text.size() == 6)
        text.prepend(QLatin1Char('#'));

    const QColor color(text);
    if (color.isValid()) {
        m_color = color;
        qreal hue = color.hsvHueF();
        if (hue < 0.0)
            hue = 0.0;
        m_wheel->setHsv(hue, color.hsvSaturationF(), 1.0);
        if (m_board->hasSelection())
            m_board->paintSelected(m_color);
    }
    m_hex->clearFocus();
    updateColorUi();
}

void UserLightPage::switchSlot(int slot)
{
    if (slot < 0 || slot >= m_slots.size() || slot == m_slot)
        return;

    m_slots[m_slot] = m_board->colors();
    saveSlots();
    m_slot = slot;
    m_board->setColors(m_slots[m_slot]);
    m_board->clearSelection();
    rebuildSlotTabs();
    m_status->setText(QStringLiteral("已切换到方案「%1」").arg(m_slotNames.value(m_slot)));
    updateColorUi();

    // 切组即写回键盘，否则另一组的颜色会被上一次的整块写入覆盖。
    if (m_controller->connected() && !m_controller->busy())
        m_controller->applyPerKey(m_board->colors(), m_slot);
}

void UserLightPage::rebuildSlotTabs()
{
    while (QLayoutItem *item = m_slotTabs->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }

    for (int i = 0; i < m_slots.size(); ++i) {
        auto *button = new QPushButton(m_slotNames.value(i), this);
        button->setObjectName(QStringLiteral("segment"));
        button->setCheckable(true);
        button->setChecked(i == m_slot);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, this, [this, i]() { switchSlot(i); });
        m_slotTabs->addWidget(button);
    }
}

void UserLightPage::addSlot()
{
    m_slots[m_slot] = m_board->colors();
    m_slots.append(QVector<QColor>(KeyLayout::kRecordCount));
    m_slotNames.append(QStringLiteral("方案 %1").arg(m_slots.size()));
    m_slot = m_slots.size() - 1;
    m_board->setColors(m_slots[m_slot]);
    m_board->clearSelection();
    rebuildSlotTabs();
    saveSlots();
    m_status->setText(QStringLiteral("已新建方案「%1」").arg(m_slotNames.value(m_slot)));
}

void UserLightPage::removeSlot()
{
    if (m_slots.size() <= 1) {
        m_status->setText(QStringLiteral("至少保留一个方案"));
        return;
    }

    m_slots.removeAt(m_slot);
    m_slotNames.removeAt(m_slot);
    m_slot = qBound(0, m_slot, m_slots.size() - 1);
    m_board->setColors(m_slots[m_slot]);
    m_board->clearSelection();
    rebuildSlotTabs();
    saveSlots();
    m_status->setText(QStringLiteral("已删除，当前方案「%1」")
                           .arg(m_slotNames.value(m_slot)));

    if (m_controller->connected() && !m_controller->busy())
        m_controller->applyPerKey(m_board->colors(), m_slot);
}

void UserLightPage::refreshSelection()
{
    const QVector<int> keys = m_board->selectedKeys();
    if (keys.isEmpty()) {
        m_selection->setText(QStringLiteral("未选择按键"));
        return;
    }
    m_selection->setText(QStringLiteral("已选 %1 个按键").arg(keys.size()));
    if (keys.size() == 1) {
        const QColor color = m_board->colors().value(keys.first());
        if (color.isValid()) {
            m_color = color;
            updateColorUi();
        }
    }
}

void UserLightPage::applyToKeyboard()
{
    m_slots[m_slot] = m_board->colors();
    saveSlots();
    m_controller->applyPerKey(m_board->colors(), m_slot);
}

void UserLightPage::clearBoard()
{
    m_board->setColors(QVector<QColor>(KeyLayout::kRecordCount));
    m_slots[m_slot] = m_board->colors();
    saveSlots();
    m_status->setText(QStringLiteral("已将编辑区所有键设为不亮，点「写入键盘」生效"));
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

GifPage::GifPage(KeyboardController *controller, QWidget *parent)
    : PageScroll(parent)
    , m_controller(controller)
{
    buildUi();
    refreshAvailability();

    connect(m_controller, &KeyboardController::stateChanged, this,
            &GifPage::refreshAvailability);
    connect(m_controller, &KeyboardController::gifUploadProgress, this,
            [this](int percent) { m_progress->setValue(percent); });
    connect(m_controller, &KeyboardController::gifUploaded, this,
            [this](bool ok, const QString &) {
                if (!ok)
                    m_progress->setValue(0);
                m_choose->setEnabled(true);
                refreshAvailability();
            });
}

void GifPage::buildUi()
{
    auto *card = new CardFrame;
    card->body()->addWidget(makeLabel(QStringLiteral("TFT 动图"),
                                      QStringLiteral("cardTitle")));
    auto *hint = makeLabel(
        QStringLiteral("选择 GIF 或静态图片后会按 240×135 转成 RGB565 写入键盘。"
                       "动图最多 108 帧，超过的部分会被截断；单张图片按 1 帧处理。"),
        QStringLiteral("tiny"));
    hint->setWordWrap(true);
    card->body()->addWidget(hint);
    card->body()->addWidget(makeDivider());

    m_preview = new QLabel(QStringLiteral("未选择 GIF"));
    m_preview->setFixedSize(320, 180);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setStyleSheet(
        QStringLiteral("background-color: #0B0F16; border: 1px solid #1C2331; "
                       "border-radius: 12px; color: #5E697C;"));

    auto *side = new QVBoxLayout;
    side->setSpacing(8);
    m_fileLabel = makeLabel(QStringLiteral("还没有选择文件"), QStringLiteral("mono"));
    m_fileLabel->setWordWrap(true);
    m_infoLabel = makeLabel(QString(), QStringLiteral("tiny"));
    m_infoLabel->setWordWrap(true);
    m_choose = new QPushButton(QStringLiteral("选择 GIF..."));
    m_choose->setObjectName(QStringLiteral("ghost"));
    m_choose->setMinimumHeight(38);
    m_choose->setCursor(Qt::PointingHandCursor);
    connect(m_choose, &QPushButton::clicked, this, &GifPage::chooseFile);
    side->addWidget(m_fileLabel);
    side->addWidget(m_infoLabel);
    side->addWidget(m_choose, 0, Qt::AlignLeft);
    side->addStretch();

    auto *previewRow = new QHBoxLayout;
    previewRow->setSpacing(18);
    previewRow->addWidget(m_preview, 0, Qt::AlignTop);
    previewRow->addLayout(side, 1);
    card->body()->addLayout(previewRow);
    contentLayout()->addWidget(card);

    auto *applyCard = new CardFrame;
    applyCard->body()->addWidget(makeLabel(QStringLiteral("应用到键盘"),
                                           QStringLiteral("cardTitle")));
    m_availability = makeLabel(QString(), QStringLiteral("tiny"));
    m_availability->setWordWrap(true);
    applyCard->body()->addWidget(m_availability);

    m_progress = new QProgressBar;
    m_progress->setObjectName(QStringLiteral("uploadProgress"));
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(10);
    applyCard->body()->addWidget(m_progress);

    auto *row = new QHBoxLayout;
    row->addStretch();
    m_upload = new QPushButton(QStringLiteral("上传到键盘"));
    m_upload->setObjectName(QStringLiteral("primary"));
    m_upload->setFixedSize(180, 48);
    m_upload->setCursor(Qt::PointingHandCursor);
    connect(m_upload, &QPushButton::clicked, this, &GifPage::upload);
    row->addWidget(m_upload);
    applyCard->body()->addLayout(row);
    contentLayout()->addWidget(applyCard);
    contentLayout()->addStretch();
}

void GifPage::chooseFile()
{
    QFileDialog dialog(this, QStringLiteral("选择动图或图片"));
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setNameFilters({
        QStringLiteral("GIF / 图片 (*.gif *.png *.jpg *.jpeg *.bmp *.webp)"),
        QStringLiteral("所有文件 (*)"),
    });
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString path = dialog.selectedFiles().value(0);
    if (path.isEmpty())
        return;
    m_path = path;
    loadPreview();
    refreshAvailability();
}

void GifPage::loadPreview()
{
    QImageReader reader(m_path);
    if (!reader.canRead()) {
        m_infoLabel->setText(QStringLiteral("无法读取：%1").arg(reader.errorString()));
        return;
    }

    const int count = reader.imageCount();
    const QSize size = reader.size();
    QString info = count > 1 ? QStringLiteral("帧数 %1").arg(count)
                             : QStringLiteral("单帧图片");
    if (size.isValid())
        info += QStringLiteral(" · %1×%2").arg(size.width()).arg(size.height());
    if (count > 108)
        info += QStringLiteral(" · 只上传前 108 帧");
    m_infoLabel->setText(info);
    m_fileLabel->setText(QFileInfo(m_path).fileName());

    if (m_movie) {
        m_movie->stop();
        m_movie->deleteLater();
        m_movie = nullptr;
    }

    if (count > 1) {
        m_movie = new QMovie(m_path, QByteArray(), this);
        m_movie->setCacheMode(QMovie::CacheAll);
        m_movie->setScaledSize(m_preview->size());
        m_preview->setMovie(m_movie);
        m_movie->start();
    } else {
        const QImage image = reader.read();
        m_preview->setPixmap(QPixmap::fromImage(image).scaled(
            m_preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void GifPage::upload()
{
    if (m_path.isEmpty())
        return;
    m_choose->setEnabled(false);
    m_upload->setEnabled(false);
    m_progress->setValue(0);
    m_controller->uploadGif(m_path);
}

void GifPage::refreshAvailability()
{
    const bool connected = m_controller->connected();
    const bool wireless = m_controller->connectionType() == QStringLiteral("wireless");
    const bool busy = m_controller->busy();
    const bool usable = connected && !wireless && !busy;

    m_upload->setEnabled(usable && !m_path.isEmpty());
    m_choose->setEnabled(!busy);

    if (!connected) {
        m_availability->setText(QStringLiteral("当前未连接键盘。请插上 USB 线。"));
        m_availability->setStyleSheet(QStringLiteral("color: #F0A23C;"));
    } else if (wireless) {
        m_availability->setText(QStringLiteral(
            "当前连接：2.4G 无线。TFT 动图目前只能通过 USB 有线写入——请插上 USB 线，"
            "并确认键盘处于有线模式。"));
        m_availability->setStyleSheet(QStringLiteral("color: #F0A23C;"));
    } else {
        m_availability->setText(QStringLiteral(
            "当前连接：USB 有线。上传期间不要拔线；单帧约几秒，108 帧约 4 分钟。"));
        m_availability->setStyleSheet(QStringLiteral("color: #5FD0A0;"));
    }
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
        {QStringLiteral("TFT 动图（GIF 上传）"), QStringLiteral("USB 已验证"), true},
        {QStringLiteral("系统配置（响应 / 休眠 / 禁用按键）"), QStringLiteral("USB 已验证"), true},
        {QStringLiteral("重置键盘 / 恢复出厂"), QStringLiteral("USB 已验证"), true},
        {QStringLiteral("USB 有线 / 2.4G 无线自动识别"), QStringLiteral("已支持"), true},
        {QStringLiteral("2.4G 灯效写入"), QStringLiteral("实测通过"), true},
        {QStringLiteral("用户灯光（逐键 RGB）"), QStringLiteral("USB 已验证"), true}
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
