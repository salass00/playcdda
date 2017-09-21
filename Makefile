TARGET  := PlayCDDA
VERSION := 2

HOST  := ppc-amigaos
CC    := $(HOST)-gcc
STRIP := $(HOST)-strip

CFLAGS     := -O2 -g -Wall -Wwrite-strings -Werror
LDFLAGS    := -static
LIBS       := 
STRIPFLAGS := --strip-unneeded -R.comment

SRCS := main.c locale.c iorequest.c ahi.c cdrom.c gui_reaction.c gui_mui.c
OBJS := $(SRCS:.c=.o)

.PHONY: all
all: $(TARGET)

locale.h: $(TARGET).cd
	catcomp $< --cfile $@

init.o: $(TARGET)_rev.h

gui_reaction.o gui_mui.o: $(TARGET)_rev.h locale.h

$(OBJS): playcdda.h

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug

.PHONY: clean
clean:
	rm -f $(TARGET) $(TARGET).debug *.o

.PHONY: revision
revision:
	bumprev $(VERSION) $(TARGET)

