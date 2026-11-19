#include "backend.h"

#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QStandardPaths>
#include <cmath>

#include "filepicker.h"

Backend::Backend(FilePicker *picker, QObject *parent) : QObject(parent), m_picker(picker) {
    connect(&m_frames, &Frames::countChanged, this, &Backend::framesChanged);
    connect(&m_playback, &QTimer::timeout, this, [this] { step(1); });
    m_playback.setTimerType(Qt::PreciseTimer);
    m_playback.setInterval(1000 / m_fps);
    m_statusTimer.setSingleShot(true);
    m_statusTimer.setInterval(3500);
    connect(&m_statusTimer, &QTimer::timeout, this, [this] { setStatus({}); });

    connect(&m_exporter, &Exporter::finished, this, [this](bool ok, const QString &message) {
        emit exportingChanged();
        if (ok)
            setStatus(tr("Exported %1").arg(QFileInfo(message).fileName()));
        else
            setError(message);
    });

    if (m_picker) {
        connect(m_picker, &FilePicker::openSelected, this, &Backend::openPath);
        connect(m_picker, &FilePicker::saveSelected, this, [this](const QString &path) {
            const QString target = withExtension(path, "flip");
            if (!target.isEmpty())
                savePath(target);
        });
        connect(m_picker, &FilePicker::exportSelected, this, [this](const QString &path, const QString &extension) {
            const QString target = Exporter::formatFor(path) ? path : withExtension(path, extension);
            if (!target.isEmpty())
                exportTo(target);
        });
        connect(m_picker, &FilePicker::failed, this, &Backend::setError);
    }

    setDocument(Document::blank(), {});
}

QString Backend::title() const { return m_path.isEmpty() ? tr("Untitled") : QFileInfo(m_path).completeBaseName(); }

void Backend::setFps(int fps) {
    fps = std::clamp(fps, Document::MinFps, Document::MaxFps);
    if (fps == m_fps)
        return;
    m_fps = fps;
    m_playback.setInterval(int(std::lround(1000.0 / fps)));
    emit fpsChanged();
    updateDirty();
}

void Backend::setPlaying(bool playing) {
    if (playing == this->playing())
        return;
    if (playing)
        m_playback.start();
    else
        m_playback.stop();
    emit playingChanged();
}

void Backend::setOnionSkin(bool on) {
    if (on == m_onionSkin)
        return;
    m_onionSkin = on;
    emit onionSkinChanged();
}

void Backend::setErasing(bool erasing) {
    if (erasing == m_erasing)
        return;
    m_erasing = erasing;
    emit toolChanged();
}

void Backend::setColor(const QColor &color) {
    // Picking a color means drawing with it.
    if (color == m_color && !m_erasing)
        return;
    m_color = color;
    m_erasing = false;
    emit toolChanged();
}

void Backend::setBrushSize(int size) {
    size = std::clamp(size, MinBrush, MaxBrush);
    if (size == m_brushSize)
        return;
    m_brushSize = size;
    emit toolChanged();
}

void Backend::growBrush(int steps) {
    // Steps grow with the brush, so both fine lines and fat fills are a few
    // presses away.
    const int step = std::max(1, m_brushSize / 6);
    setBrushSize(m_brushSize + steps * step);
}

void Backend::beginStroke(const QPointF &point, bool erase) {
    if (m_stroking)
        endStroke();
    setPlaying(false);
    remember();
    m_stroking = true;
    m_strokeErases = erase || m_erasing;
    m_last = point;
    paintSegment(point, point);
}

void Backend::continueStroke(const QPointF &point) {
    if (!m_stroking || point == m_last)
        return;
    paintSegment(m_last, point);
    m_last = point;
}

void Backend::endStroke() {
    if (!m_stroking)
        return;
    m_stroking = false;
    m_frames.touch(m_current);
    updateDirty();
}

void Backend::paintSegment(const QPointF &from, const QPointF &to) {
    QPainter painter(&m_frames.edit(m_current));
    painter.setRenderHint(QPainter::Antialiasing);
    // Erasing removes ink and lets the paper show; its soft edge comes from
    // the same antialiasing as the pen's.
    if (m_strokeErases)
        painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    painter.setPen(QPen(m_strokeErases ? QColor(Qt::black) : m_color, m_brushSize, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    if (from == to)
        painter.drawPoint(from);
    else
        painter.drawLine(from, to);
    painter.end();
    emit canvasChanged();
}

void Backend::goTo(int index) {
    index = std::clamp(index, 0, m_frames.count() - 1);
    if (index == m_current)
        return;
    if (m_stroking)
        endStroke();
    m_current = index;
    emit currentFrameChanged();
}

void Backend::step(int delta) {
    const int count = m_frames.count();
    goTo(((m_current + delta) % count + count) % count);
}

void Backend::newFrame() {
    setPlaying(false);
    remember();
    m_frames.insert(m_current + 1, Document::blankFrame(m_size));
    goTo(m_current + 1);
    updateDirty();
}

void Backend::duplicateFrame() {
    setPlaying(false);
    remember();
    m_frames.insert(m_current + 1, m_frames.image(m_current));
    goTo(m_current + 1);
    updateDirty();
}

void Backend::deleteFrame() {
    // An animation always has a frame to draw on; the last one is cleared.
    if (m_frames.count() == 1) {
        clearFrame();
        return;
    }
    setPlaying(false);
    remember();
    m_frames.remove(m_current);
    if (m_current >= m_frames.count()) {
        m_current = m_frames.count() - 1;
    }
    emit currentFrameChanged();
    updateDirty();
}

void Backend::clearFrame() {
    setPlaying(false);
    remember();
    m_frames.edit(m_current) = Document::blankFrame(m_size);
    m_frames.touch(m_current);
    emit canvasChanged();
    updateDirty();
}

void Backend::moveFrame(int delta) {
    const int target = m_current + delta;
    if (target < 0 || target >= m_frames.count())
        return;
    setPlaying(false);
    remember();
    m_frames.move(m_current, target);
    m_current = target;
    emit currentFrameChanged();
    updateDirty();
}

void Backend::remember() {
    m_undo.append({m_frames.all(), m_current});
    if (m_undo.size() > HistoryLimit)
        m_undo.removeFirst();
    m_redo.clear();
    emit historyChanged();
}

void Backend::restore(const State &state) {
    setPlaying(false);
    m_frames.restore(state.frames);
    m_current = std::clamp(state.current, 0, m_frames.count() - 1);
    emit currentFrameChanged();
    emit canvasChanged();
    emit historyChanged();
    updateDirty();
}

void Backend::undo() {
    if (m_stroking || m_undo.isEmpty())
        return;
    m_redo.append({m_frames.all(), m_current});
    restore(m_undo.takeLast());
}

void Backend::redo() {
    if (m_stroking || m_redo.isEmpty())
        return;
    m_undo.append({m_frames.all(), m_current});
    restore(m_redo.takeLast());
}

void Backend::updateDirty() {
    // Undoing back to what's on disk makes the animation clean again.
    const bool dirty = m_frames.versions() != m_savedVersions || m_fps != m_savedFps;
    if (dirty == m_dirty)
        return;
    m_dirty = dirty;
    emit dirtyChanged();
}

Document Backend::document() const {
    Document document;
    document.size = m_size;
    document.fps = m_fps;
    document.paper = m_paper;
    document.frames = m_frames.images();
    return document;
}

void Backend::setDocument(const Document &document, const QString &path) {
    setPlaying(false);
    m_stroking = false;
    m_size = document.size;
    m_paper = document.paper;
    m_frames.reset(document.frames.isEmpty() ? QList<QImage>{Document::blankFrame(m_size)} : document.frames);
    m_current = 0;
    m_path = path;
    m_undo.clear();
    m_redo.clear();
    m_savedVersions = m_frames.versions();
    if (document.fps != m_fps) {
        m_fps = document.fps;
        m_playback.setInterval(int(std::lround(1000.0 / m_fps)));
        emit fpsChanged();
    }
    m_savedFps = m_fps;
    emit documentChanged();
    emit currentFrameChanged();
    emit canvasChanged();
    emit historyChanged();
    updateDirty();
}

void Backend::newAnimation() { setDocument(Document::blank(m_size), {}); }

QString Backend::folder() const {
    if (!m_path.isEmpty())
        return QFileInfo(m_path).absolutePath();
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    return QDir(pictures).exists() ? pictures : QDir::homePath();
}

QString Backend::suggestedPath(const QString &extension) const {
    const QString name = m_path.isEmpty() ? tr("animation") : QFileInfo(m_path).completeBaseName();
    return QDir(folder()).filePath(name + "." + extension);
}

QString Backend::withExtension(const QString &path, const QString &extension) {
    if (!QFileInfo(path).suffix().isEmpty())
        return path;
    const QString target = path + "." + extension;
    if (QFileInfo::exists(target)) {
        setError(tr("%1 already exists. Pick another name, or type the full name to replace it.")
                     .arg(QFileInfo(target).fileName()));
        return {};
    }
    return target;
}

void Backend::open() {
    if (!m_picker) {
        setError(tr("There is no file picker."));
        return;
    }
    m_picker->openAnimation(folder());
}

bool Backend::openPath(const QString &path) {
    Document document;
    QString error;
    if (!document.load(path, &error)) {
        setError(error);
        return false;
    }
    setDocument(document, QFileInfo(path).absoluteFilePath());
    setError({});
    return true;
}

bool Backend::save() {
    if (m_path.isEmpty()) {
        saveAs();
        return false;
    }
    return savePath(m_path);
}

void Backend::saveAs() {
    if (!m_picker) {
        setError(tr("There is no file picker."));
        return;
    }
    m_picker->saveAnimation(suggestedPath("flip"));
}

bool Backend::savePath(const QString &path) {
    if (m_stroking)
        endStroke();
    QString error;
    if (!document().save(path, &error)) {
        setError(error);
        return false;
    }
    const bool renamed = QFileInfo(path).absoluteFilePath() != m_path;
    m_path = QFileInfo(path).absoluteFilePath();
    m_savedVersions = m_frames.versions();
    m_savedFps = m_fps;
    if (renamed)
        emit documentChanged();
    updateDirty();
    setStatus(tr("Saved %1").arg(QFileInfo(path).fileName()));
    emit saved();
    return true;
}

void Backend::exportAnimation() {
    if (Exporter::ffmpeg().isEmpty()) {
        setError(tr("ffmpeg is not installed, so Flip can't export. Install it with: omarchy-pkg-add ffmpeg"));
        return;
    }
    if (!m_picker) {
        setError(tr("There is no file picker."));
        return;
    }
    m_picker->exportAnimation(suggestedPath("gif"));
}

bool Backend::exportTo(const QString &path) {
    if (m_stroking)
        endStroke();
    setPlaying(false);
    if (!m_exporter.start(document(), path)) {
        setError(m_exporter.error());
        return false;
    }
    setError({});
    setStatus(tr("Exporting %1…").arg(QFileInfo(path).fileName()));
    m_statusTimer.stop();
    emit exportingChanged();
    return true;
}

void Backend::setError(const QString &message) {
    if (message == m_error)
        return;
    m_error = message;
    emit errorChanged();
}

void Backend::setStatus(const QString &message) {
    m_status = message;
    if (!message.isEmpty())
        m_statusTimer.start();
    emit statusChanged();
}
