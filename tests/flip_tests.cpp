#include <QDir>
#include <QFile>
#include <QPainter>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "document.h"
#include "exporter.h"
#include "filepicker.h"
#include "theme.h"
#include "translator.h"

// Answers every dialog with a path chosen by the test.
class FakePicker : public FilePicker {
public:
    QString answer;
    QString extension = "gif";
    void openAnimation(const QString &) override { emit openSelected(answer); }
    void saveAnimation(const QString &) override { emit saveSelected(answer); }
    void exportAnimation(const QString &) override { emit exportSelected(answer, extension); }
};

namespace {
QRgb pixel(const Backend &backend, int frame, int x, int y) { return backend.frames()->image(frame).pixel(x, y); }
void dot(Backend &backend, QPointF at) {
    backend.beginStroke(at);
    backend.endStroke();
}
}

class FlipTests : public QObject {
    Q_OBJECT

private slots:
    void startsWithOneBlankFrame()
    {
        Backend backend;
        QCOMPARE(backend.frameCount(), 1);
        QCOMPARE(backend.canvasSize(), QSize(1280, 720));
        QCOMPARE(qAlpha(pixel(backend, 0, 10, 10)), 0);
        QVERIFY(!backend.dirty());
    }

    void strokesInkTheCurrentFrame()
    {
        Backend backend;
        backend.setColor(QColor("#ff0000"));
        backend.setBrushSize(10);
        backend.beginStroke({100, 100});
        backend.continueStroke({200, 100});
        backend.endStroke();
        QCOMPARE(QColor(pixel(backend, 0, 150, 100)), QColor("#ff0000"));
        QCOMPARE(qAlpha(pixel(backend, 0, 150, 130)), 0);
        QVERIFY(backend.dirty());
        QVERIFY(backend.canUndo());
    }

    void erasingClearsInk()
    {
        Backend backend;
        backend.setBrushSize(20);
        dot(backend, {50, 50});
        QCOMPARE(qAlpha(pixel(backend, 0, 50, 50)), 255);
        backend.beginStroke({50, 50}, true);
        backend.endStroke();
        QCOMPARE(qAlpha(pixel(backend, 0, 50, 50)), 0);
        backend.setErasing(true);
        QVERIFY(backend.erasing());
        backend.setColor(Qt::blue);
        QVERIFY(!backend.erasing());
    }

    void undoAndRedoRestoreFramesAndCleanliness()
    {
        Backend backend;
        dot(backend, {10, 10});
        backend.newFrame();
        QCOMPARE(backend.frameCount(), 2);
        QCOMPARE(backend.currentFrame(), 1);
        backend.undo();
        QCOMPARE(backend.frameCount(), 1);
        QCOMPARE(backend.currentFrame(), 0);
        backend.undo();
        QCOMPARE(qAlpha(pixel(backend, 0, 10, 10)), 0);
        QVERIFY(!backend.dirty());
        backend.redo();
        QCOMPARE(qAlpha(pixel(backend, 0, 10, 10)), 255);
        QVERIFY(backend.dirty());
        backend.redo();
        QCOMPARE(backend.frameCount(), 2);
        QVERIFY(!backend.canRedo());
    }

    void framesDuplicateMoveAndDelete()
    {
        Backend backend;
        dot(backend, {10, 10});
        backend.duplicateFrame();
        QCOMPARE(backend.frameCount(), 2);
        QCOMPARE(qAlpha(pixel(backend, 1, 10, 10)), 255);
        backend.newFrame();
        dot(backend, {30, 30});
        QCOMPARE(backend.currentFrame(), 2);
        backend.moveFrame(-1);
        QCOMPARE(backend.currentFrame(), 1);
        QCOMPARE(qAlpha(pixel(backend, 1, 30, 30)), 255);
        backend.moveFrame(-5);
        QCOMPARE(backend.currentFrame(), 1);
        backend.deleteFrame();
        QCOMPARE(backend.frameCount(), 2);
        QCOMPARE(qAlpha(pixel(backend, 1, 30, 30)), 0);
        backend.deleteFrame();
        backend.deleteFrame();
        // The last frame is cleared, never removed.
        QCOMPARE(backend.frameCount(), 1);
        QCOMPARE(qAlpha(pixel(backend, 0, 10, 10)), 0);
    }

    void stepWrapsAndPlaybackAdvances()
    {
        Backend backend;
        backend.newFrame();
        backend.newFrame();
        backend.step(1);
        QCOMPARE(backend.currentFrame(), 0);
        backend.step(-1);
        QCOMPARE(backend.currentFrame(), 2);
        backend.setFps(60);
        backend.setPlaying(true);
        QTRY_COMPARE(backend.currentFrame(), 1);
        dot(backend, {5, 5});
        QVERIFY(!backend.playing());
        backend.setFps(500);
        QCOMPARE(backend.fps(), 60);
    }

    void savesAndOpensAnimations()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("walk.flip");
        Backend backend;
        backend.setColor(QColor("#0090ff"));
        dot(backend, {40, 40});
        backend.newFrame();
        backend.setFps(8);
        QSignalSpy saved(&backend, &Backend::saved);
        QVERIFY(backend.savePath(path));
        QCOMPARE(saved.count(), 1);
        QVERIFY(!backend.dirty());
        QCOMPARE(backend.title(), QString("walk"));

        Backend reopened;
        QVERIFY(reopened.openPath(path));
        QCOMPARE(reopened.frameCount(), 2);
        QCOMPARE(reopened.fps(), 8);
        QCOMPARE(QColor(pixel(reopened, 0, 40, 40)), QColor("#0090ff"));
        QVERIFY(!reopened.dirty());
    }

    void savingLeavesNoTemporaryFiles()
    {
        QTemporaryDir dir;
        Document document = Document::blank({64, 32});
        QString error;
        QVERIFY(document.save(dir.filePath("a.flip"), &error));
        QVERIFY(document.save(dir.filePath("a.flip"), &error));
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden), QStringList{"a.flip"});
    }

    void rejectsFilesThatAreNotAnimations()
    {
        QTemporaryDir dir;
        write(dir.filePath("junk.flip"), "hello, not an animation");
        Backend backend;
        dot(backend, {10, 10});
        QVERIFY(!backend.openPath(dir.filePath("junk.flip")));
        QVERIFY(backend.error().contains("not a Flip animation"));
        // The open drawing is untouched.
        QCOMPARE(qAlpha(pixel(backend, 0, 10, 10)), 255);

        Document document = Document::blank({64, 32});
        QString error;
        QVERIFY(document.save(dir.filePath("cut.flip"), &error));
        QFile cut(dir.filePath("cut.flip"));
        QVERIFY(cut.resize(cut.size() - 10));
        QVERIFY(!document.load(cut.fileName(), &error));
        QVERIFY(error.contains("damaged"));
    }

    void pickerNamesGetAnExtensionButNeverReplaceSilently()
    {
        QTemporaryDir dir;
        FakePicker picker;
        Backend backend(&picker);
        picker.answer = dir.filePath("hop");
        QVERIFY(!backend.save());
        QVERIFY(QFile::exists(dir.filePath("hop.flip")));

        Backend other(&picker);
        dot(other, {1, 1});
        other.saveAs();
        QVERIFY(other.error().contains("already exists"));
        QVERIFY(other.dirty());
    }

    void exportsGifAndMp4()
    {
        if (Exporter::ffmpeg().isEmpty())
            QSKIP("ffmpeg is not installed");
        QTemporaryDir dir;
        Document document = Document::blank({64, 36});
        document.frames.append(Document::blankFrame(document.size));
        document.frames.append(Document::blankFrame(document.size));
        for (const QString name : {"a.gif", "a.mp4"}) {
            Exporter exporter;
            QVERIFY2(exporter.run(document, dir.filePath(name)), qPrintable(exporter.error()));
            QVERIFY(QFileInfo(dir.filePath(name)).size() > 0);
        }
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden).size(), 2);

        Exporter exporter;
        QVERIFY(!exporter.start(document, dir.filePath("a.webm")));
        QVERIFY(exporter.error().contains(".gif"));
    }

    void failedExportKeepsTheOldFile()
    {
        if (Exporter::ffmpeg().isEmpty())
            QSKIP("ffmpeg is not installed");
        QTemporaryDir dir;
        write(dir.filePath("keep.mp4"), "precious");
        // A 1x1 picture scales to 0x0 for H.264, so ffmpeg fails.
        Document document = Document::blank({1, 1});
        Exporter exporter;
        QVERIFY(!exporter.run(document, dir.filePath("keep.mp4")));
        QFile kept(dir.filePath("keep.mp4"));
        QVERIFY(kept.open(QIODevice::ReadOnly));
        QCOMPARE(kept.readAll(), QByteArray("precious"));
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden), QStringList{"keep.mp4"});
    }

    void backendExportsThroughThePicker()
    {
        if (Exporter::ffmpeg().isEmpty())
            QSKIP("ffmpeg is not installed");
        QTemporaryDir dir;
        FakePicker picker;
        picker.answer = dir.filePath("loop");
        picker.extension = "mp4";
        Backend backend(&picker);
        backend.exportAnimation();
        QVERIFY(backend.exporting());
        QTRY_VERIFY(!backend.exporting());
        QVERIFY2(backend.error().isEmpty(), qPrintable(backend.error()));
        QVERIFY(QFile::exists(dir.filePath("loop.mp4")));
        QVERIFY(backend.status().contains("loop.mp4"));
    }

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
    }

    void themeReadsAccent()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        QFile colors(dir.path() + "/theme/colors.toml");
        QVERIFY(colors.open(QIODevice::WriteOnly));
        colors.write("accent = \"#112233\"\n");
        colors.close();

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
        QVERIFY(QColor(theme.accentInk()).lightnessF() >= 0.6);
        QCOMPARE(QColor(theme.accentInk()).hslHue(), QColor("#112233").hslHue());
    }

    void lightAccentsStayAsTheyAre()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accentInk(), QString("#ffd60a"));
    }

    void themeFollowsHyprlandRounding()
    {
        QTemporaryDir dir;
        const QString response = dir.filePath("response.json"), command = dir.filePath("hyprctl");
        auto write = [](const QString &path, const QByteArray &data) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        QVERIFY(write(command, "#!/bin/sh\ncat '" + response.toUtf8() + "'\n"));
        QVERIFY(QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(write(response, "{\"int\":0}"));

        Theme theme(dir.path(), nullptr, command);
        QCOMPARE(theme.radius(), 0);
        QVERIFY(write(response, "{\"int\":6}"));
        QTRY_COMPARE(theme.radius(), 6);
    }

    void translatorFollowsTheSystemWithoutOmarchy()
    {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), Translator::systemLanguage());
        QVERIFY(translator.translate("Main", "Save").isNull());
        QCOMPARE(Translator::readLanguage(dir.path() + "/language", "de"), QString("de"));
        write(dir.path() + "/garbage", "not a language\n");
        QCOMPARE(Translator::readLanguage(dir.path() + "/garbage", "da"), QString("da"));
        // Omarchy's pick wins over the system's.
        write(dir.path() + "/language", "da\n");
        QCOMPARE(Translator::readLanguage(dir.path() + "/language", "de"), QString("da"));
    }

    void systemLocalesMapToOmarchyLanguages()
    {
        QCOMPARE(Translator::languageFor(QLocale("de_DE")), QString("de"));
        QCOMPARE(Translator::languageFor(QLocale("de_AT")), QString("de"));
        QCOMPARE(Translator::languageFor(QLocale("pt_BR")), QString("pt-BR"));
        QCOMPARE(Translator::languageFor(QLocale("pt_PT")), QString("pt"));
        QCOMPARE(Translator::languageFor(QLocale("zh_TW")), QString("zh-TW"));
        QCOMPARE(Translator::languageFor(QLocale("ru_RU")), QString("ru"));
        QCOMPARE(Translator::languageFor(QLocale::c()), QString("en"));
    }

    void translatorFollowsTheLanguageFile()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "pt-BR\n");
        write(dir.path() + "/pt.json", R"({"Save": "Guardar", "Open": "Abrir"})");
        write(dir.path() + "/pt-BR.json", R"({"Save": "Salvar"})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QString("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QString("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void translatorPicksPluralForms()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "ru\n");
        write(dir.path() + "/en.json", R"({"%n clips": {"one": "%n clip", "other": "%n clips"}})");
        write(dir.path() + "/ru.json", R"({"%n clips": {"one": "%n клип", "few": "%n клипа", "many": "%n клипов", "other": "%n клипа"}})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 21), QString("%n клип"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 22), QString("%n клипа"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 25), QString("%n клипов"));
        QCOMPARE(Translator::pluralCategory("ar", 11), QString("many"));
        QCOMPARE(Translator::pluralCategory("ja", 1), QString("other"));
    }

private:
    static void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
};

QTEST_MAIN(FlipTests)
#include "flip_tests.moc"
