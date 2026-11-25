#include "presetstore.h"

#include "configstore.h"

#include <QJsonArray>
#include <QJsonObject>

namespace PresetStore {

QString filePath()
{
    return ConfigStore::filePath();
}

QVector<LightPreset> load()
{
    QVector<LightPreset> presets;

    const QJsonArray items = ConfigStore::array(QStringLiteral("presets"));
    presets.reserve(items.size());
    for (const QJsonValue &value : items) {
        const QJsonObject obj = value.toObject();
        LightPreset preset;
        preset.name = obj.value(QStringLiteral("name")).toString().trimmed();
        if (preset.name.isEmpty())
            continue;
        preset.mode = obj.value(QStringLiteral("mode")).toInt(preset.mode);
        preset.color = QColor(
            obj.value(QStringLiteral("color")).toString(QStringLiteral("#FF3B30")));
        if (!preset.color.isValid())
            preset.color = QColor(0xFF, 0x3B, 0x30);
        preset.brightness = obj.value(QStringLiteral("brightness")).toInt(preset.brightness);
        preset.speed = obj.value(QStringLiteral("speed")).toInt(preset.speed);
        preset.direction = obj.value(QStringLiteral("direction")).toInt(preset.direction);
        preset.colorful = obj.value(QStringLiteral("colorful")).toInt(preset.colorful);
        presets.append(preset);
    }
    return presets;
}

bool save(const QVector<LightPreset> &presets)
{
    QJsonArray items;
    for (const LightPreset &preset : presets) {
        QJsonObject obj;
        obj.insert(QStringLiteral("name"), preset.name);
        obj.insert(QStringLiteral("mode"), preset.mode);
        obj.insert(QStringLiteral("color"),
                   preset.color.name(QColor::HexRgb).toUpper());
        obj.insert(QStringLiteral("brightness"), preset.brightness);
        obj.insert(QStringLiteral("speed"), preset.speed);
        obj.insert(QStringLiteral("direction"), preset.direction);
        obj.insert(QStringLiteral("colorful"), preset.colorful);
        items.append(obj);
    }

    ConfigStore::setArray(QStringLiteral("presets"), items);
    return ConfigStore::sync();
}

} // namespace PresetStore
