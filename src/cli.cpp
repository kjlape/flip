// The command shape follows Hype's (MIT, David Heinemeier Hansson).
#include "cli.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QSaveFile>
#include <cstdio>

#include "document.h"
#include "exporter.h"

namespace {
void print(FILE *stream, const QString &text) { fprintf(stream, "%s\n", qPrintable(text)); }
void print(const QJsonObject &object) { fputs(QJsonDocument(object).toJson().constData(), stdout); }
int fail(const QString &message) {
    print(stderr, message);
    return 1;
}

struct Command {
    QCommandLineParser parser;
    Command(const QString &name, const QString &description) {
        parser.setApplicationDescription(description);
        parser.addHelpOption();
        parser.addPositionalArgument(name, description, name);
    }
    void json() { parser.addOption({"json", "Print the result as JSON"}); }
    void fps() { parser.addOption({"fps", "Frames per second (1-60)", "n"}); }
    bool wantsJson() const { return parser.isSet("json"); }
    QString argument(int index) const { return parser.positionalArguments().value(index); }
    // --fps, when given, must be a whole number in range.
    bool readFps(Document &document) const {
        if (!parser.isSet("fps"))
            return true;
        bool ok = false;
        const int fps = parser.value("fps").toInt(&ok);
        if (!ok || fps < Document::MinFps || fps > Document::MaxFps) {
            print(stderr, QString("--fps takes a number from %1 to %2.").arg(Document::MinFps).arg(Document::MaxFps));
            return false;
        }
        document.fps = fps;
        return true;
    }
};

bool load(const QString &path, Document &document) {
    if (path.isEmpty()) {
        print(stderr, "Name a .flip animation.");
        return false;
    }
    QString error;
    if (!document.load(path, &error)) {
        print(stderr, error);
        return false;
    }
    return true;
}

QJsonObject describe(const QString &path, const Document &document) {
    return {{"path", QFileInfo(path).absoluteFilePath()},
            {"frames", int(document.frames.size())},
            {"fps", document.fps},
            {"width", document.size.width()},
            {"height", document.size.height()},
            {"paper", document.paper.name()},
            {"seconds", document.duration()}};
}

int info(const QStringList &arguments) {
    Command command("info", "Describe an animation: frames, speed, and size.");
    command.parser.addPositionalArgument("animation", "A .flip animation", "<animation>");
    command.json();
    command.parser.process(arguments);
    Document document;
    if (!load(command.argument(1), document))
        return 1;
    const QJsonObject object = describe(command.argument(1), document);
    if (command.wantsJson())
        print(object);
    else
        print(stdout, QString("%1 frames at %2 fps (%3 s), %4x%5")
                          .arg(document.frames.size())
                          .arg(document.fps)
                          .arg(document.duration(), 0, 'f', 2)
                          .arg(document.size.width())
                          .arg(document.size.height()));
    return 0;
}

int exportAnimation(const QStringList &arguments) {
    Command command("export", "Export an animation as a looping GIF or an MP4 video.");
    command.parser.addPositionalArgument("animation", "A .flip animation", "<animation>");
    command.parser.addPositionalArgument("output", "A .gif or .mp4 file", "<output>");
    command.fps();
    command.json();
    command.parser.process(arguments);
    Document document;
    if (!load(command.argument(1), document) || !command.readFps(document))
        return 1;
    const QString output = command.argument(2);
    if (output.isEmpty())
        return fail("Name a .gif or .mp4 file to export to.");
    Exporter exporter;
    if (!exporter.run(document, output))
        return fail(exporter.error());
    if (command.wantsJson())
        print(QJsonObject{{"output", QFileInfo(output).absoluteFilePath()},
                          {"frames", int(document.frames.size())},
                          {"fps", document.fps}});
    else
        print(stdout, "Exported " + output);
    return 0;
}

int importImages(const QStringList &arguments) {
    Command command("import", "Make an animation from images, one frame each, in the order given.");
    command.parser.addPositionalArgument("animation", "The .flip animation to create", "<animation>");
    command.parser.addPositionalArgument("images", "PNG, JPEG, or other images", "<image>...");
    command.fps();
    command.parser.addOption({"force", "Replace the animation if it exists"});
    command.json();
    command.parser.process(arguments);

    const QString target = command.argument(1);
    const QStringList images = command.parser.positionalArguments().mid(2);
    if (target.isEmpty() || images.isEmpty())
        return fail("Name the .flip animation to create and at least one image.");
    if (QFileInfo(target).suffix().toLower() != "flip")
        return fail(target + " must end in .flip.");
    if (QFileInfo::exists(target) && !command.parser.isSet("force"))
        return fail(target + " exists; pass --force to replace it.");

    Document document;
    if (!command.readFps(document))
        return 1;
    // The first image sets the size; the rest are fitted inside it, centered.
    for (const QString &path : images) {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        const QImage image = reader.read();
        if (image.isNull())
            return fail(QString("Could not read %1: %2").arg(path, reader.errorString()));
        if (document.frames.isEmpty()) {
            if (image.width() > Document::MaxSide || image.height() > Document::MaxSide)
                return fail(QString("%1 is larger than %2 pixels on a side.").arg(path).arg(Document::MaxSide));
            document.size = image.size();
        }
        if (document.frames.size() >= Document::MaxFrames)
            return fail(QString("An animation holds at most %1 frames.").arg(Document::MaxFrames));
        QImage frame = Document::blankFrame(document.size);
        QPainter painter(&frame);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QSize fitted = image.size().scaled(document.size, Qt::KeepAspectRatio);
        painter.drawImage(QRect(QPoint((document.size.width() - fitted.width()) / 2,
                                       (document.size.height() - fitted.height()) / 2),
                                fitted),
                          image);
        painter.end();
        document.frames.append(frame);
    }

    QString error;
    if (!document.save(target, &error))
        return fail(error);
    if (command.wantsJson())
        print(describe(target, document));
    else
        print(stdout, QString("Created %1 with %2 frames").arg(target).arg(document.frames.size()));
    return 0;
}

int frames(const QStringList &arguments) {
    Command command("frames", "Write every frame as a transparent PNG (0001.png, 0002.png, ...).");
    command.parser.addPositionalArgument("animation", "A .flip animation", "<animation>");
    command.parser.addPositionalArgument("directory", "Where to write the PNGs", "<directory>");
    command.parser.addOption({"paper", "Draw the frames on the paper color instead of transparency"});
    command.json();
    command.parser.process(arguments);
    Document document;
    if (!load(command.argument(1), document))
        return 1;
    const QString directory = command.argument(2);
    if (directory.isEmpty())
        return fail("Name a directory to write the frames to.");
    if (!QDir().mkpath(directory))
        return fail("Could not create " + directory);

    QJsonArray written;
    for (int i = 0; i < document.frames.size(); ++i) {
        const QString path = QDir(directory).filePath(QString("%1.png").arg(i + 1, 4, 10, QChar('0')));
        QSaveFile file(path);
        const QImage image = command.parser.isSet("paper") ? document.composite(i) : document.frames[i];
        if (!file.open(QIODevice::WriteOnly) || !image.save(&file, "PNG") || !file.commit())
            return fail("Could not write " + path);
        written.append(QFileInfo(path).absoluteFilePath());
    }
    if (command.wantsJson())
        print(QJsonObject{{"frames", written}});
    else
        print(stdout, QString("Wrote %1 frames to %2").arg(written.size()).arg(directory));
    return 0;
}

int skill(const QStringList &arguments) {
    Command command("skill", "Print the agent skill, or install it for your coding agents with: skill install.");
    command.parser.addPositionalArgument("install", "Copy to ~/.agents/skills/flip and link into ~/.claude/skills",
                                         "[install]");
    command.parser.process(arguments);
    QFile bundled(":/skill.md");
    if (!bundled.open(QIODevice::ReadOnly))
        return 1;
    const QByteArray text = bundled.readAll();
    if (command.argument(1).isEmpty()) {
        fputs(text.constData(), stdout);
        return 0;
    }
    if (command.argument(1) != "install")
        return fail("Use flip skill to print the skill, or flip skill install.");
    const QDir home = QDir::home();
    const QString directory = home.filePath(".agents/skills/flip");
    QSaveFile file(directory + "/SKILL.md");
    if (!QDir().mkpath(directory) || !file.open(QIODevice::WriteOnly) || file.write(text) < 0 || !file.commit())
        return fail("Could not write " + file.fileName());
    print(stdout, "Installed " + file.fileName());
    if (!home.exists(".claude"))
        return 0;
    const QString link = home.filePath(".claude/skills/flip");
    const QFileInfo existing(link);
    if (existing.isSymLink())
        QFile::remove(link);
    else if (existing.exists())
        return fail(link + " exists and is not a link; leaving it alone.");
    if (!home.mkpath(".claude/skills") || !QFile::link("../../.agents/skills/flip", link))
        return fail("Could not link " + link);
    print(stdout, "Linked " + link);
    return 0;
}

int help(const QStringList &arguments) {
    const QString topic = arguments.value(2);
    if (isCliCommand(topic) && topic != "help")
        return runCli({arguments[0], topic, "--help"});
    print(stdout, "Usage: flip <command> [options]\n\n" + cliSummary());
    return 0;
}
}

QString cliSummary() {
    return "Editor:\n"
           "  open [animation]                 Draw and animate, in a window\n\n"
           "Commands that need no display (flip help <command> for options):\n"
           "  info <animation>                 Frames, speed, and size\n"
           "  export <animation> <output>      Export a looping GIF or an MP4\n"
           "  import <animation> <image>...    Make an animation from images\n"
           "  frames <animation> <directory>   Write every frame as a PNG\n"
           "  skill [install]                  Print the skill for coding agents, or install it";
}

bool isCliCommand(const QString &word) {
    return QStringList{"info", "export", "import", "frames", "skill", "help", "--help", "-h", "--version", "-v"}
        .contains(word);
}

int runCli(const QStringList &arguments) {
    const QString command = arguments.value(1);
    if (command == "--version" || command == "-v") {
        print(stdout, "flip " FLIP_VERSION);
        return 0;
    }
    return command == "info"     ? info(arguments)
           : command == "export" ? exportAnimation(arguments)
           : command == "import" ? importImages(arguments)
           : command == "frames" ? frames(arguments)
           : command == "skill"  ? skill(arguments)
                                 : help(arguments);
}
