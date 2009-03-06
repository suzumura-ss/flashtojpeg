EXE=flashtojpeg
OBJS=flash2jpeg.o jpegenc.o flash2bitmap.o logger.o
PREFIX=/usr/local

#DEBUG_CFLAGS= -g
#GPROF_FLAG = -pg
# sdl
GFX_LIBS  =`sdl-config --libs`
GFX_CFLAGS=`sdl-config --cflags`

# cairo graphics
CAIRO_LIBS  =`pkg-config cairo --libs`
CAIRO_CFLAGS=`pkg-config cairo --cflags`

CFLAGS= -O2 $(GPROF_FLAG) $(GFX_CFLAGS) $(DEBUG_CFLAGS) $(CAIRO_CFLAGS) -I ../src
LDFLAGS= $(GPROF_FLAG) $(GFX_LIBS) $(CAIRO_LIBS) -L ../src -lfad -lstdc++ -ljpeg

all:$(EXE)

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $(EXE) $(OBJS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(EXE) *.o *.bak *.out -f

install: $(SHAREDLIBV)
	-@if [ ! -d $(PREFIX)/bin ]; then mkdir -p $(PREFIX)/bin; fi
	install ${EXE) $(PREFIX)/bin

uninstall:
	rm $(PREFIX)/bin/$(EXE)
