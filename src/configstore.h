#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVariant>

// 统一配置存储：~/.config/gs3108t-pro/config.json
// 键写成 "group/key" 的形式，落盘后是嵌套的 JSON 对象。
namespace ConfigStore {

QString filePath();

QVariant value(const QString &key, const QVariant &fallback = QVariant());
void setValue(const QString &key, const QVariant &value);

QJsonArray array(const QString &key);
void setArray(const QString &key, const QJsonArray &items);

QJsonObject object(const QString &key);
void setObject(const QString &key, const QJsonObject &item);

// 把内存中的配置写回磁盘，目录不存在时自动创建。
bool sync();

} // namespace ConfigStore
