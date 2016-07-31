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

osx:
	gcc -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) -DOSX  -std=gnu99 -lSDL2_mixer -g3 -lglew -framework OpenGL src/main.c src/resource.c src/random.c src/memory.c src/renderer.c -o $(PROGRAM_NAME)

linux:
	gcc -g -I/usr/local/include/  -lGL $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) -DLINUX  -std=gnu99 -lSDL2_mixer -lGLEW  src/main.c src/resource.c src/random.c src/memory.c src/renderer.c -lm -o $(PROGRAM_NAME)

pi:
	gcc -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) -std=gnu99 -DRPI -lSDL2_mixer -L/opt/vc/lib -lGLESv2 src/main.c src/resource.c src/random.c src/memory.c src/renderer.c -lm -o $(PROGRAM_NAME)

gamelibrary:
	mkdir -p $(OBJDIR)
	gcc -c -I/usr/local/include $(SDL_CFLAGS) $(WARNINGS) -std=gnu99 -g3 -fPIC src/game.c
	gcc -shared -o $(LIBRARY_NAME) game.o $(SDL_LFLAGS)
	mv *.o $(OBJDIR)
