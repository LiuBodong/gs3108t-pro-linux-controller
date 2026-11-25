#include "configstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>

namespace {

QJsonObject g_root;
bool g_loaded = false;
bool g_dirty = false;

QString configPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QStringLiteral("/gs3108t-pro/config.json");
}

QStringList splitKey(const QString &key)
{
    return key.split(QLatin1Char('/'), Qt::SkipEmptyParts);
}

QJsonValue toJson(const QVariant &value)
{
    return QJsonValue::fromVariant(value);
}

QJsonObject readConfigFile()
{
    QFile file(configPath());
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.isObject() ? doc.object() : QJsonObject();
}

// 把旧版 QSettings（~/.config/gs3108t/*.conf）和 presets.json 迁移到 config.json。
QJsonObject migrateLegacy()
{
    QJsonObject root;

    // 旧版用默认 QSettings() 写入，Linux 下是 org/app.conf。
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope,
                       QStringLiteral("gs3108t"),
                       QStringLiteral("GS3108T PRO 控制中心"));

    const QString mode = settings.value(QStringLiteral("connection/mode")).toString();
    if (!mode.isEmpty())
        root.insert(QStringLiteral("connection"),
                    QJsonObject{{QStringLiteral("mode"), mode}});

    QJsonObject system;
    if (settings.contains(QStringLiteral("system/response")))
        system.insert(QStringLiteral("response"), settings.value(QStringLiteral("system/response")).toInt());
    if (settings.contains(QStringLiteral("system/sleep")))
        system.insert(QStringLiteral("sleep"), settings.value(QStringLiteral("system/sleep")).toInt());
    if (settings.contains(QStringLiteral("system/disableWin")))
        system.insert(QStringLiteral("disableWin"), settings.value(QStringLiteral("system/disableWin")).toBool());
    if (settings.contains(QStringLiteral("system/disableAltF4")))
        system.insert(QStringLiteral("disableAltF4"), settings.value(QStringLiteral("system/disableAltF4")).toBool());
    if (settings.contains(QStringLiteral("system/disableAltTab")))
        system.insert(QStringLiteral("disableAltTab"), settings.value(QStringLiteral("system/disableAltTab")).toBool());
    if (!system.isEmpty())
        root.insert(QStringLiteral("system"), system);

    const int count = settings.value(QStringLiteral("perkey/count")).toInt();
    if (count > 0) {
        QJsonObject perKey;
        perKey.insert(QStringLiteral("count"), count);
        QJsonArray slotList;
        for (int slot = 0; slot < count; ++slot) {
            const QString name = settings.value(QStringLiteral("perkey/name%1").arg(slot)).toString();
            const QString text = settings.value(QStringLiteral("perkey/slot%1").arg(slot)).toString();

            QJsonObject colors;
            for (const QString &item : text.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
                const QStringList parts = item.split(QLatin1Char(':'));
                if (parts.size() != 2)
                    continue;
                bool ok = false;
                const int index = parts[0].toInt(&ok);
                if (!ok || index < 0)
                    continue;
                colors.insert(parts[0], parts[1].toUpper());
            }

            QJsonObject entry;
            entry.insert(QStringLiteral("name"),
                         name.isEmpty() ? QStringLiteral("方案 %1").arg(slot + 1) : name);
            entry.insert(QStringLiteral("colors"), colors);
            slotList.append(entry);
        }
        perKey.insert(QStringLiteral("slots"), slotList);
        root.insert(QStringLiteral("perKey"), perKey);
    }

    QFile presets(QFileInfo(configPath()).absolutePath() + QStringLiteral("/presets.json"));
    if (presets.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(presets.readAll());
        if (doc.isObject()) {
            const QJsonArray items = doc.object().value(QStringLiteral("presets")).toArray();
            if (!items.isEmpty())
                root.insert(QStringLiteral("presets"), items);
        }
    }

    return root;
}

void ensureLoaded()
{
    if (g_loaded)
        return;
    g_loaded = true;

    if (QFileInfo::exists(configPath())) {
        g_root = readConfigFile();
        return;
    }

    g_root = migrateLegacy();
    g_dirty = true;
    ConfigStore::sync();
}

QJsonValue lookup(const QStringList &path)
{
    QJsonObject current = g_root;
    for (int i = 0; i < path.size(); ++i) {
        const QJsonValue node = current.value(path.at(i));
        if (i == path.size() - 1)
            return node;
        if (!node.isObject())
            return QJsonValue::Undefined;
        current = node.toObject();
    }
    return QJsonValue::Undefined;
}

void insert(QJsonObject &target, const QStringList &path, int depth, const QJsonValue &value)
{
    if (depth == path.size() - 1) {
        target.insert(path.at(depth), value);
        return;
    }
    QJsonObject child = target.value(path.at(depth)).toObject();
    insert(child, path, depth + 1, value);
    target.insert(path.at(depth), child);
}

QVariant fromJson(const QJsonValue &value)
{
    return value.toVariant();
}

} // namespace

namespace ConfigStore {

QString filePath()
{
    return configPath();
}

QVariant value(const QString &key, const QVariant &fallback)
{
    ensureLoaded();
    const QStringList path = splitKey(key);
    if (path.isEmpty())
        return fallback;

    const QJsonValue found = lookup(path);
    return found.isUndefined() ? fallback : fromJson(found);
}

void setValue(const QString &key, const QVariant &value)
{
    ensureLoaded();
    const QStringList path = splitKey(key);
    if (path.isEmpty())
        return;
    insert(g_root, path, 0, toJson(value));
    g_dirty = true;
}

QJsonArray array(const QString &key)
{
    ensureLoaded();
    const QStringList path = splitKey(key);
    if (path.isEmpty())
        return {};
    return lookup(path).toArray();
}

void setArray(const QString &key, const QJsonArray &items)
{
    ensureLoaded();
    const QStringList path = splitKey(key);
    if (path.isEmpty())
        return;
    insert(g_root, path, 0, items);
    g_dirty = true;
}

QJsonObject object(const QString &key)
{
    ensureLoaded();
    const QStringList path = splitKey(key);
    if (path.isEmpty())
        return {};
    return lookup(path).toObject();
}

void setObject(const QString &key, const QJsonObject &item)
{
    ensureLoaded();
    const QStringList path = splitKey(key);
    if (path.isEmpty())
        return;
    insert(g_root, path, 0, item);
    g_dirty = true;
}

bool sync()
{
    ensureLoaded();
    if (!g_dirty)
        return true;

    const QString path = configPath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(g_root).toJson(QJsonDocument::Indented));
    if (!file.commit())
        return false;

    g_dirty = false;
    return true;
}

} // namespace ConfigStore
