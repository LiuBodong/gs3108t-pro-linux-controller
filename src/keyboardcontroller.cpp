#include "keyboardcontroller.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QEventLoop>
#include <QSettings>

static QString joinSizes(const QList<int> &sizes)
{
    QStringList parts;
    for (int size : sizes)
        parts << QString::number(size);
    return parts.isEmpty() ? QStringLiteral("-") : parts.join(QStringLiteral("/"));
}

KeyboardController::KeyboardController(QObject *parent)
    : QObject(parent)
{
    QSettings settings;
    m_preferredMode = settings.value(QStringLiteral("connection/mode"),
                                     QStringLiteral("auto"))
                          .toString();
    if (m_preferredMode != QStringLiteral("auto")
        && m_preferredMode != QStringLiteral("usb")
        && m_preferredMode != QStringLiteral("wireless")) {
        m_preferredMode = QStringLiteral("auto");
    }
    refresh();
}

QString KeyboardController::connectionLabel() const
{
    if (!m_hasActive)
        return QStringLiteral("未连接");
    return m_active.typeLabel();
}

QString KeyboardController::connectionType() const
{
    if (!m_hasActive)
        return QStringLiteral("none");
    if (m_active.isWireless())
        return QStringLiteral("wireless");
    if (m_active.isUsb())
        return QStringLiteral("usb");
    return QStringLiteral("none");
}

QVariantList KeyboardController::devices() const
{
    QVariantList list;
    for (const HidInterface &dev : m_devices) {
        QVariantMap item;
        item.insert(QStringLiteral("node"), dev.node);
        item.insert(QStringLiteral("name"), dev.name);
        item.insert(QStringLiteral("type"), dev.typeLabel());
        item.insert(QStringLiteral("usagePage"),
                    QStringLiteral("0x")
                        + QString::number(dev.usagePage, 16).toUpper().rightJustified(
                            4, QLatin1Char('0')));
        item.insert(QStringLiteral("feature"), joinSizes(dev.featureSizes));
        item.insert(QStringLiteral("output"), joinSizes(dev.outputSizes));
        item.insert(QStringLiteral("input"), joinSizes(dev.inputSizes));
        item.insert(QStringLiteral("active"), dev.node == m_active.node);
        list.append(item);
    }
    return list;
}

void KeyboardController::setPreferredMode(const QString &mode)
{
    if (mode == m_preferredMode)
        return;
    if (mode != QStringLiteral("auto")
        && mode != QStringLiteral("usb")
        && mode != QStringLiteral("wireless")) {
        return;
    }
    setPreferredModeTransient(mode);
    QSettings settings;
    settings.setValue(QStringLiteral("connection/mode"), mode);
}

void KeyboardController::setPreferredModeTransient(const QString &mode)
{
    if (mode == m_preferredMode)
        return;
    if (mode != QStringLiteral("auto")
        && mode != QStringLiteral("usb")
        && mode != QStringLiteral("wireless")) {
        return;
    }
    m_preferredMode = mode;
    emit preferredModeChanged();
    refresh();
}

void KeyboardController::refresh()
{
    m_devices = HidManager::scan();

    HidInterface usb;
    HidInterface wireless;
    for (const HidInterface &dev : m_devices) {
        if (dev.isUsb() && usb.node.isEmpty())
            usb = dev;
        if (dev.isWireless() && wireless.node.isEmpty())
            wireless = dev;
    }

    HidInterface chosen;
    if (m_preferredMode == QStringLiteral("usb")) {
        chosen = usb;
    } else if (m_preferredMode == QStringLiteral("wireless")) {
        chosen = wireless;
    } else if (!usb.node.isEmpty()) {
        chosen = usb;
    } else {
        chosen = wireless;
    }

    m_active = chosen;
    m_hasActive = !chosen.node.isEmpty();

    if (!m_hasActive) {
        if (m_preferredMode == QStringLiteral("usb")) {
            m_lastError = QStringLiteral("未找到 USB 有线接口。请确认键盘已用 USB 线连接。");
        } else if (m_preferredMode == QStringLiteral("wireless")) {
            m_lastError = QStringLiteral("未找到 2.4G 接口。请确认接收器已插入，且键盘已切换到 2.4G 模式（FN+4）。");
        } else {
            m_lastError = QStringLiteral("未找到键盘。请连接 USB 线，或插入 2.4G 接收器并确认键盘处于 2.4G 模式。");
        }
    } else {
        m_lastError.clear();
    }

    emit stateChanged();
}

bool KeyboardController::ensureActive()
{
    if (!m_hasActive)
        refresh();
    if (!m_hasActive) {
        if (m_lastError.isEmpty())
            m_lastError = QStringLiteral("未找到键盘。");
        return false;
    }
    return true;
}

void KeyboardController::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit stateChanged();
}

void KeyboardController::fail(const QString &message)
{
    m_lastError = message;
    emit stateChanged();
}

bool KeyboardController::applyLight(int mode, int r, int g, int b,
                                    int brightness, int speed,
                                    int direction, int colorful)
{
    if (m_busy)
        return false;

    setBusy(true);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    bool ok = false;
    if (!ensureActive()) {
        ok = false;
    } else if (m_active.isWireless()) {
        ok = sendLightWireless(mode, r, g, b, brightness, speed, direction, colorful);
    } else {
        ok = sendLightUsb(mode, r, g, b, brightness, speed, direction, colorful);
    }

    setBusy(false);
    const QString message = ok
        ? QStringLiteral("灯光已应用")
        : (m_lastError.isEmpty() ? QStringLiteral("灯光设置失败") : m_lastError);
    emit lightApplied(ok, message);
    return ok;
}

bool KeyboardController::sendLightUsb(int mode, int r, int g, int b,
                                      int brightness, int speed,
                                      int direction, int colorful)
{
    QString error;
    const int fd = HidManager::openDevice(m_active, &error);
    if (fd < 0) {
        fail(error);
        return false;
    }

    auto sendControl = [&](const QByteArray &data, const QString &label) -> bool {
        QByteArray response;
        if (!HidManager::sendFeature(fd, data, &response, &error)) {
            fail(label + QStringLiteral("：") + error);
            return false;
        }
        if (response.size() >= 5 && static_cast<quint8>(response.at(4)) != 1) {
            if (qEnvironmentVariableIsSet("GSKBD_DEBUG"))
                qWarning().noquote() << label << "unexpected ACK:" << response.toHex();
        }
        return true;
    };

    bool ok = true;
    QByteArray packet(64, '\0');

    packet[0] = 0x04;
    packet[1] = 0x18;
    if (!sendControl(packet, QStringLiteral("进入设置会话")))
        ok = false;

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x04;
        packet[1] = 0x13;
        packet[8] = 0x01;
        if (!sendControl(packet, QStringLiteral("准备写入灯光")))
            ok = false;
    }

    if (ok) {
        packet.fill('\0');
        packet[0] = static_cast<char>(mode & 0xFF);
        packet[1] = static_cast<char>(r & 0xFF);
        packet[2] = static_cast<char>(g & 0xFF);
        packet[3] = static_cast<char>(b & 0xFF);
        packet[8] = static_cast<char>(colorful & 0xFF);
        packet[9] = static_cast<char>(brightness & 0xFF);
        packet[10] = static_cast<char>(speed & 0xFF);
        packet[11] = static_cast<char>(direction & 0xFF);
        packet[14] = static_cast<char>(0xAA);
        packet[15] = static_cast<char>(0x55);

        QByteArray response;
        if (!HidManager::sendFeature(fd, packet, &response, &error)) {
            fail(QStringLiteral("写入灯光数据：") + error);
            ok = false;
        }
    }

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x04;
        packet[1] = 0x02;
        if (!sendControl(packet, QStringLiteral("结束设置会话")))
            ok = false;
    }

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x04;
        packet[1] = static_cast<char>(0xF0);
        if (!HidManager::sendFeature(fd, packet, nullptr, &error)) {
            fail(QStringLiteral("保存灯光设置：") + error);
            ok = false;
        }
    }

    HidManager::closeDevice(fd);
    return ok;
}

bool KeyboardController::sendLightWireless(int mode, int r, int g, int b,
                                           int brightness, int speed,
                                           int direction, int colorful)
{
    QByteArray frame(33, '\0');
    frame[1] = 0x05;
    frame[2] = 0x10;
    frame[3] = 0x00;
    frame[4] = static_cast<char>(mode & 0xFF);
    frame[5] = static_cast<char>(r & 0xFF);
    frame[6] = static_cast<char>(g & 0xFF);
    frame[7] = static_cast<char>(b & 0xFF);
    frame[12] = static_cast<char>(colorful & 0xFF);
    frame[13] = static_cast<char>(brightness & 0xFF);
    frame[14] = static_cast<char>(speed & 0xFF);
    frame[15] = static_cast<char>(direction & 0xFF);
    frame[18] = static_cast<char>(0xAA);
    frame[19] = static_cast<char>(0x55);

    quint8 sum = 0;
    for (int i = 0; i < 32; ++i)
        sum = static_cast<quint8>(sum + static_cast<quint8>(frame.at(i)));
    frame[32] = static_cast<char>(sum);

    QString error;
    const int fd = HidManager::openDevice(m_active, &error);
    if (fd < 0) {
        fail(error);
        return false;
    }

    const bool ok = HidManager::sendOutput(fd, frame, nullptr, 0, &error);
    HidManager::closeDevice(fd);
    if (!ok)
        fail(error);
    return ok;
}

bool KeyboardController::applySystemConfig(int response, int sleep,
                                             bool disableWin, bool disableAltF4,
                                             bool disableAltTab)
{
    if (m_busy)
        return false;

    setBusy(true);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    bool ok = false;
    if (!ensureActive()) {
        ok = false;
    } else if (m_active.isWireless()) {
        fail(QStringLiteral("系统配置目前只能通过 USB 有线写入。"
                            "请插上 USB 线，并确认键盘处于有线模式。"));
    } else {
        ok = sendSystemConfigUsb(response, sleep, disableWin, disableAltF4,
                                 disableAltTab);
    }

    setBusy(false);
    const QString message = ok
        ? QStringLiteral("系统配置已应用")
        : (m_lastError.isEmpty() ? QStringLiteral("系统配置写入失败") : m_lastError);
    emit systemConfigApplied(ok, message);
    return ok;
}

bool KeyboardController::sendSystemConfigUsb(int response, int sleep,
                                             bool disableWin, bool disableAltF4,
                                             bool disableAltTab)
{
    QString error;
    const int fd = HidManager::openDevice(m_active, &error);
    if (fd < 0) {
        fail(error);
        return false;
    }

    auto sendControl = [&](const QByteArray &data, const QString &label) -> bool {
        QByteArray reply;
        if (!HidManager::sendFeature(fd, data, &reply, &error)) {
            fail(label + QStringLiteral("：") + error);
            return false;
        }
        if (reply.size() >= 5 && static_cast<quint8>(reply.at(4)) != 1) {
            fail(label + QStringLiteral("被键盘拒绝（回执 %1）。"
                                        "请按一下键盘唤醒，确认处于有线模式后再试。")
                          .arg(QString::fromLatin1(reply.toHex())));
            return false;
        }
        return true;
    };

    bool ok = true;
    QByteArray packet(64, '\0');

    packet[0] = 0x04;
    packet[1] = 0x18;
    if (!sendControl(packet, QStringLiteral("进入设置会话")))
        ok = false;

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x04;
        packet[1] = 0x17;
        packet[2] = 0x01;
        packet[8] = 0x01;
        if (!sendControl(packet, QStringLiteral("准备写入系统配置")))
            ok = false;
    }

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x00;
        packet[1] = 0x01;
        packet[2] = static_cast<char>(disableAltTab ? 1 : 0);
        packet[3] = static_cast<char>(disableAltF4 ? 1 : 0);
        packet[4] = static_cast<char>(disableWin ? 1 : 0);
        packet[5] = 0x00;
        packet[6] = static_cast<char>(qBound(0, sleep, 3));
        packet[7] = 0x00;
        packet[8] = static_cast<char>(qBound(1, response, 5));
        packet[62] = static_cast<char>(0xAA);
        packet[63] = static_cast<char>(0x55);

        QByteArray reply;
        if (!HidManager::sendFeature(fd, packet, &reply, &error)) {
            fail(QStringLiteral("写入系统配置：") + error);
            ok = false;
        }
    }

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x04;
        packet[1] = 0x02;
        if (!sendControl(packet, QStringLiteral("结束设置会话")))
            ok = false;
    }

    HidManager::closeDevice(fd);
    return ok;
}

bool KeyboardController::resetKeyboard(bool factoryReset)
{
    if (m_busy)
        return false;

    setBusy(true);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    bool ok = false;
    if (!ensureActive()) {
        ok = false;
    } else if (m_active.isWireless()) {
        fail(QStringLiteral("重置和恢复出厂目前只能通过 USB 有线执行。"
                            "请插上 USB 线，并确认键盘处于有线模式。"));
    } else {
        ok = true;
        if (factoryReset && ok)
            ok = clearPerKeyUsb();
        if (ok)
            ok = clearLightStoreUsb(0x11);
        if (ok)
            ok = clearLightStoreUsb(0x27);
        if (factoryReset && ok)
            ok = applyDefaultLightUsb();
        if (factoryReset && ok)
            ok = sendSystemConfigUsb(2, 2, false, false, false);
    }

    setBusy(false);
    QString message;
    if (ok) {
        message = factoryReset ? QStringLiteral("键盘已恢复出厂设置")
                               : QStringLiteral("键盘数据已重置");
    } else {
        message = m_lastError.isEmpty()
            ? (factoryReset ? QStringLiteral("恢复出厂失败")
                            : QStringLiteral("重置键盘失败"))
            : m_lastError;
    }
    emit resetApplied(ok, message, factoryReset);
    return ok;
}

bool KeyboardController::clearLightStoreUsb(int command)
{
    QString error;
    const int fd = HidManager::openDevice(m_active, &error);
    if (fd < 0) {
        fail(error);
        return false;
    }

    auto write = [&](const QByteArray &data, bool readback) -> bool {
        QByteArray reply;
        return HidManager::sendFeature(fd, data, readback ? &reply : nullptr, &error);
    };

    bool ok = true;
    QByteArray packet(64, '\0');
    packet[0] = 0x04;
    packet[1] = 0x18;
    if (!write(packet, true)) {
        fail(QStringLiteral("进入重置会话：") + error);
        ok = false;
    }

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x04;
        packet[1] = static_cast<char>(command & 0xFF);
        packet[8] = 0x09;
        if (!write(packet, true)) {
            fail(QStringLiteral("准备清空灯光数据：") + error);
            ok = false;
        }
    }

    if (ok) {
        const QByteArray zero(64, '\0');
        for (int i = 0; i < 9 && ok; ++i) {
            if (!write(zero, false)) {
                fail(QStringLiteral("清空灯光数据：") + error);
                ok = false;
            }
        }
    }

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x04;
        packet[1] = 0x02;
        if (!write(packet, true)) {
            fail(QStringLiteral("结束重置会话：") + error);
            ok = false;
        }
    }

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x04;
        packet[1] = static_cast<char>(0xF0);
        if (!write(packet, true)) {
            fail(QStringLiteral("保存重置结果：") + error);
            ok = false;
        }
    }

    HidManager::closeDevice(fd);
    return ok;
}

bool KeyboardController::clearPerKeyUsb()
{
    QString error;
    const int fd = HidManager::openDevice(m_active, &error);
    if (fd < 0) {
        fail(error);
        return false;
    }

    auto write = [&](const QByteArray &data, bool readback) -> bool {
        QByteArray reply;
        return HidManager::sendFeature(fd, data, readback ? &reply : nullptr, &error);
    };

    bool ok = true;
    QByteArray packet(64, '\0');
    packet[0] = 0x04;
    packet[1] = 0x19;
    if (!write(packet, true)) {
        fail(QStringLiteral("进入逐键灯光会话：") + error);
        ok = false;
    }

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x04;
        packet[1] = 0x15;
        packet[8] = 0x08;
        if (!write(packet, true)) {
            fail(QStringLiteral("准备清空逐键灯光：") + error);
            ok = false;
        }
    }

    if (ok) {
        const QByteArray zero(64, '\0');
        for (int i = 0; i < 8 && ok; ++i) {
            if (!write(zero, false)) {
                fail(QStringLiteral("清空逐键灯光：") + error);
                ok = false;
            }
        }
    }

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x04;
        packet[1] = 0x02;
        if (!write(packet, true)) {
            fail(QStringLiteral("结束逐键灯光会话：") + error);
            ok = false;
        }
    }

    HidManager::closeDevice(fd);
    return ok;
}

bool KeyboardController::applyDefaultLightUsb()
{
    return sendLightUsb(11, 0xFF, 0x00, 0x00, 5, 3, 0, 1);
}

bool KeyboardController::syncClock()
{
    if (m_busy)
        return false;

    setBusy(true);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    bool ok = false;
    if (!ensureActive()) {
        ok = false;
    } else if (m_active.isWireless()) {
        ok = sendClockWireless();
    } else {
        ok = sendClockUsb();
    }

    setBusy(false);
    const QString message = ok
        ? QStringLiteral("时间已同步到键盘：%1")
              .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")))
        : (m_lastError.isEmpty() ? QStringLiteral("时间同步失败") : m_lastError);
    emit clockSynced(ok, message);
    return ok;
}

bool KeyboardController::sendClockUsb()
{
    const QDateTime now = QDateTime::currentDateTime();
    const int year = now.date().year() % 2000;
    const int dow = now.date().dayOfWeek() % 7;

    QString error;
    const int fd = HidManager::openDevice(m_active, &error);
    if (fd < 0) {
        fail(error);
        return false;
    }

    auto sendControl = [&](const QByteArray &data, const QString &label) -> bool {
        QByteArray response;
        if (!HidManager::sendFeature(fd, data, &response, &error)) {
            fail(label + QStringLiteral("：") + error);
            return false;
        }
        if (response.size() >= 5 && static_cast<quint8>(response.at(4)) != 1) {
            if (qEnvironmentVariableIsSet("GSKBD_DEBUG"))
                qWarning().noquote() << label << "unexpected ACK:" << response.toHex();
        }
        return true;
    };

    bool ok = true;
    QByteArray packet(64, '\0');

    packet[0] = 0x04;
    packet[1] = 0x18;
    if (!sendControl(packet, QStringLiteral("进入校时会话")))
        ok = false;

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x04;
        packet[1] = 0x28;
        packet[8] = 0x01;
        if (!sendControl(packet, QStringLiteral("准备写入时间")))
            ok = false;
    }

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x00;
        packet[1] = 0x01;
        packet[2] = 0x5A;
        packet[3] = static_cast<char>(year);
        packet[4] = static_cast<char>(now.date().month());
        packet[5] = static_cast<char>(now.date().day());
        packet[6] = static_cast<char>(now.time().hour());
        packet[7] = static_cast<char>(now.time().minute());
        packet[8] = static_cast<char>(now.time().second());
        packet[10] = static_cast<char>(dow);
        packet[62] = static_cast<char>(0xAA);
        packet[63] = static_cast<char>(0x55);

        QByteArray response;
        if (!HidManager::sendFeature(fd, packet, &response, &error)) {
            fail(QStringLiteral("写入时间：") + error);
            ok = false;
        }
    }

    if (ok) {
        packet.fill('\0');
        packet[0] = 0x04;
        packet[1] = 0x02;
        if (!sendControl(packet, QStringLiteral("结束校时会话")))
            ok = false;
    }

    HidManager::closeDevice(fd);
    return ok;
}

bool KeyboardController::sendClockWireless()
{
    const QDateTime now = QDateTime::currentDateTime();
    const int year = now.date().year() % 2000;
    const int dow = now.date().dayOfWeek() % 7;

    QByteArray frame(33, '\0');
    frame[1] = 0x0C;
    frame[2] = 0x10;
    frame[3] = 0x00;
    frame[5] = 0x01;
    frame[6] = 0x5A;
    frame[7] = static_cast<char>(year);
    frame[8] = static_cast<char>(now.date().month());
    frame[9] = static_cast<char>(now.date().day());
    frame[10] = static_cast<char>(now.time().hour());
    frame[11] = static_cast<char>(now.time().minute());
    frame[12] = static_cast<char>(now.time().second());
    frame[14] = static_cast<char>(dow);
    frame[18] = static_cast<char>(0xAA);
    frame[19] = static_cast<char>(0x55);

    quint8 sum = 0;
    for (int i = 0; i < 32; ++i)
        sum = static_cast<quint8>(sum + static_cast<quint8>(frame.at(i)));
    frame[32] = static_cast<char>(sum);

    QString error;
    const int fd = HidManager::openDevice(m_active, &error);
    if (fd < 0) {
        fail(error);
        return false;
    }

    QByteArray response;
    const bool written = HidManager::sendOutput(fd, frame, &response, 800, &error);
    HidManager::closeDevice(fd);
    if (!written) {
        fail(error);
        return false;
    }

    if (response.size() < 3
        || response.at(0) != frame.at(1)
        || response.at(1) != frame.at(2)
        || response.at(2) != frame.at(3)) {
        fail(QStringLiteral("未收到 2.4G 回执。请确认键盘已切换到 2.4G 模式（FN+4），并按一下按键唤醒。"));
        return false;
    }
    return true;
}

bool KeyboardController::applyTestLight()
{
    return applyLight(7, 255, 0, 0, 5, 3, 0, 0);
}
