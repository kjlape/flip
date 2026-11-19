#include "frameimageprovider.h"

#include "backend.h"
#include "document.h"

FrameImageProvider::FrameImageProvider(Backend *backend)
    : QQuickImageProvider(QQuickImageProvider::Image), m_backend(backend) {}

QImage FrameImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    const QImage frame = m_backend->frames()->imageForVersion(id.toULongLong());
    QImage image = frame.isNull() ? QImage() : Document::composite(frame, m_backend->paper());
    if (!image.isNull() && requestedSize.isValid())
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (size)
        *size = image.size();
    return image;
}
