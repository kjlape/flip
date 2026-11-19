// Flip — draw frame by frame, then play it back as an animation.

#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>
#include <cstdio>

#include "backend.h"
#include "cli.h"
#include "drawingsurface.h"
#include "frameimageprovider.h"
#include "portalfilepicker.h"
#include "theme.h"
#include "translator.h"

int main(int argc, char *argv[]) {
    // Bare flip prints help, as a command line tool should; the launcher says
    // flip open. Commands draw no window, so they must not need a display.
    const bool command = argc == 1 || isCliCommand(argv[1]);
    if (command)
        qputenv("QT_QPA_PLATFORM", "offscreen");

    QGuiApplication app(argc, argv);
    app.setApplicationName("flip");
    app.setApplicationVersion(FLIP_VERSION);
    if (command)
        return runCli(app.arguments());

    // flip open [file], or a file handed straight to flip.
    const QStringList arguments = app.arguments();
    const QString file = arguments[1] == "open" ? arguments.value(2) : arguments[1];
    if (arguments[1] != "open" && !QFileInfo::exists(file)) {
        fprintf(stderr, "Unknown command %s\n\nUsage: flip <command> [options]\n\n%s\n", argv[1],
                qPrintable(cliSummary()));
        return 1;
    }

    // Matches the window to flip.desktop, so the compositor (Wayland app_id)
    // and the launcher pick up the installed icon. The Flatpak installs them
    // under its app ID instead and says so in FLIP_DESKTOP_FILE.
    const QString desktopFile = qEnvironmentVariable("FLIP_DESKTOP_FILE", "flip");
    app.setDesktopFileName(desktopFile);
    app.setWindowIcon(QIcon::fromTheme(desktopFile));

    QQuickStyle::setStyle("Material");

    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    PortalFilePicker picker;
    Backend backend(&picker);
    if (!file.isEmpty())
        backend.openPath(file);

    qmlRegisterType<DrawingSurface>("Flip", 1, 0, "DrawingSurface");

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.addImageProvider("frames", new FrameImageProvider(&backend));
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
