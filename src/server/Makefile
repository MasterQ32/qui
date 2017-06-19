BCC = $(CC) $(CFLAGS) -O3 -DLBUILD_PACKAGE="\"${PN}\"" -DLBUILD_VERSION="\"${PV}\"" -g -std=c99 -I ../../include -I$(WORKDIR)/env/include -L$(WORKDIR)/env/lib
MV = mv
RM = rm
COPY = cp
CPTH = install -d

OBJECTS=main.o

.PHONY: all clean install

all: ../../bin/quisrv

../../bin/quisrv: $(OBJECTS)
	$(BCC) -o $@ $^ -lSDL_image -lSDL -lpng  -ljpeg -lz
		
%.o: %.c ../../include/qui.h ../../include/quidata.h
	$(BCC) -c -o $@ $<

clean:
	-$(RM) $(OBJECTS)