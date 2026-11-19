#pragma once

#include <QQuickImageProvider>

class Backend;

// Timeline thumbnails, as image://frames/<version>. A frame's version changes
// with every edit, so QML's image cache never shows a stale drawing.
class FrameImageProvider : public QQuickImageProvider {
public:
    explicit FrameImageProvider(Backend *backend);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    Backend *m_backend;
};
