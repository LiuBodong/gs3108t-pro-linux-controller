#pragma once

#include <QColor>
#include <QDialog>
#include <QFrame>
#include <QImage>
#include <QPixmap>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "keylayout.h"

class QButtonGroup;
class QGraphicsOpacityEffect;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPropertyAnimation;
class QPushButton;
class QTimer;
class QVBoxLayout;

class CardFrame : public QFrame {
public:
    explicit CardFrame(QWidget *parent = nullptr);
    QVBoxLayout *body() const { return m_body; }

private:
    QVBoxLayout *m_body = nullptr;
};

class NavButton : public QWidget {
    Q_OBJECT

public:
    explicit NavButton(const QString &iconResource, const QString &text, QWidget *parent = nullptr);

    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }
    QSize sizeHint() const override;

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QString m_text;
    QPixmap m_iconDim;
    QPixmap m_iconActive;
    bool m_selected = false;
    bool m_hover = false;
};

class StatusChip : public QWidget {
    Q_OBJECT

public:
    explicit StatusChip(QWidget *parent = nullptr);

    void setStatus(bool connected, const QString &text, const QString &iconResource);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_text;
    QPixmap m_icon;
    bool m_connected = false;
    qreal m_phase = 0.0;
    QTimer *m_timer = nullptr;
};

class ModeChip : public QWidget {
    Q_OBJECT

public:
    explicit ModeChip(int mode, const QString &name, QWidget *parent = nullptr);

    int mode() const { return m_index; }
    void setSelected(bool selected);
    void setAccentColor(const QColor &color);
    QSize sizeHint() const override;

signals:
    void clicked(int mode);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int m_index = 0;
    QString m_name;
    bool m_selected = false;
    bool m_hover = false;
    QColor m_accent;
};

// 与主题一致的深色对话框，替代系统原生 QInputDialog / QMessageBox。
class AppDialog : public QDialog {
    Q_OBJECT

public:
    // 单行输入；用户取消时 accepted 为 false。
    static QString prompt(QWidget *parent, const QString &title, const QString &label,
                          const QString &initial, bool *accepted);
    // 确认框；返回 true 表示用户点了确认按钮。
    static bool confirm(QWidget *parent, const QString &title, const QString &text,
                        const QString &acceptText, const QString &cancelText);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    explicit AppDialog(QWidget *parent);
    void centerOnParent();
    // 开发验证用：设置 GSKBD_DIALOG_SHOT 后会自动截图并关闭对话框。
    void armAutoScreenshot();

    QPoint m_dragOffset;
};

class ColorWheel : public QWidget {
    Q_OBJECT

public:
    explicit ColorWheel(QWidget *parent = nullptr);

    qreal hue() const { return m_hue; }
    qreal saturation() const { return m_saturation; }
    qreal value() const { return m_value; }

    void setHsv(qreal hue, qreal saturation, qreal value);
    QSize sizeHint() const override;

signals:
    void hsvChanged(qreal hue, qreal saturation);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QRectF wheelRect() const;
    void ensureWheel();
    void pick(const QPointF &pos);

    qreal m_hue = 0.0;
    qreal m_saturation = 1.0;
    qreal m_value = 1.0;
    QImage m_wheel;
    QSize m_cacheSize;
    bool m_dragging = false;
};

class GlowSlider : public QWidget {
    Q_OBJECT

public:
    explicit GlowSlider(const QString &label, int minimum, int maximum, int value,
                        QWidget *parent = nullptr);

    int value() const { return m_value; }
    void setValue(int value);
    void setAccentColor(const QColor &color);
    QSize sizeHint() const override;

signals:
    void valueChanged(int value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setValueFromX(qreal x);

    QString m_label;
    int m_minimum = 0;
    int m_maximum = 5;
    int m_value = 0;
    QColor m_accent;
    bool m_dragging = false;
};

class SegmentedControl : public QWidget {
    Q_OBJECT

public:
    explicit SegmentedControl(const QStringList &labels, QWidget *parent = nullptr);

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);
    QSize sizeHint() const override;

signals:
    void currentChanged(int index);

private:
    QHBoxLayout *m_layout = nullptr;
    QButtonGroup *m_group = nullptr;
    QVector<QPushButton *> m_buttons;
    int m_currentIndex = 0;
};

class ToggleSwitch : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal knobPos READ knobPos WRITE setKnobPos)

public:
    explicit ToggleSwitch(QWidget *parent = nullptr);

    bool isChecked() const { return m_checked; }
    void setChecked(bool checked, bool animate = true);
    qreal knobPos() const { return m_knobPos; }
    void setKnobPos(qreal pos);
    QSize sizeHint() const override;

signals:
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_checked = false;
    qreal m_knobPos = 0.0;
    QPropertyAnimation *m_animation = nullptr;
};

class KeyboardPreview : public QWidget {
    Q_OBJECT

public:
    explicit KeyboardPreview(QWidget *parent = nullptr);

    void setColor(const QColor &color);
    void setBrightness(int brightness);
    void setSpeed(int speed);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_color{0xFF, 0x3B, 0x30};
    int m_brightness = 5;
    int m_speed = 3;
    qreal m_phase = 0.0;
    QTimer *m_timer = nullptr;
};

class PerKeyBoard : public QWidget {
    Q_OBJECT

public:
    explicit PerKeyBoard(QWidget *parent = nullptr);

    void setColors(const QVector<QColor> &colors);
    QVector<QColor> colors() const { return m_colors; }
    QVector<int> selectedKeys() const;
    bool hasSelection() const { return !m_selected.isEmpty(); }
    void clearSelection();
    void selectAll();
    void paintSelected(const QColor &color);
    QSize sizeHint() const override;

signals:
    void selectionChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRectF boardRect() const;
    QRectF keyRect(const KeyLayout::Key &key) const;
    int keyAt(const QPointF &pos) const;

    QVector<QColor> m_colors;
    QSet<int> m_selected;
    int m_hover = -1;
    bool m_dragging = false;
    QPointF m_dragStart;
    QPointF m_dragCurrent;
};

class Toast : public QWidget {
    Q_OBJECT

public:
    explicit Toast(QWidget *parent = nullptr);

    void showMessage(const QString &text, bool ok);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_text;
    bool m_ok = true;
    QGraphicsOpacityEffect *m_effect = nullptr;
    QPropertyAnimation *m_fadeIn = nullptr;
    QPropertyAnimation *m_fadeOut = nullptr;
    QTimer *m_hideTimer = nullptr;
};
