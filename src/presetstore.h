#pragma once

#include <QColor>
#include <QString>
#include <QVector>

// 一套完整的灯效参数，对应界面上的「我的专属配置」。
struct LightPreset {
    QString name;
    int mode = 8;
    QColor color = QColor(0xFF, 0x3B, 0x30);
    int brightness = 5;
    int speed = 3;
    int direction = 0;
    int colorful = 0;
};

namespace PresetStore {

// ~/.config/gs3108t-pro/presets.json
QString filePath();
QVector<LightPreset> load();
bool save(const QVector<LightPreset> &presets);

} // namespace PresetStore
