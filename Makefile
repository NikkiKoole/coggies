SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LFLAGS := $(shell sdl2-config --libs)


# temroary lax the compiler for timsort experiment
# -Wall
#RELAXED = -Werror

STRICT = -Werror
STRICT_RPI2 := $(STRICT) -Wextra  -Wformat=2 -Wno-import \
		   -Wimplicit -Wmain -Wchar-subscripts -Wsequence-point -Wmissing-braces \
		   -Wparentheses -Winit-self -Wswitch-enum -Wstrict-aliasing=2  \
		   -Wpointer-arith -Wcast-align \
		   -Wwrite-strings  -Wold-style-definition -Wmissing-prototypes \
		   -Wmissing-declarations  -Wnested-externs -Winline \
		   -Wdisabled-optimization -Wno-unused
SUPERSTRICT := $(STRICT_RPI2) -Wundef -Wbad-function-cast -Wstrict-prototypes -Wredundant-decls -pedantic-errors -Wcast-qual -Wshadow
WARNINGS = $(-Werror)


OBJDIR = ./objs/
LIBRARY_NAME := gamelibrary.so

PROGRAM_NAME := coggies.out
CHK_SOURCES := src/main.c

#-fsanitize=address use this to find overflow bugs etc, sgenv
DEBUG:=-g3

OPTIMIZE:=-O3
STD:=-std=gnu99

CC:=gcc

BACKEND_FILES:=src/main.c src/resource.c src/random.c src/memory.c src/renderer.c src/game.c

osx:
	${CC} -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) $(OPTIMIZE) -DOSX ${STD} -lSDL2_mixer ${DEBUG} -lglew -framework OpenGL ${BACKEND_FILES} -o $(PROGRAM_NAME)

linux:
	${CC} -I/usr/local/include/  $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) -DLINUX ${STD} -lSDL2_mixer ${DEBUG} -lGL  -lGLEW  ${BACKEND_FILES} -lm -o $(PROGRAM_NAME)

pi:
	${CC} -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) $(OPTIMIZE) -mfp16-format=alternative -mfpu=neon-fp16 -mfloat-abi=hard -DRPI ${STD} -lSDL2_mixer -L/opt/vc/lib -lEGL -lGLESv2 ${BACKEND_FILES} -lm -o $(PROGRAM_NAME)


# dont forget, renderer.o is needed here
gamelibrary:
	mkdir -p $(OBJDIR)
	${CC} -c -I/usr/local/include  $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) ${STD} ${DEBUGFLAG} $(OPTIMIZE) -DOSX -fPIC src/game.c src/random.c
	${CC} ${DEBUGFLAG} $(OPTIMIZE)  -DOSX   -shared -o $(LIBRARY_NAME) game.o random.o
	mv *.o $(OBJDIR)
