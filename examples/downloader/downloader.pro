QT       += core gui widgets

TARGET = downloader
TEMPLATE = app

include (../../src/qtcurl.pri)

SOURCES += main.cpp\
        MainWindow.cpp

HEADERS  += MainWindow.h

FORMS    += MainWindow.ui


# Assume libcurl is installed at place where compiler sees it by default.
# Just like after 'apt install libcurl4-openssl-dev' on ubuntu, for example

LIBS += -lcurl
