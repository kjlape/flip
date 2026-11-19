#pragma once

// The file picker interface follows Omacut (MIT, David Heinemeier Hansson),
// so tests can hand the backend a fake one.

#include <QObject>
#include <QString>

class FilePicker : public QObject {
    Q_OBJECT

public:
    explicit FilePicker(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~FilePicker() = default;

    virtual void openAnimation(const QString &folder) = 0;
    virtual void saveAnimation(const QString &suggestedPath) = 0;
    virtual void exportAnimation(const QString &suggestedPath) = 0;

signals:
    void openSelected(const QString &path);
    void saveSelected(const QString &path);
    // The extension of the filter the user picked ("gif" or "mp4"), for a
    // name typed without one.
    void exportSelected(const QString &path, const QString &extension);
    void failed(const QString &message);
};
