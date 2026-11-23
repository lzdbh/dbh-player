#include <QApplication>
#include <QColor>
#include <QLocale>
#include <QMenu>
#include <QPalette>
#include <QStyleHints>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QPainter>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QTranslator>
#include <QUrl>

#ifdef Q_OS_WIN
#include <dwmapi.h>
#endif

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

#ifdef Q_OS_WIN
// Windows 11 rounds top-level windows natively (DWM-drawn, with anti-aliasing
// and a system shadow). Doing it natively avoids the layered-surface marching
// ants a QML mask produces at the window corners.
void installNativeRoundedCorners(QQuickWindow *window)
{
    if (!window)
        return;

    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    const auto setCornerPreference = [hwnd](DWM_WINDOW_CORNER_PREFERENCE preference) {
        DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference,
                              sizeof(preference));
    };

    setCornerPreference(DWMWCP_ROUND);
    // Remove the 1px system border so the dark shell reaches the window edge.
    const COLORREF noBorder = DWMWA_COLOR_NONE;
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &noBorder, sizeof(noBorder));

    // Maximized/fullscreen windows are flush with the screen; rounding there
    // would reveal the desktop at the corners.
    const auto update = [setCornerPreference, window]() {
        const QWindow::Visibility visibility = window->visibility();
        const bool square = visibility == QWindow::FullScreen
                            || visibility == QWindow::Maximized;
        setCornerPreference(square ? DWMWCP_DONOTROUND : DWMWCP_ROUND);
    };
    update();
    QObject::connect(window, &QWindow::visibilityChanged, window, update);
}
#endif
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
#if defined(Q_OS_MACOS) && defined(Q_PROCESSOR_X86_64)
    // On Intel Macs the FFmpeg backend's VideoToolbox hwaccel path copies
    // frames back to CPU and caps delivery (~16fps for 1080p/4K), causing
    // stutter. Software decoding sustains the full frame rate (measured 25fps
    // for 4K HEVC at ~34% CPU) and is smooth. Disable hwaccel unless overridden.
    if (qEnvironmentVariableIsEmpty("QT_FFMPEG_DECODING_HW_DEVICE_TYPES"))
        qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", "none");
#endif

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("DBHX Player"));
    QApplication::setApplicationVersion(QStringLiteral(DBHX_VERSION));
    QApplication::setOrganizationName(QStringLiteral("DBHX"));
    QApplication::setQuitOnLastWindowClosed(true);
    Logging::install();

    // Force dark color scheme so default-styled controls (e.g. the internally
    // rendered nested submenu entry) use light text like the custom UI.
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);

    // Dark application palette so default-styled Qt Quick controls (e.g. the
    // internally-rendered nested submenu entry) use light text and a dark
    // hover instead of the light defaults.
    {
        QPalette pal;
        pal.setColor(QPalette::Window, QColor(0x26, 0x33, 0x3e));
        pal.setColor(QPalette::WindowText, QColor(0xee, 0xf2, 0xf6));
        pal.setColor(QPalette::Base, QColor(0x1e, 0x28, 0x2f));
        pal.setColor(QPalette::AlternateBase, QColor(0x2e, 0x3c, 0x48));
        pal.setColor(QPalette::Text, QColor(0xee, 0xf2, 0xf6));
        pal.setColor(QPalette::Button, QColor(0x2e, 0x3c, 0x48));
        pal.setColor(QPalette::ButtonText, QColor(0xee, 0xf2, 0xf6));
        pal.setColor(QPalette::Highlight, QColor(0x3a, 0x4a, 0x58));
        pal.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::ToolTipBase, QColor(0x1e, 0x28, 0x2f));
        pal.setColor(QPalette::ToolTipText, QColor(0xee, 0xf2, 0xf6));
        pal.setColor(QPalette::PlaceholderText, QColor(0x7f, 0x8d, 0x9a));
        pal.setColor(QPalette::Mid, QColor(0x7f, 0x8d, 0x9a));
        pal.setColor(QPalette::Light, QColor(0x40, 0x50, 0x5c));
        pal.setColor(QPalette::Dark, QColor(0x1a, 0x22, 0x29));
        QApplication::setPalette(pal);
    }

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

    // Optional file path arguments: open them on startup (file association /
    // CLI). Skip option-style args such as -AppleLanguages.
    QStringList initialFiles;
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (!args.at(i).startsWith(QLatin1Char('-')))
            initialFiles << QUrl::fromLocalFile(args.at(i)).toString();
    }

    QQmlApplicationEngine engine;
    // The QML module is compiled into the executable at the resource root
    // (:/Dbhx). Newer Qt locates it automatically; older Qt only resolves
    // loadFromModule() when the root is on the import path.
    engine.addImportPath(QStringLiteral(":/"));
    engine.rootContext()->setContextProperty(QStringLiteral("initialFiles"), initialFiles);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Dbhx", "Main");

    auto *window = engine.rootObjects().isEmpty()
                       ? nullptr
                       : qobject_cast<QQuickWindow *>(engine.rootObjects().first());
#ifdef Q_OS_WIN
    installNativeRoundedCorners(window);
#endif

    if (QSystemTrayIcon::isSystemTrayAvailable() && window) {
        QObject *player = window->findChild<QObject *>(QStringLiteral("playerController"));

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