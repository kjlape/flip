#pragma once

#include <QAbstractListModel>
#include <QImage>

// Every change to a frame gives it a new version, so thumbnails can be cached
// by version and an undone frame finds its old thumbnail again.
struct Frame {
    QImage image;
    quint64 version = 0;
};

// The frames of the open animation, as the model the timeline shows. QImage
// shares its pixels until written, so copying the whole list for undo is
// cheap: only the frame being drawn on gets duplicated.
class Frames : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles { VersionRole = Qt::UserRole + 1 };

    explicit Frames(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_frames.size(); }
    QImage image(int index) const;
    quint64 version(int index) const;
    QList<Frame> all() const { return m_frames; }
    QList<QImage> images() const;
    QList<quint64> versions() const;
    QImage imageForVersion(quint64 version) const;

    // Paint into the returned image, then call touch() when the stroke ends.
    QImage &edit(int index);
    void touch(int index);
    void insert(int index, const QImage &image);
    void remove(int index);
    void move(int from, int to);
    void reset(const QList<QImage> &images);
    void restore(const QList<Frame> &frames);

signals:
    void countChanged();

private:
    static quint64 nextVersion();

    QList<Frame> m_frames;
};
