#include "document.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDataStream>
#include <QFile>
#include <QPainter>
#include <QSaveFile>

namespace {
// "FLIP", then a format version, so later versions can still read old files.
constexpr quint32 Magic = 0x464C4950;
constexpr quint16 Version = 1;

QString tr(const char *text) { return QCoreApplication::translate("Document", text); }
}

Document Document::blank(const QSize &size) {
    Document document;
    document.size = size;
    document.frames = {blankFrame(size)};
    return document;
}

QImage Document::blankFrame(const QSize &size) {
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

QImage Document::composite(const QImage &frame, const QColor &paper) {
    QImage image(frame.size(), QImage::Format_RGB32);
    image.fill(paper);
    QPainter painter(&image);
    painter.drawImage(0, 0, frame);
    return image;
}

bool Document::save(const QString &path, QString *error) const {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = tr("Could not write %1: %2").arg(path, file.errorString());
        return false;
    }
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << Magic << Version << qint32(size.width()) << qint32(size.height()) << qint32(fps)
           << quint32(paper.rgb()) << qint32(frames.size());
    for (const QImage &frame : frames) {
        // PNG keeps each frame small and lossless, with its transparency.
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        frame.save(&buffer, "PNG");
        stream << png;
    }
    if (stream.status() != QDataStream::Ok || !file.commit()) {
        if (error)
            *error = tr("Could not write %1: %2").arg(path, file.errorString());
        return false;
    }
    return true;
}

bool Document::load(const QString &path, QString *error) {
    auto fail = [&](const QString &message) {
        if (error)
            *error = message;
        return false;
    };

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return fail(tr("Could not open %1: %2").arg(path, file.errorString()));

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_6_0);
    quint32 magic = 0;
    quint16 version = 0;
    stream >> magic >> version;
    if (magic != Magic)
        return fail(tr("%1 is not a Flip animation.").arg(path));
    if (version > Version)
        return fail(tr("%1 was made by a newer version of Flip.").arg(path));

    qint32 width = 0, height = 0, rate = 0, count = 0;
    quint32 rgb = 0;
    stream >> width >> height >> rate >> rgb >> count;
    if (stream.status() != QDataStream::Ok || width < 1 || height < 1 || width > MaxSide || height > MaxSide
        || rate < MinFps || rate > MaxFps || count < 1 || count > MaxFrames)
        return fail(tr("%1 is damaged.").arg(path));

    Document loaded;
    loaded.size = QSize(width, height);
    loaded.fps = rate;
    loaded.paper = QColor::fromRgb(rgb);
    for (int i = 0; i < count; ++i) {
        QByteArray png;
        stream >> png;
        QImage frame;
        if (stream.status() != QDataStream::Ok || !frame.loadFromData(png, "PNG") || frame.size() != loaded.size)
            return fail(tr("%1 is damaged.").arg(path));
        loaded.frames.append(frame.convertToFormat(QImage::Format_ARGB32_Premultiplied));
    }

    *this = loaded;
    return true;
}
