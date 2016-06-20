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


all:
	gcc -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) -DOSX -lSDL2_mixer -g -lglew -framework OpenGL src/main.c src/random.c src/memory.c src/renderer.c

linux:
	gcc -g -I/usr/local/include/  -lGL $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) -DLINUX  -std=gnu99 -lSDL2_mixer -lGLEW  src/main.c src/random.c src/memory.c src/renderer.c

pi:
	gcc -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) -std=gnu99 -DRPI -lSDL2_mixer -L/opt/vc/lib -lGLESv2 src/main.c src/random.c src/memory.c src/renderer.c
