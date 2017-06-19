BCC = $(CC) $(CFLAGS) -O3 -DLBUILD_PACKAGE="\"${PN}\"" -DLBUILD_VERSION="\"${PV}\"" -g -std=c99 -I ../../include -I$(WORKDIR)/env/include -L$(WORKDIR)/env/lib
MV = mv
RM = rm
COPY = cp
CPTH = install -d
AR = ar

OBJECTS=windows.o bitmaps.o

.PHONY: all clean install

all: ../../lib/libqui.a

../../lib/libqui.a: $(OBJECTS)
	$(AR) rcs $@ $^

%.o: %.c ../../include/qui.h ../../include/quidata.h
	$(BCC) -c -o $@ $<

clean:
	-$(RM) $(OBJECTS)