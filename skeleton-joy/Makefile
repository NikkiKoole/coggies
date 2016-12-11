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

SUPERSTRICT := $(STRICT_RPI2) -Wundef -Wbad-function-cast -Wstrict-prototypes -Wredundant-decls  -Wcast-qual -Wshadow #-pedantic-errors

WARNINGS = $(STRICT_RPI2)

DEBUG:= -g3 -fsanitize=address -fno-omit-frame-pointer

OPTIMIZE:= #-O3

STD:= -std=gnu99


all:
	${CC}  -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS)  $(OPTIMIZE) $(DEBUG) ${STD}  -lSDL2_image  main.c  -o skelet-joy
