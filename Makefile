BCC = $(CC) $(CFLAGS) -O3 -DLBUILD_PACKAGE="\"${PN}\"" -DLBUILD_VERSION="\"${PV}\"" -g -std=c99 -I include -I$(WORKDIR)/env/include -L$(WORKDIR)/env/lib -o
MV = mv
RM = rm
COPY = cp
CPTH = install -d

PROGRAMS = qui_server demo fontdemo dora qterm pingpong

.PHONY: all clean install

all: $(PROGRAMS)

qui_server: src/qui_server.c include/qui.h include/quidata.h
	$(BCC) qui_server src/qui_server.c -lSDL_image -lSDL -lpng  -ljpeg -lz \
		-I$(WORKDIR)/env/include \
		-L$(WORKDIR)/env/lib

demo: src/demo.c src/qui.c include/qui.h include/quidata.h
	$(BCC) $@ src/demo.c src/qui.c -lpng -lz

fontdemo: src/fontdemo.c src/qui.c include/qui.h src/tfont.c include/tfont.h include/quidata.h
	$(BCC) $@ src/fontdemo.c src/qui.c src/tfont.c -lpng -lz

dora: src/dora.c src/qui.c include/qui.h include/quidata.h
	$(BCC) $@ src/dora.c src/qui.c -lpng -lz

qterm: src/qterm.c src/qui.c include/qui.h include/quidata.h
	$(BCC) $@ src/qterm.c src/qui.c -lpng -lz

pingpong: src/pingpong.c
	$(BCC) $@ src/pingpong.c

clean:
	-$(RM) qui demo

install:
	$(CPTH) $(INSTALL_TO)/bin
	$(CPTH) $(INSTALL_TO)/share
	$(COPY) $(PROGRAMS) $(INSTALL_TO)/bin/
	$(COPY) data/* $(INSTALL_TO)/share/

	# Cheat: Copy the server to root as well, so we can just call it
	$(COPY) qui_server /home/felix/projects/tyn/tyndur/build/root-local/
#	$(COPY) data/* /home/felix/projects/tyn/tyndur/build/root-local/guiapps/
