#include <QApplication>
#include <QLocale>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QPainter>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QTranslator>
#include <QUrl>

#include "Logging.h"

namespace {
QIcon makeTrayIcon()
{
    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(QStringLiteral("#1677ff")));
    p.drawRoundedRect(4, 4, 56, 56, 14, 14);
    p.setBrush(Qt::white);
    QPolygonF triangle;
    triangle << QPointF(26, 20) << QPointF(26, 44) << QPointF(46, 32);
    p.drawPolygon(triangle);
    p.end();
    return QIcon(pm);
}
} // namespace

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // The MSYS2 FFmpeg backend (gcc16 + FFmpeg 9) has an intermittent startup
    // crash; the native Windows Media Foundation backend is stable. Default to
    // WMF on Windows, but allow an explicit QT_MEDIA_BACKEND override.
    if (qEnvironmentVariableIsEmpty("QT_MEDIA_BACKEND"))
        qputenv("QT_MEDIA_BACKEND", "windows");
#endif

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("DBHX Player"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setOrganizationName(QStringLiteral("DBHX"));
    QApplication::setQuitOnLastWindowClosed(true);
    Logging::install();

    // Source strings are Chinese; load English when the system isn't Chinese.
    QTranslator translator;
    const bool english = !QLocale::system().name().startsWith(QLatin1String("zh"))
                         && translator.load(QStringLiteral(":/i18n/dbhx_en.qm"));
    if (english)
        QApplication::installTranslator(&translator);
    qCInfo(lcPlayer) << "DBHX Player" << QApplication::applicationVersion()
                     << "locale:" << QLocale::system().name()
                     << "english UI:" << english;

    // Basic style gives us full control over the custom, per-platform theming.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // Threaded scene-graph render loop presents video on a dedicated thread at
    // display vsync (Metal), avoiding main-thread stalls that cause A/V stutter.
    qputenv("QSG_RENDER_LOOP", "threaded");

    // Optional file path arguments: open them on startup (file association /
    // CLI). Skip option-style args such as -AppleLanguages.
    QStringList initialFiles;
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (!args.at(i).startsWith(QLatin1Char('-')))
            initialFiles << QUrl::fromLocalFile(args.at(i)).toString();
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("initialFiles"), initialFiles);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Dbhx", "Main");

    if (QSystemTrayIcon::isSystemTrayAvailable() && !engine.rootObjects().isEmpty()) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QObject *player = window ? window->findChild<QObject *>(QStringLiteral("playerController"))
                                 : nullptr;

        auto *tray = new QSystemTrayIcon(makeTrayIcon(), &app);
        auto *menu = new QMenu();
        QObject::connect(menu->addAction(QStringLiteral("显示主界面")), &QAction::triggered,
                         &app, [window]() {
                             if (window) {
                                 window->show();
                                 window->raise();
                                 window->requestActivate();
                             }
                         });
        QObject::connect(menu->addAction(QStringLiteral("播放 / 暂停")), &QAction::triggered,
                         &app, [player]() {
                             if (player)
                                 QMetaObject::invokeMethod(player, "togglePlay");
                         });
        menu->addSeparator();
        QObject::connect(menu->addAction(QStringLiteral("退出")), &QAction::triggered,
                         &app, &QApplication::quit);
        tray->setContextMenu(menu);
        tray->setToolTip(QStringLiteral("DBHX Player"));
        QObject::connect(tray, &QSystemTrayIcon::activated, &app,
                         [window](QSystemTrayIcon::ActivationReason reason) {
                             if (reason == QSystemTrayIcon::DoubleClick && window) {
                                 window->show();
                                 window->raise();
                             }
                         });
        tray->show();
    }

    return app.exec();
}