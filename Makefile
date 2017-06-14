BCC = $(CC) $(CFLAGS) -O3 -g -o
MV = mv
RM = rm
COPY = cp
CPTH = install -d

.PHONY: all clean install

all: qui_server demo

qui_server: qui_server.c
	$(BCC) qui_server -std=c99 -I include qui_server.c -lSDL_image -lSDL  -lpng  -ljpeg -lz \
		-I$(WORKDIR)/env/include \
		-L$(WORKDIR)/env/lib

demo: demo.c qui.c qui.h
	$(BCC) demo -std=c99 -I include demo.c qui.c

clean:
	-$(RM) qui demo

install:
	$(CPTH) $(INSTALL_TO)/bin
	$(COPY) qui_server demo $(INSTALL_TO)/bin

	$(COPY) qui_server demo /home/felix/projects/tyn/tyndur/build/root-local/