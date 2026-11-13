#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // The MSYS2 FFmpeg backend (gcc16 + FFmpeg 9) has an intermittent startup
    // crash; the native Windows Media Foundation backend is stable. Default to
    // WMF on Windows, but allow an explicit QT_MEDIA_BACKEND override.
    if (qEnvironmentVariableIsEmpty("QT_MEDIA_BACKEND"))
        qputenv("QT_MEDIA_BACKEND", "windows");
#endif

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("DBHX Player"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QGuiApplication::setOrganizationName(QStringLiteral("DBHX"));

    // Basic style gives us full control over the custom, per-platform theming.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // Optional file path argument: open it on startup (file association / CLI).
    QString initialFile;
    const QStringList args = app.arguments();
    if (args.size() > 1)
        initialFile = QUrl::fromLocalFile(args.at(1)).toString();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("initialFile"), initialFile);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Dbhx", "Main");

    return app.exec();
}