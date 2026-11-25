#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QStyleFactory>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "keyboardcontroller.h"
#include "mainwindow.h"
#include "theme.h"
#include "widgets.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("GS3108T PRO 控制中心"));
    QApplication::setApplicationDisplayName(QStringLiteral("GS3108T PRO 控制中心"));
    QApplication::setOrganizationName(QStringLiteral("gs3108t"));
    QApplication::setOrganizationDomain(QStringLiteral("gs3108t.local"));

    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication::setFont(Theme::uiFont(13));
    app.setStyleSheet(Theme::styleSheet());
    app.setWindowIcon(QIcon(Theme::logoPixmap(64)));

    KeyboardController controller;
    const QStringList args = app.arguments();

    if (args.contains(QStringLiteral("--wireless")))
        controller.setPreferredModeTransient(QStringLiteral("wireless"));
    else if (args.contains(QStringLiteral("--usb")))
        controller.setPreferredModeTransient(QStringLiteral("usb"));

    if (args.contains(QStringLiteral("--list"))) {
        const QVariantList devices = controller.devices();
        for (const QVariant &entry : devices) {
            const QVariantMap device = entry.toMap();
            qInfo().noquote()
                << device.value(QStringLiteral("node")).toString()
                << device.value(QStringLiteral("name")).toString()
                << device.value(QStringLiteral("type")).toString()
                << device.value(QStringLiteral("usagePage")).toString()
                << "feature=" + device.value(QStringLiteral("feature")).toString()
                << "output=" + device.value(QStringLiteral("output")).toString();
        }
        if (devices.isEmpty())
            qInfo().noquote() << "未找到 GS3108T-PRO 键盘";
        return 0;
    }

    if (args.contains(QStringLiteral("--apply-test"))) {
        const bool ok = controller.applyTestLight();
        qInfo().noquote() << (ok ? "灯光测试已发送" : "灯光测试失败")
                          << controller.lastError();
        return ok ? 0 : 1;
    }

    const int gifIndex = args.indexOf(QStringLiteral("--gif-upload"));
    if (gifIndex >= 0 && gifIndex + 1 < args.size()) {
        int lastPercent = -1;
        QObject::connect(&controller, &KeyboardController::gifUploadProgress,
                         [&lastPercent](int percent) {
                             if (percent / 10 != lastPercent / 10) {
                                 lastPercent = percent;
                                 qInfo().noquote() << QStringLiteral("上传进度 %1%").arg(percent);
                             }
                         });
        const bool ok = controller.uploadGif(args.at(gifIndex + 1));
        qInfo().noquote() << (ok ? "动图已上传" : "动图上传失败")
                          << controller.lastError();
        return ok ? 0 : 1;
    }

    if (args.contains(QStringLiteral("--config-test"))) {
        const bool ok = controller.applySystemConfig(3, 1, false, false, false);
        qInfo().noquote() << (ok ? "系统配置测试已发送" : "系统配置测试失败")
                          << controller.lastError();
        return ok ? 0 : 1;
    }

    if (args.contains(QStringLiteral("--reset-keyboard"))
        || args.contains(QStringLiteral("--factory-reset"))) {
        const bool factory = args.contains(QStringLiteral("--factory-reset"));
        const bool ok = controller.resetKeyboard(factory);
        qInfo().noquote() << (ok ? (factory ? "恢复出厂已发送" : "重置键盘已发送")
                                 : (factory ? "恢复出厂失败" : "重置键盘失败"))
                          << controller.lastError();
        return ok ? 0 : 1;
    }

    if (args.contains(QStringLiteral("--clock-test"))) {
        const bool ok = controller.syncClock();
        qInfo().noquote() << (ok ? "校时已发送" : "校时失败")
                          << controller.lastError();
        return ok ? 0 : 1;
    }

    MainWindow window(&controller);
    window.show();

    if (args.contains(QStringLiteral("--dialog-test")))
        QTimer::singleShot(700, &window, [&window]() {
            if (qEnvironmentVariable("GSKBD_DIALOG_TEST") == QStringLiteral("confirm")) {
                AppDialog::confirm(&window, QStringLiteral("删除配置"),
                                   QStringLiteral("确定删除「夜间护眼」？"),
                                   QStringLiteral("删除"), QStringLiteral("取消"));
                return;
            }
            bool ok = false;
            AppDialog::prompt(&window, QStringLiteral("保存配置"),
                              QStringLiteral("配置名称"),
                              QStringLiteral("配置 1"), &ok);
        });

    const int pageIndex = args.indexOf(QStringLiteral("--page"));
    if (pageIndex >= 0 && pageIndex + 1 < args.size())
        window.setPage(args.at(pageIndex + 1).toInt());

    const int screenshotIndex = args.indexOf(QStringLiteral("--screenshot"));
    if (screenshotIndex >= 0 && screenshotIndex + 1 < args.size()) {
        const QString path = args.at(screenshotIndex + 1);
        if (args.contains(QStringLiteral("--screenshot-bottom")))
            QTimer::singleShot(1200, &app, [&window]() { window.scrollPageToBottom(); });
        QTimer::singleShot(1400, &app, [&window, &app, path]() {
            const QPixmap image = window.grab();
            if (!image.isNull() && image.save(path))
                qInfo().noquote() << "截图已保存:" << path;
            else
                qWarning().noquote() << "截图保存失败:" << path;
            app.quit();
        });
    }

    return app.exec();
}
