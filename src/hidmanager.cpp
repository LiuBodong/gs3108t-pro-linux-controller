#include "hidmanager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QThread>

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <linux/hidraw.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

bool HidInterface::isWireless() const
{
    return outputSizes.contains(32)
        && (usagePage == 0xFF60 || name.contains(QStringLiteral("2.4G"), Qt::CaseInsensitive));
}

bool HidInterface::isUsb() const
{
    return featureSizes.contains(64)
        && (usagePage == 0xFF13 || name.contains(QStringLiteral("SONiX"), Qt::CaseInsensitive));
}

QString HidInterface::typeLabel() const
{
    if (isWireless())
        return QStringLiteral("2.4G 无线");
    if (isUsb())
        return QStringLiteral("USB 有线");
    return QStringLiteral("未知接口");
}

QList<HidInterface> HidManager::scan()
{
    QList<HidInterface> result;
    QDir dir(QStringLiteral("/sys/class/hidraw"));
    const QStringList entries = dir.entryList(QStringList() << QStringLiteral("hidraw*"),
                                              QDir::Dirs | QDir::NoDotAndDotDot,
                                              QDir::Name);
    for (const QString &entry : entries) {
        const QString base = dir.filePath(entry);

        QFile uevent(base + QStringLiteral("/device/uevent"));
        if (!uevent.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        QMap<QString, QString> fields;
        const QByteArray ueventData = uevent.readAll();
        const QList<QByteArray> lines = ueventData.split('\n');
        for (const QByteArray &rawLine : lines) {
            const QString line = QString::fromUtf8(rawLine).trimmed();
            const int eq = line.indexOf(QLatin1Char('='));
            if (eq > 0)
                fields.insert(line.left(eq), line.mid(eq + 1));
        }

        const QString hidId = fields.value(QStringLiteral("HID_ID")).toUpper();
        const QStringList parts = hidId.split(QLatin1Char(':'));
        if (parts.size() < 3)
            continue;

        bool vidOk = false;
        bool pidOk = false;
        const quint16 vid = parts.at(1).toUShort(&vidOk, 16);
        const quint16 pid = parts.at(2).toUShort(&pidOk, 16);
        if (!vidOk || !pidOk || vid != 0x05AC || pid != 0x024F)
            continue;

        HidInterface info;
        info.node = QStringLiteral("/dev/") + entry;
        info.name = fields.value(QStringLiteral("HID_NAME"));
        info.vendorId = vid;
        info.productId = pid;

        QFile descFile(base + QStringLiteral("/device/report_descriptor"));
        if (descFile.open(QIODevice::ReadOnly))
            parseDescriptor(descFile.readAll(), info);

        result.append(info);
    }
    return result;
}

bool HidManager::parseDescriptor(const QByteArray &desc, HidInterface &info)
{
    int i = 0;
    int reportSize = -1;
    int reportCount = -1;
    quint16 usagePage = 0;

    while (i < desc.size()) {
        const quint8 b = static_cast<quint8>(desc.at(i));
        const int sizeCode = b & 0x03;
        const int size = sizeCode == 3 ? 4 : sizeCode;
        const quint8 tag = b & 0xFC;

        quint32 value = 0;
        for (int k = 0; k < size; ++k) {
            const int pos = i + 1 + k;
            if (pos < desc.size())
                value |= static_cast<quint32>(static_cast<quint8>(desc.at(pos))) << (8 * k);
        }

        if (tag == 0x04) {
            usagePage = static_cast<quint16>(value);
        } else if (tag == 0x74) {
            reportSize = static_cast<int>(value);
        } else if (tag == 0x94) {
            reportCount = static_cast<int>(value);
        } else if (tag == 0x84) {
            // Report ID; the sizes below are still useful for interface detection.
        } else if ((tag == 0x80 || tag == 0x90 || tag == 0xB0)
                   && reportSize > 0 && reportCount > 0) {
            const int bytes = reportSize * reportCount / 8;
            if (tag == 0x80)
                info.inputSizes.append(bytes);
            else if (tag == 0x90)
                info.outputSizes.append(bytes);
            else
                info.featureSizes.append(bytes);
        }

        i += 1 + size;
    }

    info.usagePage = usagePage;
    return true;
}

int HidManager::openDevice(const HidInterface &dev, QString *error)
{
    const int fd = ::open(dev.node.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);
    if (fd < 0 && error) {
        *error = QStringLiteral("无法打开 %1：%2")
                     .arg(dev.node, QString::fromLocal8Bit(std::strerror(errno)));
    }
    return fd;
}

bool HidManager::sendFeature(int fd, const QByteArray &data64,
                             QByteArray *response, QString *error)
{
    QByteArray buf(65, '\0');
    const int n = qMin(64, data64.size());
    std::memcpy(buf.data() + 1, data64.constData(), n);

    if (::ioctl(fd, HIDIOCSFEATURE(65), buf.data()) < 0) {
        if (error)
            *error = QStringLiteral("写入 Feature 报告失败：%1")
                         .arg(QString::fromLocal8Bit(std::strerror(errno)));
        return false;
    }

    QThread::msleep(30);

    if (response) {
        QByteArray resp(65, '\0');
        if (::ioctl(fd, HIDIOCGFEATURE(65), resp.data()) < 0) {
            if (error)
                *error = QStringLiteral("读取 Feature 回执失败：%1")
                             .arg(QString::fromLocal8Bit(std::strerror(errno)));
            return false;
        }
        *response = resp;
        if (qEnvironmentVariableIsSet("GSKBD_DEBUG")) {
            qInfo().noquote() << "HID SET" << data64.toHex()
                              << "GET" << resp.toHex();
        }
    }
    return true;
}

bool HidManager::sendOutput(int fd, const QByteArray &frame,
                            QByteArray *response, int timeoutMs, QString *error)
{
    ssize_t n = ::write(fd, frame.constData(), static_cast<size_t>(frame.size()));
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        // 大块 Output（GIF 的 4097 字节）可能暂时写不进去，等设备腾出缓冲再重试。
        for (int attempt = 0; attempt < 50 && n < 0; ++attempt) {
            struct pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            if (::poll(&pfd, 1, 100) <= 0)
                continue;
            n = ::write(fd, frame.constData(), static_cast<size_t>(frame.size()));
            if (n >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
                break;
        }
    }
    if (n < 0 && (errno == EINVAL || errno == EMSGSIZE) && frame.size() == 33) {
        // Some kernels expect the 32 data bytes without the report-ID byte.
        n = ::write(fd, frame.constData() + 1, 32);
    }
    if (n < 0) {
        if (error)
            *error = QStringLiteral("写入 Output 报告失败：%1")
                         .arg(QString::fromLocal8Bit(std::strerror(errno)));
        return false;
    }

    if (response) {
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        const int ready = ::poll(&pfd, 1, timeoutMs);
        if (ready > 0) {
            char buf[4096];
            const ssize_t r = ::read(fd, buf, sizeof(buf));
            if (r > 0)
                *response = QByteArray(buf, static_cast<int>(r));
        }
    }
    return true;
}

void HidManager::closeDevice(int fd)
{
    if (fd >= 0)
        ::close(fd);
}
