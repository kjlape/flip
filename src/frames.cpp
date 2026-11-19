#include "frames.h"

Frames::Frames(QObject *parent) : QAbstractListModel(parent) {}

quint64 Frames::nextVersion() {
    static quint64 version = 0;
    return ++version;
}

int Frames::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : m_frames.size(); }

QVariant Frames::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_frames.size() || role != VersionRole)
        return {};
    // A string, because QML numbers can't hold every 64-bit version.
    return QString::number(m_frames[index.row()].version);
}

QHash<int, QByteArray> Frames::roleNames() const { return {{VersionRole, "version"}}; }

QImage Frames::image(int index) const { return index >= 0 && index < m_frames.size() ? m_frames[index].image : QImage(); }

quint64 Frames::version(int index) const { return index >= 0 && index < m_frames.size() ? m_frames[index].version : 0; }

QList<QImage> Frames::images() const {
    QList<QImage> images;
    for (const Frame &frame : m_frames)
        images.append(frame.image);
    return images;
}

QList<quint64> Frames::versions() const {
    QList<quint64> versions;
    for (const Frame &frame : m_frames)
        versions.append(frame.version);
    return versions;
}

QImage Frames::imageForVersion(quint64 version) const {
    for (const Frame &frame : m_frames)
        if (frame.version == version)
            return frame.image;
    return {};
}

QImage &Frames::edit(int index) { return m_frames[index].image; }

void Frames::touch(int index) {
    m_frames[index].version = nextVersion();
    emit dataChanged(this->index(index), this->index(index), {VersionRole});
}

void Frames::insert(int index, const QImage &image) {
    beginInsertRows({}, index, index);
    m_frames.insert(index, {image, nextVersion()});
    endInsertRows();
    emit countChanged();
}

void Frames::remove(int index) {
    beginRemoveRows({}, index, index);
    m_frames.removeAt(index);
    endRemoveRows();
    emit countChanged();
}

void Frames::move(int from, int to) {
    if (from == to)
        return;
    // Qt counts the destination as the row the moved one lands in front of.
    beginMoveRows({}, from, from, {}, to > from ? to + 1 : to);
    m_frames.move(from, to);
    endMoveRows();
}

void Frames::reset(const QList<QImage> &images) {
    QList<Frame> frames;
    for (const QImage &image : images)
        frames.append({image, nextVersion()});
    restore(frames);
}

void Frames::restore(const QList<Frame> &frames) {
    // Undoing a stroke changes one frame; only a different count needs the
    // timeline rebuilt.
    if (frames.size() == m_frames.size()) {
        for (int i = 0; i < frames.size(); ++i) {
            if (frames[i].version == m_frames[i].version)
                continue;
            m_frames[i] = frames[i];
            emit dataChanged(index(i), index(i), {VersionRole});
        }
        return;
    }
    beginResetModel();
    m_frames = frames;
    endResetModel();
    emit countChanged();
}
