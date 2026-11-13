#include "AppContext.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QtGlobal>

AppContext::AppContext(QObject *parent)
    : QObject(parent)
{
#if defined(Q_OS_MACOS)
    m_platform = QStringLiteral("macos");
#elif defined(Q_OS_WIN)
    m_platform = QStringLiteral("windows");
#elif defined(Q_OS_LINUX)
    m_platform = QStringLiteral("linux");
#else
    m_platform = QStringLiteral("unknown");
#endif
}

QString AppContext::platform() const
{
    return m_platform;
}

bool AppContext::isMac() const
{
    return m_platform == QLatin1String("macos");
}

bool AppContext::isWindows() const
{
    return m_platform == QLatin1String("windows");
}

QString AppContext::appName() const
{
    return QCoreApplication::applicationName();
}

QString AppContext::appVersion() const
{
    return QCoreApplication::applicationVersion();
}

QString AppContext::picturesDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
}