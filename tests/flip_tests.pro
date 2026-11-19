QT += core gui quick quickcontrols2 testlib dbus
CONFIG += c++17 testcase
TARGET = flip_tests
TEMPLATE = app
DEFINES += FLIP_VERSION=\\\"test\\\"

INCLUDEPATH += ../src

HEADERS += ../src/backend.h ../src/document.h ../src/exporter.h ../src/filepicker.h ../src/frames.h \
           ../src/theme.h ../src/translator.h
SOURCES += flip_tests.cpp ../src/backend.cpp ../src/document.cpp ../src/exporter.cpp ../src/frames.cpp \
           ../src/theme.cpp ../src/translator.cpp
