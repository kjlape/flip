#pragma once

#include <QColor>
#include <QImage>
#include <QList>
#include <QSize>
#include <QString>

// A flipbook: equally sized, transparent frames drawn over one paper color.
// The GUI, the exporter, and the command line all read and write this.
struct Document {
    static constexpr int DefaultWidth = 1280;
    static constexpr int DefaultHeight = 720;
    static constexpr int DefaultFps = 12;
    static constexpr int MinFps = 1;
    static constexpr int MaxFps = 60;
    static constexpr int MaxSide = 4096;
    static constexpr int MaxFrames = 5000;

    QSize size{DefaultWidth, DefaultHeight};
    int fps = DefaultFps;
    QColor paper{255, 255, 255};
    QList<QImage> frames;

    // One empty frame on white paper.
    static Document blank(const QSize &size = {DefaultWidth, DefaultHeight});
    static QImage blankFrame(const QSize &size);
    // A frame drawn over the paper, opaque, as it is shown and exported.
    static QImage composite(const QImage &frame, const QColor &paper);
    QImage composite(int index) const { return composite(frames.value(index), paper); }
    double duration() const { return fps > 0 ? double(frames.size()) / fps : 0; }

    // Reading leaves the document untouched when the file can't be used.
    bool load(const QString &path, QString *error);
    // Written beside the target and renamed over it, so a crash never leaves
    // half a file behind.
    bool save(const QString &path, QString *error) const;
};
