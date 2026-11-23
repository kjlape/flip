QT += core gui concurrent qml quick quickcontrols2 dbus

CONFIG += c++17 release
TARGET = flip
TEMPLATE = app
DEFINES += FLIP_VERSION=\\\"0.1.1\\\"

HEADERS += \
    src/backend.h \
    src/cli.h \
    src/document.h \
    src/drawingsurface.h \
    src/exporter.h \
    src/filepicker.h \
    src/frameimageprovider.h \
    src/frames.h \
    src/portalfilepicker.h \
    src/theme.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/cli.cpp \
    src/document.cpp \
    src/drawingsurface.cpp \
    src/exporter.cpp \
    src/frameimageprovider.cpp \
    src/frames.cpp \
    src/portalfilepicker.cpp \
    src/theme.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
