SOURCES := powder.c
HEADERS := font.h

CFLAGS := -std=c99 -D_POSIX_C_SOURCE=200112L -D_GNU_SOURCE -DSCALE=1
OFLAGS := -O3 -ffast-math -ftree-vectorize -funsafe-math-optimizations
LIBS := -lSDL -lm
LIBS_WIN := -lmingw32 -lSDLmain -lSDL -lm
#For static linking:
#LIBS_WIN := -lmingw32 -lSDLmain -Wl,-Bstatic -lSDL -Wl,-Bdynamic -lm -lwinmm -ldxguid
MFLAGS_NORMAL := -march=native -msse3
MFLAGS_LEGACY := 

CC := gcc
#For cross compiling - if using MinGW on Windows, change this to gcc
CC_WIN := i686-w64-mingw32-gcc

powder: $(SOURCES) $(HEADERS)
	$(CC) -m32 $(CFLAGS) $(OFLAGS) $(MFLAGS_NORMAL) $(SOURCES) $(LIBS) -o $@

powder-legacy: $(SOURCES) $(HEADERS)
	$(CC) -m32 $(CFLAGS) $(OFLAGS) $(MFLAGS_LEGACY) $(SOURCES) $(LIBS) -o $@

powder-64: $(SOURCES) $(HEADERS)
	$(CC) -m64 $(CFLAGS) $(OFLAGS) $(MFLAGS_LEGACY) $(SOURCES) $(LIBS) -o $@

powder.exe: $(SOURCES) $(HEADERS)
	$(CC_WIN) -mwindows $(CFLAGS) $(OFLAGS) $(MFLAGS_NORMAL) $(SOURCES) $(LIBS_WIN) -o $@

powder-legacy.exe: $(SOURCES) $(HEADERS)
	$(CC_WIN) -mwindows $(CFLAGS) $(OFLAGS) $(MFLAGS_NORMAL) $(SOURCES) $(LIBS_WIN) -o $@


all: powder powder-legacy powder.exe powder-legacy.exe

