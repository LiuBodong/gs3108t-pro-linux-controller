#pragma once

#include <QColor>
#include <QFont>
#include <QPixmap>
#include <QString>

namespace Theme {

inline const QColor Bg{0x0B, 0x0D, 0x12};
inline const QColor Bg2{0x12, 0x17, 0x22};
inline const QColor Panel{0x15, 0x1A, 0x24};
inline const QColor Panel2{0x1A, 0x20, 0x30};
inline const QColor Panel3{0x20, 0x28, 0x3A};
inline const QColor Border{0x25, 0x2D, 0x3D};
inline const QColor BorderSoft{0x1C, 0x23, 0x31};
inline const QColor Text{0xF4, 0xF7, 0xFC};
inline const QColor TextDim{0x8B, 0x95, 0xA9};
inline const QColor TextMuted{0x5E, 0x69, 0x7C};
inline const QColor Accent{0x6E, 0x8B, 0xFF};
inline const QColor Accent2{0xA0, 0x6B, 0xFF};
inline const QColor Success{0x3D, 0xDC, 0x97};
inline const QColor Danger{0xFF, 0x5F, 0x7E};
inline const QColor Warning{0xFF, 0xB8, 0x6B};
inline const QColor Track{0x23, 0x2A, 0x38};

QString uiFamily();
QString displayFamily();
QString monoFamily();

QFont uiFont(int pixelSize, QFont::Weight weight = QFont::Normal);
QFont displayFont(int pixelSize, QFont::Weight weight = QFont::DemiBold);
QFont monoFont(int pixelSize, QFont::Weight weight = QFont::Normal);

QString styleSheet();
QPixmap tintedIcon(const QString &resource, const QColor &color, int size);
QPixmap logoPixmap(int size);
QString rgba(const QColor &color, int alpha);
QString hex(const QColor &color);

} // namespace Theme
