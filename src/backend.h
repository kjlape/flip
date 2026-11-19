#pragma once

#include <QColor>
#include <QObject>
#include <QPointF>
#include <QSize>
#include <QTimer>

#include "document.h"
#include "exporter.h"
#include "frames.h"

class FilePicker;

// The open animation and everything done to it: drawing, frames, undo,
// playback, saving, and exporting. QML only lays it out.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(Frames *frames READ frames CONSTANT)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY framesChanged)
    Q_PROPERTY(int currentFrame READ currentFrame WRITE goTo NOTIFY currentFrameChanged)
    Q_PROPERTY(int fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool onionSkin READ onionSkin WRITE setOnionSkin NOTIFY onionSkinChanged)
    Q_PROPERTY(bool erasing READ erasing WRITE setErasing NOTIFY toolChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY toolChanged)
    Q_PROPERTY(int brushSize READ brushSize WRITE setBrushSize NOTIFY toolChanged)
    Q_PROPERTY(QSize canvasSize READ canvasSize NOTIFY documentChanged)
    Q_PROPERTY(QColor paper READ paper NOTIFY documentChanged)
    Q_PROPERTY(QString title READ title NOTIFY documentChanged)
    Q_PROPERTY(QString filePath READ filePath NOTIFY documentChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(bool exporting READ exporting NOTIFY exportingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    static constexpr int MinBrush = 1;
    static constexpr int MaxBrush = 160;
    static constexpr int HistoryLimit = 200;

    explicit Backend(FilePicker *picker = nullptr, QObject *parent = nullptr);

    Frames *frames() { return &m_frames; }
    const Frames *frames() const { return &m_frames; }
    int frameCount() const { return m_frames.count(); }
    int currentFrame() const { return m_current; }
    int fps() const { return m_fps; }
    bool playing() const { return m_playback.isActive(); }
    bool onionSkin() const { return m_onionSkin; }
    bool erasing() const { return m_erasing; }
    QColor color() const { return m_color; }
    int brushSize() const { return m_brushSize; }
    QSize canvasSize() const { return m_size; }
    QColor paper() const { return m_paper; }
    QString title() const;
    QString filePath() const { return m_path; }
    bool dirty() const { return m_dirty; }
    bool canUndo() const { return !m_undo.isEmpty(); }
    bool canRedo() const { return !m_redo.isEmpty(); }
    bool exporting() const { return m_exporter.running(); }
    QString error() const { return m_error; }
    QString status() const { return m_status; }

    void setFps(int fps);
    void setPlaying(bool playing);
    void setOnionSkin(bool on);
    void setErasing(bool erasing);
    void setColor(const QColor &color);
    void setBrushSize(int size);

    // Strokes are in canvas pixels; DrawingSurface maps the pointer to them.
    // The right mouse button erases whatever tool is picked.
    void beginStroke(const QPointF &point, bool erase = false);
    void continueStroke(const QPointF &point);
    void endStroke();
    bool stroking() const { return m_stroking; }

    Document document() const;
    void setDocument(const Document &document, const QString &path);

    Q_INVOKABLE void goTo(int index);
    Q_INVOKABLE void step(int delta);
    Q_INVOKABLE void togglePlay() { setPlaying(!playing()); }
    Q_INVOKABLE void newFrame();
    Q_INVOKABLE void duplicateFrame();
    Q_INVOKABLE void deleteFrame();
    Q_INVOKABLE void clearFrame();
    Q_INVOKABLE void moveFrame(int delta);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void growBrush(int steps);

    Q_INVOKABLE void newAnimation();
    Q_INVOKABLE void open();
    Q_INVOKABLE bool openPath(const QString &path);
    // Saves in place and returns true, or asks where first and returns false;
    // saved() follows either way once the file is written.
    Q_INVOKABLE bool save();
    Q_INVOKABLE void saveAs();
    Q_INVOKABLE bool savePath(const QString &path);
    Q_INVOKABLE void exportAnimation();
    Q_INVOKABLE bool exportTo(const QString &path);
    Q_INVOKABLE void dismissError() { setError({}); }

signals:
    void framesChanged();
    void currentFrameChanged();
    void fpsChanged();
    void playingChanged();
    void onionSkinChanged();
    void toolChanged();
    void documentChanged();
    void dirtyChanged();
    void historyChanged();
    void exportingChanged();
    void errorChanged();
    void statusChanged();
    // The current frame's pixels changed mid-stroke.
    void canvasChanged();
    void saved();

private:
    struct State {
        QList<Frame> frames;
        int current = 0;
    };

    void paintSegment(const QPointF &from, const QPointF &to);
    void remember();
    void restore(const State &state);
    void updateDirty();
    void setError(const QString &message);
    void setStatus(const QString &message);
    QString folder() const;
    QString suggestedPath(const QString &extension) const;
    // A typed name without an extension gets one, unless that would
    // silently replace a file the picker never asked about.
    QString withExtension(const QString &path, const QString &extension);

    FilePicker *m_picker = nullptr;
    Frames m_frames;
    Exporter m_exporter;
    QTimer m_playback;
    QTimer m_statusTimer;
    QList<State> m_undo, m_redo;

    QSize m_size{Document::DefaultWidth, Document::DefaultHeight};
    QColor m_paper{255, 255, 255};
    int m_current = 0;
    int m_fps = Document::DefaultFps;
    bool m_onionSkin = true;
    bool m_erasing = false;
    QColor m_color{0x1a, 0x1a, 0x1a};
    int m_brushSize = 6;

    bool m_stroking = false;
    bool m_strokeErases = false;
    QPointF m_last;

    QString m_path;
    QList<quint64> m_savedVersions;
    int m_savedFps = Document::DefaultFps;
    bool m_dirty = false;
    QString m_error;
    QString m_status;
};
