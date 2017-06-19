BCC = $(CC) $(CFLAGS) -O3 \
		-DLBUILD_PACKAGE="\"${PN}\"" \
		-DLBUILD_VERSION="\"${PV}\"" \
		-g -std=c99 \
		-I ../../include \
		-L ../../lib \
		-I$(WORKDIR)/env/include \
		-L$(WORKDIR)/env/lib
MV = mv
RM = rm
COPY = cp
CPTH = install -d

.PHONY: all clean install

all: ../../bin/fontdemo ../../bin/guidemo

../../bin/fontdemo: fontdemo.c tfont.c tfont.h
	$(BCC) -o $@ fontdemo.c tfont.c -lqui -lpng -ljpeg -lz
	
../../bin/guidemo: demo.c
	$(BCC) -o $@ demo.c -lqui -lpng -ljpeg -lz


clean:
	-$(RM) $(OBJECTS)