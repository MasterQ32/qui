
TEMPLATE = app

DISTFILES += \
    build.sh \
    qui-alpha.lbuild \
    Makefile

INCLUDEPATH += "/home/felix/projects/tyn/lbuilds/env/include/"
INCLUDEPATH += "$$PWD/include/"

HEADERS += \
    include/qui.h \
    include/quidata.h \
    src/demos/tfont.h

SOURCES += \
    src/addons/dora/main.c \
    src/addons/qterm/main.c \
    src/demos/demo.c \
    src/demos/fontdemo.c \
    src/demos/tfont.c \
    src/lib/bitmaps.c \
    src/lib/windows.c \
    src/server/main.c
