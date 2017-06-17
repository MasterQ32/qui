
TEMPLATE = app

DISTFILES += \
    build.sh \
    qui-alpha.lbuild \
    Makefile \
    ascii.tfn

HEADERS += \
    include/quidata.h \
    include/qui.h \
    include/tfont.h

SOURCES += \
    src/qui_server.c \
    src/qui.c \
    src/demo.c \
    src/tfont.c \
    src/fontdemo.c \
    src/dora.c \
    src/qterm.c

INCLUDEPATH += "/home/felix/projects/tyn/lbuilds/env/include/"
INCLUDEPATH += "$$PWD/include/"
