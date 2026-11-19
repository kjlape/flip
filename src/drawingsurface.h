#pragma once

#include <QImage>
#include <QPointer>
#include <QQuickPaintedItem>

#include "backend.h"

// The paper you draw on: the current frame fitted into the item, with the
// frames before and after it showing faintly (the onion skin) while you draw.
class DrawingSurface : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(Backend *animation READ backend WRITE setBackend NOTIFY backendChanged)

public:
    explicit DrawingSurface(QQuickItem *parent = nullptr);

    Backend *backend() const { return m_backend; }
    void setBackend(Backend *backend);

    void paint(QPainter *painter) override;
    // Where the canvas sits in the item, in item coordinates.
    QRectF paperRect() const;

signals:
    void backendChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

private:
    QPointF toCanvas(const QPointF &point) const;
    QImage onion(int index, const QColor &tint, QImage &cache, quint64 &cachedVersion) const;

    QPointer<Backend> m_backend;
    QPointF m_hover;
    bool m_hovering = false;
    mutable QImage m_before, m_after;
    mutable quint64 m_beforeVersion = 0, m_afterVersion = 0;
};
