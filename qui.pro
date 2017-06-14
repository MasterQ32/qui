
TEMPLATE = app

DISTFILES += \
    build.sh \
    qui-alpha.lbuild \
    Makefile

HEADERS += \
    quidata.h \
    qui.h

SOURCES += \
    qui_server.c \
    qui.c \
    demo.c

INCLUDEPATH += "/home/felix/projects/tyn/lbuilds/env/include/"
