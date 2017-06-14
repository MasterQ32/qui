BCC = $(CC) $(CFLAGS) -O3 -o
MV = mv
RM = rm
COPY = cp
CPTH = install -d

.PHONY: all clean install

all: qui

qui: qui.c
	$(BCC) qui -std=c99 -I include qui.c

clean:
	-$(RM) qui

install:
	$(CPTH) $(INSTALL_TO)/bin
	$(COPY) qui $(INSTALL_TO)/bin

	$(COPY) qui /home/felix/projects/tyn/tyndur/build/root-local/