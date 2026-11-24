#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

struct HidInterface {
    QString node;
    QString name;
    quint16 vendorId = 0;
    quint16 productId = 0;
    quint16 usagePage = 0;
    QList<int> inputSizes;
    QList<int> outputSizes;
    QList<int> featureSizes;

    bool isWireless() const;
    bool isUsb() const;
    QString typeLabel() const;
};

class HidManager {
public:
    static QList<HidInterface> scan();
    static bool parseDescriptor(const QByteArray &desc, HidInterface &info);

    static int openDevice(const HidInterface &dev, QString *error = nullptr);
    static bool sendFeature(int fd, const QByteArray &data64,
                            QByteArray *response = nullptr,
                            QString *error = nullptr);
    static bool sendOutput(int fd, const QByteArray &frame,
                           QByteArray *response = nullptr,
                           int timeoutMs = 300,
                           QString *error = nullptr);
    static void closeDevice(int fd);
};
