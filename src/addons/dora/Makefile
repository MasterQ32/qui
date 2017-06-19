BCC = $(CC) $(CFLAGS) -O3 \
		-DLBUILD_PACKAGE="\"${PN}\"" \
		-DLBUILD_VERSION="\"${PV}\"" \
		-g -std=c99 \
		-I ../../../include \
		-L ../../../lib \
		-I$(WORKDIR)/env/include \
		-L$(WORKDIR)/env/lib
MV = mv
RM = rm
COPY = cp
CPTH = install -d

OBJECTS=main.o

.PHONY: all clean install

all: ../../../bin/dora

../../../bin/dora: $(OBJECTS)
	$(BCC) -o $@ $^ -lqui -lpng -ljpeg -lz
		
%.o: %.c ../../../include/qui.h
	$(BCC) -c -o $@ $<

clean:
	-$(RM) $(OBJECTS)