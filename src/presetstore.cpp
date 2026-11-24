#include "presetstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace PresetStore {

QString filePath()
{
    const QString base =
        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return base + QStringLiteral("/gs3108t-pro/presets.json");
}

QVector<LightPreset> load()
{
    QVector<LightPreset> presets;

    QFile file(filePath());
    if (!file.open(QIODevice::ReadOnly))
        return presets;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return presets;

    const QJsonArray items = doc.object().value(QStringLiteral("presets")).toArray();
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

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("presets"), items);

    const QString path = filePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) >= 0;
}

} // namespace PresetStore
