#include "drawingsurface.h"

#include <QCursor>
#include <QPainter>

#include "backend.h"

namespace {
constexpr qreal Margin = 28;
// Classic animation-desk colors: the previous drawing in red, the next in green.
const QColor BeforeTint(0xe5, 0x48, 0x4d);
const QColor AfterTint(0x30, 0xa4, 0x6c);
}

DrawingSurface::DrawingSurface(QQuickItem *parent) : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
    setCursor(Qt::CrossCursor);
}

void DrawingSurface::setBackend(Backend *backend) {
    if (backend == m_backend)
        return;
    if (m_backend)
        disconnect(m_backend, nullptr, this, nullptr);
    m_backend = backend;
    if (m_backend) {
        auto repaint = [this] { update(); };
        for (auto signal : {&Backend::canvasChanged, &Backend::currentFrameChanged, &Backend::framesChanged,
                            &Backend::onionSkinChanged, &Backend::playingChanged, &Backend::toolChanged,
                            &Backend::documentChanged})
            connect(m_backend, signal, this, repaint);
    }
    emit backendChanged();
    update();
}

QRectF DrawingSurface::paperRect() const {
    if (!m_backend)
        return {};
    const QSizeF canvas = m_backend->canvasSize();
    const QRectF room = boundingRect().adjusted(Margin, Margin, -Margin, -Margin);
    if (room.width() <= 0 || room.height() <= 0)
        return {};
    const qreal scale = std::min(room.width() / canvas.width(), room.height() / canvas.height());
    const QSizeF size = canvas * scale;
    return QRectF(room.center() - QPointF(size.width() / 2, size.height() / 2), size);
}

QPointF DrawingSurface::toCanvas(const QPointF &point) const {
    const QRectF paper = paperRect();
    const qreal scale = m_backend->canvasSize().width() / paper.width();
    return (point - paper.topLeft()) * scale;
}

QImage DrawingSurface::onion(int index, const QColor &tint, QImage &cache, quint64 &cachedVersion) const {
    const quint64 version = m_backend->frames()->version(index);
    if (version != cachedVersion) {
        // Recolor every inked pixel, keeping its coverage, so the ghost reads
        // as a guide rather than as part of the drawing.
        cache = m_backend->frames()->image(index).copy();
        QPainter painter(&cache);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(cache.rect(), tint);
        cachedVersion = version;
    }
    return cache;
}

void DrawingSurface::paint(QPainter *painter) {
    if (!m_backend)
        return;
    const QRectF paper = paperRect();
    if (paper.isEmpty())
        return;

    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->setRenderHint(QPainter::Antialiasing);

    // A soft edge lifts the paper off the dark window.
    for (int i = 1; i <= 6; ++i)
        painter->fillRect(paper.adjusted(-i, -i + 2, i, i + 2), QColor(0, 0, 0, 14));
    painter->fillRect(paper, m_backend->paper());

    const int current = m_backend->currentFrame();
    if (m_backend->onionSkin() && !m_backend->playing()) {
        if (current > 0) {
            painter->setOpacity(0.32);
            painter->drawImage(paper, onion(current - 1, BeforeTint, m_before, m_beforeVersion));
        }
        if (current + 1 < m_backend->frameCount()) {
            painter->setOpacity(0.2);
            painter->drawImage(paper, onion(current + 1, AfterTint, m_after, m_afterVersion));
        }
        painter->setOpacity(1);
    }
    painter->drawImage(paper, m_backend->frames()->image(current));

    // The brush outline, at the size it will draw, in black and white so it
    // shows on any ink.
    if (m_hovering && !m_backend->playing() && paper.contains(m_hover)) {
        const qreal radius = std::max<qreal>(1.5, m_backend->brushSize() * paper.width() / m_backend->canvasSize().width() / 2);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(QColor(0, 0, 0, 150), 2));
        painter->drawEllipse(m_hover, radius, radius);
        painter->setPen(QPen(QColor(255, 255, 255, 220), 1));
        painter->drawEllipse(m_hover, radius, radius);
    }
}

void DrawingSurface::mousePressEvent(QMouseEvent *event) {
    if (!m_backend || paperRect().isEmpty()) {
        event->ignore();
        return;
    }
    forceActiveFocus();
    m_hover = event->position();
    m_backend->beginStroke(toCanvas(event->position()), event->button() == Qt::RightButton);
    event->accept();
}

void DrawingSurface::mouseMoveEvent(QMouseEvent *event) {
    if (!m_backend)
        return;
    m_hover = event->position();
    m_backend->continueStroke(toCanvas(event->position()));
    update();
}

void DrawingSurface::mouseReleaseEvent(QMouseEvent *) {
    if (m_backend)
        m_backend->endStroke();
}

void DrawingSurface::hoverMoveEvent(QHoverEvent *event) {
    m_hover = event->position();
    m_hovering = true;
    update();
}

void DrawingSurface::hoverLeaveEvent(QHoverEvent *) {
    m_hovering = false;
    update();
}
