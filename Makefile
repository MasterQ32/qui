MV = mv
RM = rm
COPY = cp
CPTH = install -d

.PHONY: all clean install subdirs

THINGS=bin/quisrv lib/libqui.a bin/qterm bin/dora bin/fontdemo bin/guidemo

all: subdirs $(THINGS)

bin/quisrv:
	$(MAKE) -C src/server/
	
lib/libqui.a:
	$(MAKE) -C src/lib/
	
bin/qterm:
	$(MAKE) -C src/addons/qterm/

bin/dora:
	$(MAKE) -C src/addons/dora/
	
bin/guidemo bin/fontdemo:
	$(MAKE) -C src/demos/
	

subdirs:
	mkdir -p bin
	mkdir -p lib

clean:
	-$(RM) $(THINGS)

install:
	$(CPTH) $(INSTALL_TO)/bin
	$(CPTH) $(INSTALL_TO)/lib
	$(CPTH) $(INSTALL_TO)/share
	$(COPY) bin/* $(INSTALL_TO)/bin/
	$(COPY) lib/* $(INSTALL_TO)/lib/
	$(COPY) data/* $(INSTALL_TO)/share/