#include "mainwindow.h"

#include "keyboardcontroller.h"
#include "pages.h"
#include "theme.h"
#include "widgets.h"

#include <QApplication>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QSizeGrip>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWindow>

namespace {

QToolButton *makeWindowButton(const QString &iconName, const QString &objectName,
                              QWidget *parent)
{
    auto *button = new QToolButton(parent);
    button->setObjectName(objectName);
    button->setFixedSize(36, 30);
    button->setCursor(Qt::PointingHandCursor);
    button->setIcon(QIcon(Theme::tintedIcon(QStringLiteral(":/assets/icons/") + iconName,
                                             Theme::TextDim, 14)));
    button->setIconSize(QSize(14, 14));
    return button;
}

} // namespace

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("titleBar"));
    setFixedHeight(52);
    setMouseTracking(true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(18, 0, 8, 0);
    layout->setSpacing(10);

    auto *logo = new QLabel;
    logo->setFixedSize(28, 28);
    logo->setPixmap(Theme::logoPixmap(28));
    layout->addWidget(logo, 0, Qt::AlignVCenter);

    auto *brand = new QLabel(QStringLiteral("GS3108T PRO"));
    brand->setFont(Theme::displayFont(14, QFont::DemiBold));
    brand->setStyleSheet(QStringLiteral("color: #F4F7FC;"));
    layout->addWidget(brand, 0, Qt::AlignVCenter);

    auto *sub = new QLabel(QStringLiteral("控制中心"));
    sub->setFont(Theme::uiFont(13));
    sub->setStyleSheet(QStringLiteral("color: #5E697C;"));
    layout->addWidget(sub, 0, Qt::AlignVCenter);

    layout->addStretch();

    auto *minimize = makeWindowButton(QStringLiteral("minimize.svg"), QStringLiteral("windowButton"), this);
    auto *maximize = makeWindowButton(QStringLiteral("maximize.svg"), QStringLiteral("windowButton"), this);
    auto *close = makeWindowButton(QStringLiteral("close.svg"), QStringLiteral("closeButton"), this);
    layout->addWidget(minimize, 0, Qt::AlignVCenter);
    layout->addWidget(maximize, 0, Qt::AlignVCenter);
    layout->addWidget(close, 0, Qt::AlignVCenter);

    connect(minimize, &QToolButton::clicked, this, &TitleBar::minimizeClicked);
    connect(maximize, &QToolButton::clicked, this, &TitleBar::maximizeClicked);
    connect(close, &QToolButton::clicked, this, &TitleBar::closeClicked);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (auto *handle = window()->windowHandle(); handle && handle->startSystemMove()) {
            m_dragging = false;
            return;
        }
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint()
            - window()->frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton))
        window()->move(event->globalPosition().toPoint() - m_dragOffset);
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}

Sidebar::Sidebar(KeyboardController *controller, QWidget *parent)
    : QFrame(parent)
    , m_controller(controller)
{
    setObjectName(QStringLiteral("sidebar"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(236);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(4);

    struct NavItem {
        const char *icon;
        const char *label;
    };
    const NavItem items[] = {
        {":/assets/icons/light.svg", "灯光控制"},
        {":/assets/icons/brightness.svg", "用户灯光"},
        {":/assets/icons/clock.svg", "TFT 时钟"},
        {":/assets/icons/palette.svg", "TFT 动图"},
        {":/assets/icons/keyboard.svg", "系统配置"},
        {":/assets/icons/device.svg", "设备与连接"},
        {":/assets/icons/about.svg", "关于"}
    };

    for (int i = 0; i < 7; ++i) {
        auto *button = new NavButton(QString::fromLatin1(items[i].icon),
                                     QString::fromUtf8(items[i].label));
        connect(button, &NavButton::clicked, this, [this, i]() {
            setCurrentPage(i);
            emit pageChanged(i);
        });
        layout->addWidget(button);
        m_buttons.append(button);
    }

    layout->addStretch();

    auto *deviceCard = new QFrame;
    deviceCard->setObjectName(QStringLiteral("deviceCard"));
    deviceCard->setAttribute(Qt::WA_StyledBackground, true);
    deviceCard->setFixedHeight(76);

    auto *deviceLayout = new QHBoxLayout(deviceCard);
    deviceLayout->setContentsMargins(12, 12, 12, 12);
    deviceLayout->setSpacing(10);

    m_deviceIcon = new QLabel;
    m_deviceIcon->setFixedSize(30, 30);
    m_deviceIcon->setAlignment(Qt::AlignCenter);
    deviceLayout->addWidget(m_deviceIcon, 0, Qt::AlignVCenter);

    auto *texts = new QVBoxLayout;
    texts->setSpacing(1);
    m_deviceTitle = new QLabel;
    m_deviceTitle->setFont(Theme::uiFont(13, QFont::DemiBold));
    m_deviceTitle->setStyleSheet(QStringLiteral("color: #F4F7FC;"));
    m_deviceSub = new QLabel;
    m_deviceSub->setFont(Theme::monoFont(11));
    m_deviceSub->setStyleSheet(QStringLiteral("color: #5E697C;"));
    texts->addWidget(m_deviceTitle);
    texts->addWidget(m_deviceSub);
    deviceLayout->addLayout(texts, 1);

    m_deviceDot = new QLabel;
    m_deviceDot->setFixedSize(7, 7);
    deviceLayout->addWidget(m_deviceDot, 0, Qt::AlignVCenter);

    layout->addWidget(deviceCard);

    auto *version = new QLabel(QStringLiteral("v0.2.0"));
    version->setAlignment(Qt::AlignHCenter);
    version->setFont(Theme::monoFont(11));
    version->setStyleSheet(QStringLiteral("color: #5E697C;"));
    version->setContentsMargins(0, 4, 0, 0);
    layout->addWidget(version);

    connect(m_controller, &KeyboardController::stateChanged, this,
            &Sidebar::updateDeviceCard);
    updateDeviceCard();
}

void Sidebar::setCurrentPage(int index)
{
    for (int i = 0; i < m_buttons.size(); ++i)
        m_buttons.at(i)->setSelected(i == index);
}

void Sidebar::updateDeviceCard()
{
    const bool connected = m_controller->connected();
    const bool wireless = m_controller->connectionType() == QStringLiteral("wireless");

    m_deviceIcon->setPixmap(Theme::tintedIcon(
        wireless ? QStringLiteral(":/assets/icons/wireless.svg")
                 : QStringLiteral(":/assets/icons/usb.svg"),
        connected ? Theme::Success : Theme::Danger, 15));
    m_deviceIcon->setStyleSheet(
        QStringLiteral("background-color: %1; border-radius: 9px;")
            .arg(connected ? QStringLiteral("#173328") : QStringLiteral("#241B1B")));

    m_deviceTitle->setText(connected ? m_controller->connectionLabel()
                                     : QStringLiteral("未连接"));
    m_deviceSub->setText(connected ? m_controller->deviceNode()
                                   : QStringLiteral("请连接键盘"));
    m_deviceDot->setStyleSheet(
        QStringLiteral("background-color: %1; border-radius: 3px;")
            .arg(connected ? Theme::Success.name() : Theme::Danger.name()));
}

MainWindow::MainWindow(KeyboardController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
{
    setWindowTitle(QStringLiteral("GS3108T PRO 控制中心"));
    resize(1240, 800);
    setMinimumSize(1080, 700);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    if (auto *screen = QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        move(available.center() - QPoint(width() / 2, height() / 2));
    }

    buildUi();

    connect(m_controller, &KeyboardController::stateChanged, this, &MainWindow::updateStatus);
    connect(m_controller, &KeyboardController::lightApplied, this,
            [this](bool ok, const QString &message) {
                m_toast->showMessage(message, ok);
            });
    connect(m_controller, &KeyboardController::perKeyApplied, this,
            [this](bool ok, const QString &message) {
                m_toast->showMessage(message, ok);
            });
    connect(m_controller, &KeyboardController::systemConfigApplied, this,
            [this](bool ok, const QString &message) {
                m_toast->showMessage(message, ok);
            });
    connect(m_controller, &KeyboardController::resetApplied, this,
            [this](bool ok, const QString &message, bool) {
                m_toast->showMessage(message, ok);
            });
    connect(m_controller, &KeyboardController::gifUploaded, this,
            [this](bool ok, const QString &message) {
                m_toast->showMessage(message, ok);
            });
    connect(m_controller, &KeyboardController::clockSynced, this,
            [this](bool ok, const QString &message) {
                m_toast->showMessage(message, ok);
            });

    updateStatus();
    setPage(0);
}

void MainWindow::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_titleBar = new TitleBar(this);
    root->addWidget(m_titleBar);

    auto *body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    m_sidebar = new Sidebar(m_controller, this);
    body->addWidget(m_sidebar);

    auto *content = new QWidget;
    content->setObjectName(QStringLiteral("content"));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(26, 8, 26, 20);
    contentLayout->setSpacing(14);

    auto *header = new QWidget;
    header->setFixedHeight(64);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(14);

    auto *titles = new QVBoxLayout;
    titles->setSpacing(2);
    m_pageTitle = new QLabel;
    m_pageTitle->setObjectName(QStringLiteral("pageTitle"));
    m_pageTitle->setFont(Theme::displayFont(24, QFont::DemiBold));
    m_pageTitle->setStyleSheet(QStringLiteral("color: #F4F7FC;"));
    m_pageSubtitle = new QLabel;
    m_pageSubtitle->setObjectName(QStringLiteral("pageSubtitle"));
    m_pageSubtitle->setFont(Theme::uiFont(13));
    m_pageSubtitle->setStyleSheet(QStringLiteral("color: #8B95A9;"));
    titles->addWidget(m_pageTitle);
    titles->addWidget(m_pageSubtitle);
    headerLayout->addLayout(titles, 1);

    m_statusChip = new StatusChip;
    headerLayout->addWidget(m_statusChip, 0, Qt::AlignVCenter);

    m_refreshButton = new QToolButton;
    m_refreshButton->setFixedSize(36, 36);
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    m_refreshButton->setToolTip(QStringLiteral("重新扫描 HID 接口"));
    m_refreshButton->setIcon(QIcon(Theme::tintedIcon(
        QStringLiteral(":/assets/icons/refresh.svg"), Theme::TextDim, 15)));
    m_refreshButton->setIconSize(QSize(15, 15));
    m_refreshButton->setStyleSheet(
        QStringLiteral("QToolButton { background-color: #141A25; border: 1px solid #252D3D; "
                       "border-radius: 11px; } "
                       "QToolButton:hover { background-color: #1B2230; }"));
    connect(m_refreshButton, &QToolButton::clicked, m_controller, &KeyboardController::refresh);
    headerLayout->addWidget(m_refreshButton, 0, Qt::AlignVCenter);

    contentLayout->addWidget(header);

    m_stack = new QStackedWidget;
    m_stack->addWidget(new LightPage(m_controller));
    m_stack->addWidget(new UserLightPage(m_controller));
    m_stack->addWidget(new ClockPage(m_controller));
    m_stack->addWidget(new GifPage(m_controller));
    m_stack->addWidget(new SystemPage(m_controller));
    m_stack->addWidget(new DevicePage(m_controller));
    m_stack->addWidget(new AboutPage);
    contentLayout->addWidget(m_stack, 1);

    body->addWidget(content, 1);
    root->addLayout(body, 1);

    connect(m_sidebar, &Sidebar::pageChanged, this, &MainWindow::setPage);
    connect(m_titleBar, &TitleBar::minimizeClicked, this, &QWidget::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeClicked, this, [this]() {
        if (isMaximized())
            showNormal();
        else
            showMaximized();
    });
    connect(m_titleBar, &TitleBar::closeClicked, this, &QWidget::close);

    m_toast = new Toast(this);
    m_toast->hide();

    m_sizeGrip = new QSizeGrip(this);
    m_sizeGrip->setFixedSize(16, 16);
    m_sizeGrip->setStyleSheet(QStringLiteral("background: transparent;"));
    m_sizeGrip->raise();

    auto *quitShortcut = new QShortcut(QKeySequence::Quit, this);
    connect(quitShortcut, &QShortcut::activated, this, &QWidget::close);
}

void MainWindow::scrollPageToBottom()
{
    auto *page = qobject_cast<QScrollArea *>(m_stack->currentWidget());
    if (!page)
        return;
    QScrollBar *bar = page->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void MainWindow::setPage(int index)
{
    static const QStringList titles = {
        QStringLiteral("灯光控制"),
        QStringLiteral("用户灯光"),
        QStringLiteral("TFT 时钟"),
        QStringLiteral("TFT 动图"),
        QStringLiteral("系统配置"),
        QStringLiteral("设备与连接"),
        QStringLiteral("关于")
    };
    static const QStringList subtitles = {
        QStringLiteral("实时调整键盘背光"),
        QStringLiteral("逐键 RGB 与多组方案"),
        QStringLiteral("把系统时间写入键盘 TFT 屏"),
        QStringLiteral("把 GIF 动画写进键盘的 TFT 屏"),
        QStringLiteral("按键响应、休眠时间与禁用组合键"),
        QStringLiteral("连接方式与 HID 接口"),
        QStringLiteral("版本信息与功能进度")
    };

    index = qBound(0, index, titles.size() - 1);
    m_stack->setCurrentIndex(index);
    m_pageTitle->setText(titles.at(index));
    m_pageSubtitle->setText(subtitles.at(index));
    m_sidebar->setCurrentPage(index);
}

void MainWindow::updateStatus()
{
    const bool connected = m_controller->connected();
    const QString icon = m_controller->connectionType() == QStringLiteral("wireless")
                             ? QStringLiteral(":/assets/icons/wireless.svg")
                             : QStringLiteral(":/assets/icons/usb.svg");
    m_statusChip->setStatus(connected, m_controller->connectionLabel(), icon);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal radius = isMaximized() ? 0.0 : 18.0;
    QPainterPath path;
    path.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

    painter.save();
    painter.setClipPath(path);

    QLinearGradient background(0, 0, 0, height());
    background.setColorAt(0.0, Theme::Bg);
    background.setColorAt(1.0, Theme::Bg2);
    painter.fillPath(path, background);

    QRadialGradient leftGlow(QPointF(-40, -60), 480);
    leftGlow.setColorAt(0.0, QColor(0x6E, 0x8B, 0xFF, 40));
    leftGlow.setColorAt(1.0, QColor(0x6E, 0x8B, 0xFF, 0));
    painter.fillRect(rect(), leftGlow);

    QRadialGradient rightGlow(QPointF(width() + 60, height() + 80), 420);
    rightGlow.setColorAt(0.0, QColor(0xA0, 0x6B, 0xFF, 36));
    rightGlow.setColorAt(1.0, QColor(0xA0, 0x6B, 0xFF, 0));
    painter.fillRect(rect(), rightGlow);

    painter.restore();

    painter.setPen(QPen(QColor(255, 255, 255, 18), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_toast && m_toast->isVisible()) {
        m_toast->move((width() - m_toast->width()) / 2,
                      height() - m_toast->height() - 26);
    }
    if (m_sizeGrip)
        m_sizeGrip->move(width() - m_sizeGrip->width(), height() - m_sizeGrip->height());
    update();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (m_sizeGrip)
            m_sizeGrip->setVisible(!isMaximized());
        update();
    }
    QWidget::changeEvent(event);
}

Qt::Edges MainWindow::edgesAt(const QPoint &pos) const
{
    constexpr int margin = 6;
    Qt::Edges edges;
    if (pos.x() <= margin)
        edges |= Qt::LeftEdge;
    if (pos.x() >= width() - margin)
        edges |= Qt::RightEdge;
    if (pos.y() <= margin)
        edges |= Qt::TopEdge;
    if (pos.y() >= height() - margin)
        edges |= Qt::BottomEdge;
    return edges;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const Qt::Edges edges = edgesAt(event->position().toPoint());
        if (edges != Qt::Edges()) {
            if (auto *handle = windowHandle())
                handle->startSystemResize(edges);
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    const Qt::Edges edges = edgesAt(event->position().toPoint());
    if (edges == Qt::Edges()) {
        unsetCursor();
    } else if ((edges & Qt::LeftEdge) && (edges & Qt::TopEdge)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if ((edges & Qt::RightEdge) && (edges & Qt::BottomEdge)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if ((edges & Qt::RightEdge) && (edges & Qt::TopEdge)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if ((edges & Qt::LeftEdge) && (edges & Qt::BottomEdge)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (edges & (Qt::LeftEdge | Qt::RightEdge)) {
        setCursor(Qt::SizeHorCursor);
    } else {
        setCursor(Qt::SizeVerCursor);
    }
    QWidget::mouseMoveEvent(event);
}

void MainWindow::leaveEvent(QEvent *event)
{
    unsetCursor();
    QWidget::leaveEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Q && (event->modifiers() & Qt::ControlModifier)) {
        close();
        return;
    }
    QWidget::keyPressEvent(event);
}
