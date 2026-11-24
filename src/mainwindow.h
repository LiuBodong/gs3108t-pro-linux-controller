#pragma once

#include <QFrame>
#include <QPoint>
#include <QVector>
#include <QWidget>

class KeyboardController;
class QLabel;
class QSizeGrip;
class QStackedWidget;
class QToolButton;
class NavButton;
class StatusChip;
class Toast;

class TitleBar : public QWidget {
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);

signals:
    void minimizeClicked();
    void maximizeClicked();
    void closeClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_dragging = false;
    QPoint m_dragOffset;
};

class Sidebar : public QFrame {
    Q_OBJECT

public:
    explicit Sidebar(KeyboardController *controller, QWidget *parent = nullptr);

    void setCurrentPage(int index);

signals:
    void pageChanged(int index);

private:
    void updateDeviceCard();

    KeyboardController *m_controller = nullptr;
    QVector<NavButton *> m_buttons;
    QLabel *m_deviceIcon = nullptr;
    QLabel *m_deviceTitle = nullptr;
    QLabel *m_deviceSub = nullptr;
    QLabel *m_deviceDot = nullptr;
};

class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(KeyboardController *controller, QWidget *parent = nullptr);
    void setPage(int index);
    void scrollPageToBottom();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void buildUi();
    void updateStatus();
    Qt::Edges edgesAt(const QPoint &pos) const;

    KeyboardController *m_controller = nullptr;
    TitleBar *m_titleBar = nullptr;
    Sidebar *m_sidebar = nullptr;
    QLabel *m_pageTitle = nullptr;
    QLabel *m_pageSubtitle = nullptr;
    StatusChip *m_statusChip = nullptr;
    QToolButton *m_refreshButton = nullptr;
    QStackedWidget *m_stack = nullptr;
    Toast *m_toast = nullptr;
    QSizeGrip *m_sizeGrip = nullptr;
};
