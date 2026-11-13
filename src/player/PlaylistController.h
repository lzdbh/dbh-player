#pragma once

#include <QAbstractListModel>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// Ordered playback queue with auto-advance, exposed to QML as a list model.
class PlaylistController : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles { UrlRole = Qt::UserRole + 1, TitleRole, CurrentRole };

    explicit PlaylistController(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int currentIndex() const;
    int count() const;

    Q_INVOKABLE void addUrls(const QList<QUrl> &urls);
    Q_INVOKABLE void removeAt(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void playAt(int index);
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    // Called when the current media ends; advances if possible.
    Q_INVOKABLE void onMediaEnded();

signals:
    void currentIndexChanged();
    void countChanged();
    void requestOpen(const QUrl &url);

private:
    void setCurrentIndex(int index);

    QList<QUrl> m_urls;
    int m_currentIndex = -1;
};