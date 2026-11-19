#include "exporter.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <cstdio>

namespace {
// Enough queued frames to keep ffmpeg busy without holding the whole
// animation in memory twice.
constexpr qint64 QueueLimit = 16 * 1024 * 1024;
}

Exporter::Exporter(QObject *parent) : QObject(parent) {
    connect(&m_process, &QProcess::started, this, &Exporter::feed);
    connect(&m_process, &QProcess::bytesWritten, this, &Exporter::feed);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            fail(tr("Could not start ffmpeg: %1").arg(m_process.errorString()));
    });
    connect(&m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        if (status != QProcess::NormalExit || code != 0) {
            const QStringList lines = QString::fromUtf8(m_process.readAllStandardError()).trimmed().split('\n');
            fail(lines.last().isEmpty() ? tr("ffmpeg stopped with exit code %1.").arg(code)
                                        : tr("ffmpeg failed: %1").arg(lines.last().trimmed()));
            return;
        }
        // rename(2) replaces the target in one step.
        if (std::rename(QFile::encodeName(m_partial).constData(), QFile::encodeName(m_target).constData()) != 0) {
            fail(tr("Could not write %1.").arg(m_target));
            return;
        }
        m_document = {};
        emit finished(true, m_target);
    });
}

Exporter::~Exporter() {
    if (running()) {
        m_process.kill();
        m_process.waitForFinished(1000);
        QFile::remove(m_partial);
    }
}

std::optional<Exporter::Format> Exporter::formatFor(const QString &path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == "gif")
        return Format::Gif;
    if (suffix == "mp4")
        return Format::Mp4;
    return std::nullopt;
}

QString Exporter::ffmpeg() { return QStandardPaths::findExecutable("ffmpeg"); }

QStringList Exporter::arguments(Format format, const Document &document, const QString &output) {
    // Composited frames are 32-bit RGB, stored blue-green-red-alpha in memory.
    QStringList arguments{"-hide_banner", "-loglevel", "error", "-nostats", "-y",
                          "-f", "rawvideo", "-pix_fmt", "bgra",
                          "-video_size", QString("%1x%2").arg(document.size.width()).arg(document.size.height()),
                          "-framerate", QString::number(document.fps), "-i", "-"};
    if (format == Format::Gif)
        // One palette for the whole animation keeps colors steady between frames.
        arguments << "-filter_complex"
                  << "split[a][b];[a]palettegen=stats_mode=diff[p];[b][p]paletteuse=dither=bayer:bayer_scale=5:diff_mode=rectangle"
                  << "-loop" << "0" << "-f" << "gif";
    else
        // H.264 in 4:2:0 needs even dimensions, and plays everywhere.
        arguments << "-vf" << "scale=trunc(iw/2)*2:trunc(ih/2)*2" << "-c:v" << "libx264" << "-pix_fmt" << "yuv420p"
                  << "-crf" << "18" << "-movflags" << "+faststart" << "-f" << "mp4";
    return arguments << output;
}

bool Exporter::start(const Document &document, const QString &target) {
    m_error.clear();
    if (running()) {
        m_error = tr("An export is already running.");
        return false;
    }
    const auto format = formatFor(target);
    if (!format) {
        m_error = tr("Export to a .gif or .mp4 file.");
        return false;
    }
    const QString program = ffmpeg();
    if (program.isEmpty()) {
        m_error = tr("ffmpeg is not installed, so Flip can't export. Install it with: omarchy-pkg-add ffmpeg");
        return false;
    }
    if (document.frames.isEmpty()) {
        m_error = tr("There are no frames to export.");
        return false;
    }

    const QFileInfo info(target);
    m_document = document;
    m_target = info.absoluteFilePath();
    m_partial = info.absoluteDir().filePath("." + info.fileName() + ".part");
    m_next = 0;
    m_closed = false;
    m_process.start(program, arguments(*format, document, m_partial));
    return true;
}

bool Exporter::run(const Document &document, const QString &target) {
    if (!start(document, target))
        return false;
    bool ok = false;
    QEventLoop loop;
    connect(this, &Exporter::finished, &loop, [&](bool success) {
        ok = success;
        loop.quit();
    });
    loop.exec();
    return ok;
}

void Exporter::feed() {
    if (m_closed || m_process.state() != QProcess::Running)
        return;
    while (m_next < m_document.frames.size() && m_process.bytesToWrite() < QueueLimit) {
        const QImage frame = m_document.composite(m_next++);
        m_process.write(reinterpret_cast<const char *>(frame.constBits()), frame.sizeInBytes());
    }
    if (m_next >= m_document.frames.size()) {
        // Anything still buffered is written before the channel closes.
        m_closed = true;
        m_process.closeWriteChannel();
    }
}

void Exporter::fail(const QString &message) {
    if (!m_partial.isEmpty())
        QFile::remove(m_partial);
    m_document = {};
    m_error = message;
    emit finished(false, message);
}
