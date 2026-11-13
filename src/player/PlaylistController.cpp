#include "PlaylistController.h"

#include <QFileInfo>

PlaylistController::PlaylistController(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PlaylistController::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_urls.size();
}

QVariant PlaylistController::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_urls.size())
        return {};
    const QUrl &url = m_urls.at(index.row());
    switch (role) {
    case UrlRole: return url;
    case TitleRole: return QFileInfo(url.toLocalFile()).completeBaseName();
    case CurrentRole: return index.row() == m_currentIndex;
    default: return {};
    }
}

QHash<int, QByteArray> PlaylistController::roleNames() const
{
    return {{UrlRole, "url"}, {TitleRole, "title"}, {CurrentRole, "current"}};
}

int PlaylistController::currentIndex() const { return m_currentIndex; }
int PlaylistController::count() const { return m_urls.size(); }

void PlaylistController::addUrls(const QList<QUrl> &urls)
{
    if (urls.isEmpty())
        return;
    beginInsertRows(QModelIndex(), m_urls.size(), m_urls.size() + urls.size() - 1);
    m_urls.append(urls);
    endInsertRows();
    emit countChanged();
    if (m_currentIndex < 0)
        playAt(0);
}

void PlaylistController::removeAt(int index)
{
    if (index < 0 || index >= m_urls.size())
        return;
    beginRemoveRows(QModelIndex(), index, index);
    m_urls.removeAt(index);
    endRemoveRows();
    emit countChanged();

    if (m_urls.isEmpty()) {
        setCurrentIndex(-1);
    } else if (index == m_currentIndex) {
        setCurrentIndex(-1);
        playAt(qMin(index, m_urls.size() - 1));
    } else if (index < m_currentIndex) {
        setCurrentIndex(m_currentIndex - 1);
    }
}

void PlaylistController::clear()
{
    if (m_urls.isEmpty())
        return;
    beginResetModel();
    m_urls.clear();
    endResetModel();
    setCurrentIndex(-1);
    emit countChanged();
}

void PlaylistController::playAt(int index)
{
    if (index < 0 || index >= m_urls.size())
        return;
    setCurrentIndex(index);
    emit requestOpen(m_urls.at(index));
}

void PlaylistController::next()
{
    if (m_currentIndex + 1 < m_urls.size())
        playAt(m_currentIndex + 1);
}

void PlaylistController::previous()
{
    if (m_currentIndex > 0)
        playAt(m_currentIndex - 1);
}

void PlaylistController::onMediaEnded()
{
    if (m_currentIndex >= 0 && m_currentIndex + 1 < m_urls.size())
        next();
}

void PlaylistController::setCurrentIndex(int index)
{
    if (m_currentIndex == index)
        return;
    const int old = m_currentIndex;
    m_currentIndex = index;
    if (old >= 0 && old < m_urls.size())
        emit dataChanged(this->index(old), this->index(old), {CurrentRole});
    if (index >= 0 && index < m_urls.size())
        emit dataChanged(this->index(index), this->index(index), {CurrentRole});
    emit currentIndexChanged();
}