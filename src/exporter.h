#pragma once

#include <QObject>
#include <QProcess>
#include <optional>

#include "document.h"

// Turns an animation into a GIF or MP4 by streaming raw frames into ffmpeg.
// The result is written beside the target and renamed over it when ffmpeg
// succeeds, so a failed export never replaces a good file.
class Exporter : public QObject {
    Q_OBJECT

public:
    enum class Format { Gif, Mp4 };

    explicit Exporter(QObject *parent = nullptr);
    ~Exporter() override;

    static std::optional<Format> formatFor(const QString &path);
    static QString ffmpeg();
    static QStringList arguments(Format format, const Document &document, const QString &output);

    // False, with error() set, when the export can't begin.
    bool start(const Document &document, const QString &target);
    bool running() const { return m_process.state() != QProcess::NotRunning; }
    QString error() const { return m_error; }

    // Runs an export to the end; for the command line.
    bool run(const Document &document, const QString &target);

signals:
    // On success the message is the target path, otherwise what went wrong.
    void finished(bool ok, const QString &message);

private:
    void feed();
    void fail(const QString &message);

    QProcess m_process;
    Document m_document;
    QString m_target;
    QString m_partial;
    QString m_error;
    int m_next = 0;
    bool m_closed = false;
};
