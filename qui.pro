
TEMPLATE = app

DISTFILES += \
    build.sh \
    qui-alpha.lbuild \
    Makefile \
    ascii.tfn

HEADERS += \
    quidata.h \
    qui.h \
    tfont.h

SOURCES += \
    qui_server.c \
    qui.c \
    demo.c \
    tfont.c \
    fontdemo.c \
    dora.c

INCLUDEPATH += "/home/felix/projects/tyn/lbuilds/env/include/"
