#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

// Application-wide context exposed to QML as a singleton (Dbhx.AppContext).
// Holds platform identity and app metadata; later tracks plug services here.
class AppContext : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString platform READ platform CONSTANT)
    Q_PROPERTY(bool isMac READ isMac CONSTANT)
    Q_PROPERTY(bool isWindows READ isWindows CONSTANT)
    Q_PROPERTY(QString appName READ appName CONSTANT)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)

public:
    explicit AppContext(QObject *parent = nullptr);

    QString platform() const;
    bool isMac() const;
    bool isWindows() const;
    QString appName() const;
    QString appVersion() const;

    Q_INVOKABLE QString picturesDir() const;
    Q_INVOKABLE QVariant loadSetting(const QString &key, const QVariant &defaultValue) const;
    Q_INVOKABLE void saveSetting(const QString &key, const QVariant &value) const;

private:
    QString m_platform;
};