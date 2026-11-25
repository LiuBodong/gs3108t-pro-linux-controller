#pragma once

#include <QObject>
#include <QVariantList>

#include "hidmanager.h"

class KeyboardController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString connectionLabel READ connectionLabel NOTIFY stateChanged)
    Q_PROPERTY(QString connectionType READ connectionType NOTIFY stateChanged)
    Q_PROPERTY(QString deviceNode READ deviceNode NOTIFY stateChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY stateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY stateChanged)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY stateChanged)
    Q_PROPERTY(QString preferredMode READ preferredMode WRITE setPreferredMode
                   NOTIFY preferredModeChanged)

public:
    explicit KeyboardController(QObject *parent = nullptr);

    bool connected() const { return m_hasActive; }
    bool busy() const { return m_busy; }
    QString connectionLabel() const;
    QString connectionType() const;
    QString deviceNode() const { return m_active.node; }
    QString deviceName() const { return m_active.name; }
    QString lastError() const { return m_lastError; }
    QVariantList devices() const;
    QString preferredMode() const { return m_preferredMode; }
    void setPreferredMode(const QString &mode);
    void setPreferredModeTransient(const QString &mode);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool applyLight(int mode, int r, int g, int b,
                                int brightness, int speed,
                                int direction, int colorful);
    Q_INVOKABLE bool applySystemConfig(int response, int sleep,
                                       bool disableWin, bool disableAltF4,
                                       bool disableAltTab);
    Q_INVOKABLE bool resetKeyboard(bool factoryReset);
    Q_INVOKABLE bool syncClock();
    Q_INVOKABLE bool applyTestLight();

signals:
    void stateChanged();
    void preferredModeChanged();
    void lightApplied(bool ok, const QString &message);
    void systemConfigApplied(bool ok, const QString &message);
    void resetApplied(bool ok, const QString &message, bool factoryReset);
    void clockSynced(bool ok, const QString &message);

private:
    bool ensureActive();
    bool sendLightUsb(int mode, int r, int g, int b,
                      int brightness, int speed, int direction, int colorful);
    bool sendLightWireless(int mode, int r, int g, int b,
                           int brightness, int speed, int direction, int colorful);
    bool sendSystemConfigUsb(int response, int sleep,
                             bool disableWin, bool disableAltF4,
                             bool disableAltTab);
    bool clearLightStoreUsb(int command);
    bool clearPerKeyUsb();
    bool applyDefaultLightUsb();
    bool sendClockUsb();
    bool sendClockWireless();

    void setBusy(bool busy);
    void fail(const QString &message);

    QList<HidInterface> m_devices;
    HidInterface m_active;
    bool m_hasActive = false;
    bool m_busy = false;
    QString m_lastError;
    QString m_preferredMode = QStringLiteral("auto");
};
