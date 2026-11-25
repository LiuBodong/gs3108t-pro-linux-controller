#include "widgets.h"

#include "theme.h"

#include <QButtonGroup>
#include <QEnterEvent>
#include <QFontMetrics>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

#include <algorithm>
#include <cmath>

namespace {
constexpr qreal kPi = 3.14159265358979323846;

QColor withAlpha(const QColor &color, int alpha)
{
    QColor result = color;
    result.setAlpha(qBound(0, alpha, 255));
    return result;
}

QColor blend(const QColor &a, const QColor &b, qreal t)
{
    t = qBound<qreal>(0.0, t, 1.0);
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t,
                            a.alphaF() + (b.alphaF() - a.alphaF()) * t);
}
} // namespace

CardFrame::CardFrame(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("card"));
    setAttribute(Qt::WA_StyledBackground, true);
    m_body = new QVBoxLayout(this);
    m_body->setContentsMargins(20, 20, 20, 20);
    m_body->setSpacing(14);
}

NavButton::NavButton(const QString &iconResource, const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_text(text)
{
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(46);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_iconDim = Theme::tintedIcon(iconResource, Theme::TextDim, 19);
    m_iconActive = Theme::tintedIcon(iconResource, Theme::Text, 19);
}

void NavButton::setSelected(bool selected)
{
    if (m_selected == selected)
        return;
    m_selected = selected;
    update();
}

QSize NavButton::sizeHint() const
{
    return QSize(200, 46);
}

void NavButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF background = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    if (m_selected || m_hover) {
        painter.setPen(m_selected ? QPen(QColor(0x33, 0x40, 0x66), 1) : Qt::NoPen);
        painter.setBrush(m_selected ? QColor(0x23, 0x2B, 0x40) : QColor(0x18, 0x1E, 0x2A));
        painter.drawRoundedRect(background, 12, 12);
    }

    if (m_selected) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::Accent);
        painter.drawRoundedRect(QRectF(0, (height() - 20) / 2.0, 3, 20), 1.5, 1.5);
    }

    const QPixmap &icon = m_selected ? m_iconActive : m_iconDim;
    if (!icon.isNull())
        painter.drawPixmap(QPointF(16, (height() - 19) / 2.0), icon);

    painter.setPen(m_selected ? Theme::Text : Theme::TextDim);
    painter.setFont(Theme::uiFont(14, m_selected ? QFont::DemiBold : QFont::Normal));
    painter.drawText(QRectF(48, 0, width() - 60, height()),
                     Qt::AlignVCenter | Qt::AlignLeft, m_text);
}

void NavButton::enterEvent(QEnterEvent *event)
{
    m_hover = true;
    update();
    QWidget::enterEvent(event);
}

void NavButton::leaveEvent(QEvent *event)
{
    m_hover = false;
    update();
    QWidget::leaveEvent(event);
}

void NavButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()))
        emit clicked();
    QWidget::mouseReleaseEvent(event);
}

StatusChip::StatusChip(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(34);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_timer = new QTimer(this);
    m_timer->setInterval(50);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        m_phase += 0.12;
        if (m_phase > kPi * 2)
            m_phase -= kPi * 2;
        update();
    });
    m_timer->start();
}

void StatusChip::setStatus(bool connected, const QString &text, const QString &iconResource)
{
    m_connected = connected;
    m_text = text;
    m_icon = iconResource.isEmpty() ? QPixmap()
                                    : Theme::tintedIcon(iconResource, Theme::TextDim, 13);
    updateGeometry();
    update();
}

QSize StatusChip::sizeHint() const
{
    const QFontMetrics metrics(Theme::uiFont(12));
    int width = metrics.horizontalAdvance(m_text) + 39;
    if (!m_icon.isNull())
        width += 19;
    return QSize(qMax(96, width), 34);
}

void StatusChip::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF background = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QPen(Theme::Border, 1));
    painter.setBrush(QColor(0x14, 0x1A, 0x25));
    painter.drawRoundedRect(background, 17, 17);

    int x = 14;
    QColor dot = m_connected ? Theme::Success : Theme::Danger;
    if (m_connected)
        dot.setAlphaF(0.55 + 0.45 * (0.5 + 0.5 * std::sin(m_phase)));
    painter.setPen(Qt::NoPen);
    painter.setBrush(dot);
    painter.drawEllipse(QPointF(x + 4, height() / 2.0), 4, 4);
    x += 13;

    if (!m_icon.isNull()) {
        painter.drawPixmap(QPointF(x, (height() - 13) / 2.0), m_icon);
        x += 19;
    }

    painter.setPen(Theme::Text);
    painter.setFont(Theme::uiFont(12));
    painter.drawText(QRectF(x, 0, width() - x - 12, height()),
                     Qt::AlignVCenter | Qt::AlignLeft, m_text);
}

ModeChip::ModeChip(int mode, const QString &name, QWidget *parent)
    : QWidget(parent)
    , m_index(mode)
    , m_name(name)
    , m_accent(Theme::Accent)
{
    setFixedHeight(74);
    setMinimumWidth(64);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
}

void ModeChip::setSelected(bool selected)
{
    if (m_selected == selected)
        return;
    m_selected = selected;
    update();
}

void ModeChip::setAccentColor(const QColor &color)
{
    m_accent = color;
    update();
}

QSize ModeChip::sizeHint() const
{
    return QSize(96, 74);
}

void ModeChip::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF background = QRectF(rect()).adjusted(0.75, 0.75, -0.75, -0.75);
    QColor fill = QColor(0x12, 0x17, 0x22);
    if (m_selected)
        fill = withAlpha(m_accent, 38);
    else if (m_hover)
        fill = QColor(0x1A, 0x20, 0x30);

    painter.setPen(QPen(m_selected ? m_accent : Theme::Border, m_selected ? 1.5 : 1.0));
    painter.setBrush(fill);
    painter.drawRoundedRect(background, 14, 14);

    painter.setPen(Qt::NoPen);
    painter.setBrush(m_accent);
    painter.drawEllipse(QPointF(18, 24), 4, 4);

    painter.setPen(Theme::Text);
    painter.setFont(Theme::uiFont(13, QFont::DemiBold));
    painter.drawText(QRectF(28, 12, width() - 38, 20),
                     Qt::AlignVCenter | Qt::AlignLeft, QString::number(m_index));

    painter.setPen(Theme::TextMuted);
    painter.setFont(Theme::uiFont(10));
    painter.drawText(QRectF(16, 44, width() - 28, 16),
                     Qt::AlignVCenter | Qt::AlignLeft, m_name);

    if (m_selected) {
        const QPixmap check = Theme::tintedIcon(QStringLiteral(":/assets/icons/check.svg"),
                                                m_accent, 12);
        painter.drawPixmap(QPointF(width() - 20, 8), check);
    }
}

void ModeChip::enterEvent(QEnterEvent *event)
{
    m_hover = true;
    update();
    QWidget::enterEvent(event);
}

void ModeChip::leaveEvent(QEvent *event)
{
    m_hover = false;
    update();
    QWidget::leaveEvent(event);
}

void ModeChip::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()))
        emit clicked(m_index);
    QWidget::mouseReleaseEvent(event);
}

AppDialog::AppDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
}

void AppDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF body = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QPen(QColor(0x33, 0x40, 0x5A), 1));
    painter.setBrush(QColor(0x0E, 0x13, 0x1B));
    painter.drawRoundedRect(body, 18, 18);
}

void AppDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (windowHandle() && windowHandle()->startSystemMove())
            return;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void AppDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        move(event->globalPosition().toPoint() - m_dragOffset);
    QDialog::mouseMoveEvent(event);
}

void AppDialog::centerOnParent()
{
    adjustSize();
    if (QWidget *owner = parentWidget())
        move(owner->mapToGlobal(owner->rect().center())
             - QPoint(width() / 2, height() / 2));
}

void AppDialog::armAutoScreenshot()
{
    const QString path = qEnvironmentVariable("GSKBD_DIALOG_SHOT");
    if (path.isEmpty())
        return;
    QTimer::singleShot(500, this, [this, path]() {
        grab().save(path);
        reject();
    });
}

QString AppDialog::prompt(QWidget *parent, const QString &title, const QString &label,
                          const QString &initial, bool *accepted)
{
    AppDialog dialog(parent);
    dialog.setFixedWidth(420);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 22, 26, 22);
    layout->setSpacing(12);

    auto *titleLabel = new QLabel(title);
    titleLabel->setFont(Theme::uiFont(16, QFont::DemiBold));
    titleLabel->setStyleSheet(QStringLiteral("color: #F4F7FC;"));
    layout->addWidget(titleLabel);

    auto *hint = new QLabel(label);
    hint->setStyleSheet(QStringLiteral("color: #8B95A9;"));
    layout->addWidget(hint);

    auto *edit = new QLineEdit(initial);
    edit->setMinimumHeight(38);
    edit->selectAll();
    layout->addWidget(edit);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addStretch();

    auto *cancel = new QPushButton(QStringLiteral("取消"));
    cancel->setObjectName(QStringLiteral("ghost"));
    cancel->setCursor(Qt::PointingHandCursor);

    auto *confirmButton = new QPushButton(QStringLiteral("保存"));
    confirmButton->setObjectName(QStringLiteral("primary"));
    confirmButton->setCursor(Qt::PointingHandCursor);
    confirmButton->setFixedHeight(38);
    confirmButton->setMinimumWidth(96);

    buttons->addWidget(cancel);
    buttons->addWidget(confirmButton);
    layout->addLayout(buttons);

    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(confirmButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(edit, &QLineEdit::returnPressed, &dialog, &QDialog::accept);

    edit->setFocus();
    dialog.centerOnParent();
    dialog.armAutoScreenshot();

    const bool ok = dialog.exec() == QDialog::Accepted;
    if (accepted)
        *accepted = ok;
    return ok ? edit->text() : QString();
}

bool AppDialog::confirm(QWidget *parent, const QString &title, const QString &text,
                        const QString &acceptText, const QString &cancelText)
{
    AppDialog dialog(parent);
    dialog.setFixedWidth(400);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 22, 26, 22);
    layout->setSpacing(12);

    auto *titleLabel = new QLabel(title);
    titleLabel->setFont(Theme::uiFont(16, QFont::DemiBold));
    titleLabel->setStyleSheet(QStringLiteral("color: #F4F7FC;"));
    layout->addWidget(titleLabel);

    auto *body = new QLabel(text);
    body->setWordWrap(true);
    body->setStyleSheet(QStringLiteral("color: #B9C6E6;"));
    layout->addWidget(body);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addStretch();

    auto *cancel = new QPushButton(cancelText);
    cancel->setObjectName(QStringLiteral("ghost"));
    cancel->setCursor(Qt::PointingHandCursor);

    auto *accept = new QPushButton(acceptText);
    accept->setObjectName(QStringLiteral("danger"));
    accept->setCursor(Qt::PointingHandCursor);

    buttons->addWidget(cancel);
    buttons->addWidget(accept);
    layout->addLayout(buttons);

    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(accept, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.centerOnParent();
    dialog.armAutoScreenshot();
    return dialog.exec() == QDialog::Accepted;
}

ColorWheel::ColorWheel(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(160, 160);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::CrossCursor);
}

QSize ColorWheel::sizeHint() const
{
    return QSize(200, 200);
}

QRectF ColorWheel::wheelRect() const
{
    const int side = qMin(width(), height());
    return QRectF((width() - side) / 2.0, (height() - side) / 2.0, side, side)
        .adjusted(1.0, 1.0, -1.0, -1.0);
}

void ColorWheel::ensureWheel()
{
    const QRectF area = wheelRect();
    const int side = qMax(10, qRound(area.width()));
    if (!m_wheel.isNull() && m_cacheSize == QSize(side, side))
        return;

    QImage image(side, side, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    const qreal center = side / 2.0;
    const qreal radius = center - 1.0;
    for (int y = 0; y < side; ++y) {
        for (int x = 0; x < side; ++x) {
            const qreal dx = x + 0.5 - center;
            const qreal dy = y + 0.5 - center;
            const qreal distance = std::hypot(dx, dy);
            if (distance > radius)
                continue;
            qreal hue = std::atan2(dy, dx) / (2.0 * kPi) + 0.5;
            if (hue < 0.0)
                hue += 1.0;
            if (hue >= 1.0)
                hue -= 1.0;
            const qreal saturation = qBound<qreal>(0.0, distance / radius, 1.0);
            const qreal alpha = qBound<qreal>(0.0, (radius - distance) * 2.0, 1.0);
            image.setPixelColor(x, y, QColor::fromHsvF(hue, saturation, 1.0, alpha));
        }
    }

    m_wheel = image;
    m_cacheSize = QSize(side, side);
}

void ColorWheel::setHsv(qreal hue, qreal saturation, qreal value)
{
    hue = std::fmod(hue, 1.0);
    if (hue < 0.0)
        hue += 1.0;
    m_hue = hue;
    m_saturation = qBound<qreal>(0.0, saturation, 1.0);
    m_value = qBound<qreal>(0.0, value, 1.0);
    update();
}

void ColorWheel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    ensureWheel();
    const QRectF area = wheelRect();
    painter.drawImage(area, m_wheel);

    painter.setPen(QPen(QColor(255, 255, 255, 26), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(area.adjusted(0.5, 0.5, -0.5, -0.5));

    const QPointF center = area.center();
    const qreal angle = (m_hue - 0.5) * 2.0 * kPi;
    const qreal distance = m_saturation * area.width() / 2.0;
    const QPointF indicator(center.x() + std::cos(angle) * distance,
                            center.y() + std::sin(angle) * distance);

    painter.setPen(Qt::NoPen);
    painter.setBrush(withAlpha(QColor::fromHsvF(m_hue, m_saturation, m_value), 50));
    painter.drawEllipse(indicator, 13, 13);

    painter.setPen(QPen(QColor(0, 0, 0, 90), 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(indicator, 8.5, 8.5);

    painter.setPen(QPen(Qt::white, 2));
    painter.setBrush(QColor::fromHsvF(m_hue, m_saturation, m_value));
    painter.drawEllipse(indicator, 7, 7);
}

void ColorWheel::pick(const QPointF &pos)
{
    const QRectF area = wheelRect();
    const QPointF center = area.center();
    const qreal radius = area.width() / 2.0;
    qreal dx = pos.x() - center.x();
    qreal dy = pos.y() - center.y();
    qreal distance = std::hypot(dx, dy);
    if (distance > radius && distance > 0.0) {
        dx *= radius / distance;
        dy *= radius / distance;
        distance = radius;
    }

    qreal hue = std::atan2(dy, dx) / (2.0 * kPi) + 0.5;
    if (hue < 0.0)
        hue += 1.0;
    if (hue >= 1.0)
        hue -= 1.0;
    const qreal saturation = qBound<qreal>(0.0, distance / radius, 1.0);

    if (std::abs(hue - m_hue) < 0.0005 && std::abs(saturation - m_saturation) < 0.0005)
        return;

    m_hue = hue;
    m_saturation = saturation;
    update();
    emit hsvChanged(m_hue, m_saturation);
}

void ColorWheel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        pick(event->position());
    }
    QWidget::mousePressEvent(event);
}

void ColorWheel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging)
        pick(event->position());
    QWidget::mouseMoveEvent(event);
}

void ColorWheel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}

void ColorWheel::resizeEvent(QResizeEvent *event)
{
    m_cacheSize = QSize();
    QWidget::resizeEvent(event);
}

GlowSlider::GlowSlider(const QString &label, int minimum, int maximum, int value,
                       QWidget *parent)
    : QWidget(parent)
    , m_label(label)
    , m_minimum(minimum)
    , m_maximum(maximum)
    , m_accent(Theme::Accent)
{
    m_value = qBound(m_minimum, value, m_maximum);
    setMinimumHeight(56);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
}

void GlowSlider::setValue(int value)
{
    value = qBound(m_minimum, value, m_maximum);
    if (m_value == value)
        return;
    m_value = value;
    update();
    emit valueChanged(m_value);
}

void GlowSlider::setAccentColor(const QColor &color)
{
    m_accent = color;
    update();
}

QSize GlowSlider::sizeHint() const
{
    return QSize(240, 56);
}

void GlowSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setFont(Theme::uiFont(12));
    painter.setPen(Theme::TextDim);
    painter.drawText(QRectF(0, 0, width() - 40, 18),
                     Qt::AlignVCenter | Qt::AlignLeft, m_label);

    painter.setFont(Theme::uiFont(13, QFont::DemiBold));
    painter.setPen(Theme::Text);
    painter.drawText(QRectF(width() - 40, 0, 40, 18),
                     Qt::AlignVCenter | Qt::AlignRight, QString::number(m_value));

    const int trackY = height() - 20;
    const qreal ratio = (m_maximum == m_minimum)
                            ? 0.0
                            : qreal(m_value - m_minimum) / qreal(m_maximum - m_minimum);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Theme::Track);
    painter.drawRoundedRect(QRectF(0, trackY, width(), 8), 4, 4);

    if (ratio > 0.0) {
        QLinearGradient gradient(0, trackY, width(), trackY);
        gradient.setColorAt(0.0, m_accent);
        gradient.setColorAt(1.0, Theme::Accent2);
        painter.setBrush(gradient);
        painter.drawRoundedRect(QRectF(0, trackY, width() * ratio, 8), 4, 4);
    }

    const qreal handleX = width() * ratio - 9;
    painter.setBrush(withAlpha(m_accent, 45));
    painter.drawEllipse(QPointF(handleX + 9, trackY + 4), 15, 15);

    painter.setPen(QPen(m_accent, 3));
    painter.setBrush(Qt::white);
    painter.drawEllipse(QPointF(handleX + 9, trackY + 4), 8, 8);
}

void GlowSlider::setValueFromX(qreal x)
{
    const qreal ratio = qBound<qreal>(0.0, x / qMax(1, width()), 1.0);
    const int value = m_minimum + qRound(ratio * (m_maximum - m_minimum));
    setValue(value);
}

void GlowSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        setValueFromX(event->position().x());
    }
    QWidget::mousePressEvent(event);
}

void GlowSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging)
        setValueFromX(event->position().x());
    QWidget::mouseMoveEvent(event);
}

void GlowSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}

SegmentedControl::SegmentedControl(const QStringList &labels, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("segmented"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(38);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(3, 3, 3, 3);
    m_layout->setSpacing(0);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    for (int i = 0; i < labels.size(); ++i) {
        auto *button = new QPushButton(labels.at(i), this);
        button->setObjectName(QStringLiteral("segment"));
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_layout->addWidget(button);
        m_group->addButton(button, i);
        m_buttons.append(button);
    }

    if (!m_buttons.isEmpty())
        m_buttons.first()->setChecked(true);

    connect(m_group, &QButtonGroup::idClicked, this, [this](int id) {
        if (id == m_currentIndex) {
            m_buttons.at(id)->setChecked(true);
            return;
        }
        m_currentIndex = id;
        emit currentChanged(id);
    });
}

void SegmentedControl::setCurrentIndex(int index)
{
    if (m_buttons.isEmpty())
        return;
    index = qBound(0, index, m_buttons.size() - 1);
    if (m_currentIndex == index && m_buttons.at(index)->isChecked())
        return;
    m_currentIndex = index;
    m_buttons.at(index)->setChecked(true);
}

QSize SegmentedControl::sizeHint() const
{
    return QSize(300, 38);
}

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(46, 26);
    setCursor(Qt::PointingHandCursor);
    m_animation = new QPropertyAnimation(this, "knobPos", this);
    m_animation->setDuration(160);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
}

void ToggleSwitch::setKnobPos(qreal pos)
{
    m_knobPos = qBound<qreal>(0.0, pos, 1.0);
    update();
}

void ToggleSwitch::setChecked(bool checked, bool animate)
{
    m_checked = checked;
    const qreal target = checked ? 1.0 : 0.0;
    m_animation->stop();
    if (!animate) {
        setKnobPos(target);
        return;
    }
    m_animation->setStartValue(m_knobPos);
    m_animation->setEndValue(target);
    m_animation->start();
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(46, 26);
}

void ToggleSwitch::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QColor track = blend(Theme::Track, Theme::Accent, m_knobPos);
    painter.setPen(QPen(blend(Theme::Border, Theme::Accent, m_knobPos), 1));
    painter.setBrush(track);
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 13, 13);

    const qreal knobX = 3.0 + m_knobPos * (width() - 20.0 - 6.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 40));
    painter.drawEllipse(QRectF(knobX - 1, 2, 22, 22));

    painter.setBrush(Qt::white);
    painter.drawEllipse(QRectF(knobX, 3, 20, 20));
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint())) {
        setChecked(!m_checked, true);
        emit toggled(m_checked);
    }
    QWidget::mouseReleaseEvent(event);
}

KeyboardPreview::KeyboardPreview(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(360, 150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_timer = new QTimer(this);
    m_timer->setInterval(33);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        m_phase += 0.012 + m_speed * 0.004;
        if (m_phase >= 1.0)
            m_phase -= 1.0;
        update();
    });
    m_timer->start();
}

void KeyboardPreview::setColor(const QColor &color)
{
    m_color = color;
    update();
}

void KeyboardPreview::setBrightness(int brightness)
{
    m_brightness = qBound(0, brightness, 5);
    update();
}

void KeyboardPreview::setSpeed(int speed)
{
    m_speed = qBound(0, speed, 5);
}

QSize KeyboardPreview::sizeHint() const
{
    return QSize(420, 170);
}

void KeyboardPreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF area = rect();
    const QPointF center = area.center();

    QRadialGradient glow(center, qMin(area.width(), area.height()) * 0.58);
    glow.setColorAt(0.0, withAlpha(m_color, 36 + m_brightness * 5));
    glow.setColorAt(1.0, withAlpha(m_color, 0));
    painter.setPen(Qt::NoPen);
    painter.fillRect(area, glow);

    const int keyWidth = 22;
    const int keyHeight = 18;
    const int gap = 4;
    const int columns = 12;
    const int rows = 4;
    const qreal totalWidth = columns * keyWidth + (columns - 1) * gap;
    const qreal totalHeight = (rows + 1) * keyHeight + rows * gap;
    const qreal x0 = area.center().x() - totalWidth / 2.0;
    const qreal y0 = area.center().y() - totalHeight / 2.0 - 5.0;

    auto waveAlpha = [this](int row, int column) {
        const qreal position = std::fmod(m_phase * 16.0 + row * 2.0, 16.0);
        const qreal distance = std::abs(position - column);
        const qreal wave = std::exp(-(distance * distance) / 6.5);
        return qBound(0, int(18 + wave * (46 + m_brightness * 14)), 240);
    };

    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const QRectF key(x0 + column * (keyWidth + gap),
                             y0 + row * (keyHeight + gap),
                             keyWidth, keyHeight);
            painter.setPen(QPen(QColor(255, 255, 255, 16), 1));
            painter.setBrush(withAlpha(m_color, waveAlpha(row, column)));
            painter.drawRoundedRect(key, 4, 4);
        }
    }

    const qreal spaceY = y0 + rows * (keyHeight + gap);
    const qreal spaceWidth = 160;
    const qreal sideWidth = (totalWidth - spaceWidth - 2 * gap) / 2.0;
    const qreal spaceWave = std::exp(-std::pow(m_phase * 16.0 - 8.0, 2.0) / 12.0);
    const int spaceAlpha = qBound(0, int(18 + spaceWave * (46 + m_brightness * 14)), 240);

    painter.setPen(QPen(QColor(255, 255, 255, 16), 1));
    painter.setBrush(withAlpha(m_color, 40));
    painter.drawRoundedRect(QRectF(x0, spaceY, sideWidth, keyHeight), 4, 4);
    painter.setBrush(withAlpha(m_color, spaceAlpha));
    painter.drawRoundedRect(QRectF(x0 + sideWidth + gap, spaceY, spaceWidth, keyHeight), 4, 4);
    painter.setBrush(withAlpha(m_color, 40));
    painter.drawRoundedRect(QRectF(x0 + sideWidth + gap + spaceWidth + gap, spaceY,
                                   sideWidth, keyHeight), 4, 4);

    QLinearGradient reflection(x0, spaceY + keyHeight + 6, x0, spaceY + keyHeight + 30);
    reflection.setColorAt(0.0, withAlpha(m_color, 26));
    reflection.setColorAt(1.0, withAlpha(m_color, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(reflection);
    painter.drawRoundedRect(QRectF(x0, spaceY + keyHeight + 6, totalWidth, 24), 10, 10);
}

PerKeyBoard::PerKeyBoard(QWidget *parent)
    : QWidget(parent)
    , m_colors(KeyLayout::kRecordCount)
{
    setMinimumSize(520, 220);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
}

void PerKeyBoard::setColors(const QVector<QColor> &colors)
{
    m_colors = colors;
    if (m_colors.size() < KeyLayout::kRecordCount)
        m_colors.resize(KeyLayout::kRecordCount);
    update();
}

QVector<int> PerKeyBoard::selectedKeys() const
{
    QVector<int> keys = m_selected.values().toVector();
    std::sort(keys.begin(), keys.end());
    return keys;
}

void PerKeyBoard::clearSelection()
{
    if (m_selected.isEmpty())
        return;
    m_selected.clear();
    emit selectionChanged();
    update();
}

void PerKeyBoard::selectAll()
{
    for (int i = 0; i < KeyLayout::kKeyCount; ++i)
        m_selected.insert(KeyLayout::kKeys[i].index);
    emit selectionChanged();
    update();
}

void PerKeyBoard::paintSelected(const QColor &color)
{
    if (!color.isValid())
        return;
    for (int index : m_selected) {
        if (index >= 0 && index < m_colors.size())
            m_colors[index] = color;
    }
    update();
}

QSize PerKeyBoard::sizeHint() const
{
    return QSize(860, 340);
}

QRectF PerKeyBoard::boardRect() const
{
    const qreal padding = 18.0;
    const QRectF available = rect().adjusted(padding, padding, -padding, -padding);
    const qreal scale = qMin(available.width() / KeyLayout::kBoardWidth,
                             available.height() / KeyLayout::kBoardHeight);
    const qreal width = KeyLayout::kBoardWidth * scale;
    const qreal height = KeyLayout::kBoardHeight * scale;
    return QRectF(available.center().x() - width / 2.0,
                  available.center().y() - height / 2.0, width, height);
}

QRectF PerKeyBoard::keyRect(const KeyLayout::Key &key) const
{
    const QRectF board = boardRect();
    const qreal scale = board.width() / KeyLayout::kBoardWidth;
    const qreal x = board.left() + (key.x - 12.0) * scale;
    const qreal y = board.top() + (key.y - 17.0) * scale;
    return QRectF(x, y, key.w * scale, key.h * scale);
}

int PerKeyBoard::keyAt(const QPointF &pos) const
{
    for (int i = 0; i < KeyLayout::kKeyCount; ++i) {
        if (keyRect(KeyLayout::kKeys[i]).contains(pos))
            return KeyLayout::kKeys[i].index;
    }
    return -1;
}

void PerKeyBoard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF board = boardRect();
    painter.setPen(QPen(QColor(255, 255, 255, 18), 1));
    painter.setBrush(QColor(0x0B1018));
    painter.drawRoundedRect(board.adjusted(-10, -10, 10, 10), 14, 14);

    const QRectF dragRect = QRectF(m_dragStart, m_dragCurrent).normalized();

    for (int i = 0; i < KeyLayout::kKeyCount; ++i) {
        const KeyLayout::Key &key = KeyLayout::kKeys[i];
        const QRectF r = keyRect(key);
        const QColor color = m_colors.value(key.index);
        const bool selected = m_selected.contains(key.index);
        const bool hovered = m_hover == key.index;

        painter.setBrush(color.isValid() ? color : QColor(0x1A2230));
        painter.setPen(QPen(selected ? Theme::Accent
                                     : (hovered ? QColor(255, 255, 255, 90)
                                                : QColor(255, 255, 255, 22)),
                            selected ? 1.8 : 1.0));
        painter.drawRoundedRect(r, 4, 4);

        if (selected) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(withAlpha(Theme::Accent, 70), 3.0));
            painter.drawRoundedRect(r.adjusted(-1.5, -1.5, 1.5, 1.5), 5, 5);
        }

        const bool bright = color.isValid() && color.lightnessF() > 0.6;
        painter.setPen(bright ? QColor(0x0A0E15) : QColor(0xC9D2E0));
        QFont font = Theme::uiFont(7);
        painter.setFont(font);
        painter.drawText(r, Qt::AlignCenter, KeyLayout::displayName(key));
    }

    if (m_dragging) {
        painter.setBrush(withAlpha(Theme::Accent, 26));
        painter.setPen(QPen(withAlpha(Theme::Accent, 160), 1.0, Qt::DashLine));
        painter.drawRect(dragRect);
    }
}

void PerKeyBoard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    m_dragStart = event->position();
    m_dragCurrent = m_dragStart;
    m_dragging = false;
    const int index = keyAt(m_dragStart);
    if (index < 0)
        return;

    if (!(event->modifiers() & Qt::ControlModifier))
        m_selected.clear();
    if (m_selected.contains(index))
        m_selected.remove(index);
    else
        m_selected.insert(index);
    emit selectionChanged();
    update();
}

void PerKeyBoard::mouseMoveEvent(QMouseEvent *event)
{
    const int hover = keyAt(event->position());
    if (hover != m_hover) {
        m_hover = hover;
        update();
    }
    if (event->buttons() & Qt::LeftButton) {
        m_dragCurrent = event->position();
        if (!m_dragging
            && (m_dragCurrent - m_dragStart).manhattanLength() > 6.0) {
            m_dragging = true;
        }
        if (m_dragging)
            update();
    }
}

void PerKeyBoard::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    if (m_dragging) {
        const QRectF dragRect = QRectF(m_dragStart, event->position()).normalized();
        if (!(event->modifiers() & Qt::ControlModifier))
            m_selected.clear();
        for (int i = 0; i < KeyLayout::kKeyCount; ++i) {
            if (keyRect(KeyLayout::kKeys[i]).intersects(dragRect))
                m_selected.insert(KeyLayout::kKeys[i].index);
        }
        emit selectionChanged();
    }
    m_dragging = false;
    update();
}

void PerKeyBoard::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    if (m_hover >= 0) {
        m_hover = -1;
        update();
    }
}

Toast::Toast(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    m_effect = new QGraphicsOpacityEffect(this);
    m_effect->setOpacity(0.0);
    setGraphicsEffect(m_effect);

    m_fadeIn = new QPropertyAnimation(m_effect, "opacity", this);
    m_fadeIn->setDuration(180);
    m_fadeIn->setStartValue(0.0);
    m_fadeIn->setEndValue(1.0);

    m_fadeOut = new QPropertyAnimation(m_effect, "opacity", this);
    m_fadeOut->setDuration(220);
    m_fadeOut->setStartValue(1.0);
    m_fadeOut->setEndValue(0.0);
    connect(m_fadeOut, &QPropertyAnimation::finished, this, &QWidget::hide);

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, [this]() {
        m_fadeOut->stop();
        m_fadeOut->start();
    });

    hide();
}

void Toast::showMessage(const QString &text, bool ok)
{
    m_text = text;
    m_ok = ok;
    adjustSize();
    resize(sizeHint());
    if (parentWidget()) {
        move(parentWidget()->width() - width() - 26,
             parentWidget()->height() - height() - 26);
    }
    show();
    raise();
    m_fadeIn->stop();
    m_fadeOut->stop();
    m_effect->setOpacity(0.0);
    m_fadeIn->start();
    m_hideTimer->start(3400);
    update();
}

QSize Toast::sizeHint() const
{
    const QFontMetrics metrics(Theme::uiFont(13));
    return QSize(qMin(460, metrics.horizontalAdvance(m_text) + 64), 46);
}

void Toast::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QColor background = m_ok ? QColor(0x17, 0x33, 0x28) : QColor(0x3A, 0x1B, 0x25);
    const QColor border = m_ok ? QColor(0x2C, 0x6A, 0x50) : QColor(0x6A, 0x2B, 0x3A);
    painter.setPen(QPen(border, 1));
    painter.setBrush(background);
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 14, 14);

    const QPixmap icon = Theme::tintedIcon(
        m_ok ? QStringLiteral(":/assets/icons/check.svg")
             : QStringLiteral(":/assets/icons/close.svg"),
        m_ok ? Theme::Success : Theme::Danger, 16);
    painter.drawPixmap(QPointF(18, (height() - 16) / 2.0), icon);

    painter.setPen(Theme::Text);
    painter.setFont(Theme::uiFont(13));
    painter.drawText(QRectF(44, 0, width() - 58, height()),
                     Qt::AlignVCenter | Qt::AlignLeft, m_text);
}
