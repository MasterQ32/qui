BCC = $(CC) $(CFLAGS) -O3 -g -std=c99 -I include -I$(WORKDIR)/env/include -L$(WORKDIR)/env/lib -o
MV = mv
RM = rm
COPY = cp
CPTH = install -d

.PHONY: all clean install

all: qui_server demo fontdemo dora

qui_server: qui_server.c
	$(BCC) qui_server qui_server.c -lSDL_image -lSDL -lpng  -ljpeg -lz \
		-I$(WORKDIR)/env/include \
		-L$(WORKDIR)/env/lib

demo: demo.c qui.c qui.h
	$(BCC) $@ demo.c qui.c -lpng -lz

fontdemo: fontdemo.c qui.c qui.h tfont.c tfont.h
	$(BCC) $@ fontdemo.c qui.c tfont.c -lpng -lz

dora: dora.c qui.c qui.h
	$(BCC) $@ dora.c qui.c -lpng -lz

	
clean:
	-$(RM) qui demo

install:
	$(CPTH) $(INSTALL_TO)/bin
	$(CPTH) $(INSTALL_TO)/lib
	$(COPY) qui_server demo fontdemo dora $(INSTALL_TO)/bin
	$(COPY) ascii.tfn  $(INSTALL_TO)/lib

	$(COPY) qui_server demo fontdemo dora ascii.tfn /home/felix/projects/tyn/tyndur/build/root-local/
