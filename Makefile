BCC = $(CC) $(CFLAGS) -O3 -o
MV = mv
RM = rm
COPY = cp
CPTH = install -d

.PHONY: all clean install

all: qui demo

qui: qui.c
	$(BCC) qui -std=c99 -I include qui.c -lSDL_image -lSDL  -lpng  -ljpeg -lz \
		-I$(WORKDIR)/env/include \
		-L$(WORKDIR)/env/lib

demo: demo.c
	$(BCC) demo -std=c99 -I include demo.c

clean:
	-$(RM) qui demo

install:
	$(CPTH) $(INSTALL_TO)/bin
	$(COPY) qui demo $(INSTALL_TO)/bin

	$(COPY) qui demo /home/felix/projects/tyn/tyndur/build/root-local/