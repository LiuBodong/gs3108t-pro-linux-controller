#include "theme.h"

#include <QFile>
#include <QFontDatabase>
#include <QLinearGradient>
#include <QPainter>
#include <QSvgRenderer>

namespace Theme {

namespace {

bool hasFamily(const QStringList &families, const QString &name)
{
    for (const QString &family : families) {
        if (family.compare(name, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

QString pickFamily(const QStringList &candidates, const QString &fallback)
{
    const QStringList families = QFontDatabase::families();
    for (const QString &candidate : candidates) {
        if (hasFamily(families, candidate))
            return candidate;
    }
    return fallback;
}

} // namespace

QString uiFamily()
{
    static const QString family = pickFamily(
        {QStringLiteral("WenQuanYi Micro Hei"),
         QStringLiteral("Noto Sans CJK SC"),
         QStringLiteral("Source Han Sans SC")},
        QStringLiteral("Sans Serif"));
    return family;
}

QString displayFamily()
{
    static const QString family = pickFamily(
        {QStringLiteral("LXGW WenKai"),
         QStringLiteral("Noto Serif CJK SC"),
         QStringLiteral("Source Han Serif SC")},
        uiFamily());
    return family;
}

QString monoFamily()
{
    static const QString family = pickFamily(
        {QStringLiteral("Maple Mono NF CN"),
         QStringLiteral("JetBrains Mono"),
         QStringLiteral("DejaVu Sans Mono")},
        QStringLiteral("Monospace"));
    return family;
}

QFont uiFont(int pixelSize, QFont::Weight weight)
{
    QFont font(uiFamily());
    font.setPixelSize(pixelSize);
    font.setWeight(weight);
    return font;
}

QFont displayFont(int pixelSize, QFont::Weight weight)
{
    QFont font(displayFamily());
    font.setPixelSize(pixelSize);
    font.setWeight(weight);
    return font;
}

QFont monoFont(int pixelSize, QFont::Weight weight)
{
    QFont font(monoFamily());
    font.setPixelSize(pixelSize);
    font.setWeight(weight);
    return font;
}

QString styleSheet()
{
    QFile file(QStringLiteral(":/assets/style.qss"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}

QPixmap logoPixmap(int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QLinearGradient gradient(0, 0, size, size);
    gradient.setColorAt(0.0, Accent);
    gradient.setColorAt(1.0, Accent2);
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawRoundedRect(QRectF(0, 0, size, size), size * 0.26, size * 0.26);

    const int iconSize = qRound(size * 0.48);
    const QPixmap icon = tintedIcon(QStringLiteral(":/assets/icons/keyboard.svg"),
                                    Qt::white, iconSize);
    painter.drawPixmap(QPointF((size - iconSize) / 2.0, (size - iconSize) / 2.0), icon);
    return pixmap;
}

QPixmap tintedIcon(const QString &resource, const QColor &color, int size)
{
    QSvgRenderer renderer(resource);
    if (!renderer.isValid())
        return {};

    constexpr qreal scale = 2.0;
    const int px = qRound(size * scale);

    QPixmap source(px, px);
    source.fill(Qt::transparent);
    {
        QPainter painter(&source);
        painter.setRenderHint(QPainter::Antialiasing);
        renderer.render(&painter, QRectF(0, 0, px, px));
    }

    QPixmap tinted(px, px);
    tinted.fill(Qt::transparent);
    {
        QPainter painter(&tinted);
        painter.drawPixmap(0, 0, source);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(QRect(0, 0, px, px), color);
    }

    // 绘制完成后再标记 DPR，否则 QPainter 会按逻辑尺寸缩放坐标系，只画出左上角一块。
    source.setDevicePixelRatio(scale);
    tinted.setDevicePixelRatio(scale);
    return tinted;
}

QString rgba(const QColor &color, int alpha)
{
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(alpha);
}

QString hex(const QColor &color)
{
    return color.name(QColor::HexRgb).toUpper();
}

} // namespace Theme
