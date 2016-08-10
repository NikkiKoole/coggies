SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LFLAGS := $(shell sdl2-config --libs)


STRICT = -Werror -Wall
STRICT_RPI2 := $(STRICT) -Wextra  -Wformat=2 -Wno-import \
		   -Wimplicit -Wmain -Wchar-subscripts -Wsequence-point -Wmissing-braces \
		   -Wparentheses -Winit-self -Wswitch-enum -Wstrict-aliasing=2  \
		   -Wpointer-arith -Wcast-align \
		   -Wwrite-strings  -Wold-style-definition -Wmissing-prototypes \
		   -Wmissing-declarations  -Wnested-externs -Winline \
		   -Wdisabled-optimization -Wno-unused
SUPERSTRICT := $(STRICT_RPI2) -Wundef -Wbad-function-cast -Wstrict-prototypes -Wredundant-decls -pedantic-errors -Wcast-qual -Wshadow
WARNINGS = $(SUPERSTRICT)


OBJDIR = ./objs/
LIBRARY_NAME := gamelibrary.so

PROGRAM_NAME := coggies.app
CHK_SOURCES := src/main.c

DEBUGFLAG:=-g3
OPTIMIZE:=-O3
STD:=-std=gnu99

CC:=clang

BACKEND_FILES:=src/main.c src/resource.c src/random.c src/memory.c src/renderer.c

osx:
	${CC} -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) -DOSX ${STD} -lSDL2_mixer ${DEBUG} -lglew -framework OpenGL ${BACKEND_FILES} -o $(PROGRAM_NAME)

linux:
	${CC} -I/usr/local/include/  $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) -DLINUX ${STD} -lSDL2_mixer ${DEBUG} -lGL  -lGLEW  ${BACKEND_FILES} -lm -o $(PROGRAM_NAME)

pi:
	${CC} -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) -DRPI ${STD} -lSDL2_mixer -L/opt/vc/lib -lGLESv2 ${BACKEND_FILES} -lm -o $(PROGRAM_NAME)

gamelibrary:
	mkdir -p $(OBJDIR)
	${CC} -c -I/usr/local/include $(SDL_CFLAGS) $(WARNINGS) ${STD} ${DEBUGFLAG} -fPIC src/game.c
	${CC} -shared -o $(LIBRARY_NAME) game.o $(SDL_LFLAGS)
	mv *.o $(OBJDIR)
