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


OBJDIR = ./objs/
LIBRARY_NAME := gamelibrary.so

PROGRAM_NAME := coggies.out
CHK_SOURCES := src/main.c


DEBUG:= -g3 -fsanitize=address -fno-omit-frame-pointer

OPTIMIZE:= #-O3
STD:= -std=gnu99

CC:= gcc #g++ -x c

BACKEND_FILES:=src/main.c src/resource.c src/random.c src/memory.c src/renderer.c src/game.c src/pathfind.c src/level.c

osx:
	${CC} -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) $(OPTIMIZE) -DOSX ${STD} -lSDL2_mixer ${DEBUG} -lglew -framework OpenGL ${BACKEND_FILES} -o $(PROGRAM_NAME)

linux:
	${CC} -I/usr/local/include/  $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) -DLINUX ${STD} -lSDL2_mixer   ${DEBUG} -lGL  -lGLEW  ${BACKEND_FILES} -lm -o $(PROGRAM_NAME)

pi:
	${CC} -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) $(OPTIMIZE) -mfp16-format=alternative -mfpu=neon-fp16 -mfloat-abi=hard -DRPI ${STD} -lSDL2_mixer -L/opt/vc/lib -lEGL -lGLESv2 ${BACKEND_FILES} -lm -o $(PROGRAM_NAME)


steer:
	${CC}  -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS)  $(OPTIMIZE) $(DEBUG) ${STD}  -lSDL2_mixer -lSDL2_image  -lSDL2_ttf steer_test/main.c steer_test/Remotery.c

gamelibrary:
	mkdir -p $(OBJDIR)
	${CC} -c -I/usr/local/include  $(SDL_CFLAGS) $(WARNINGS) ${STD} ${DEBUGFLAG} $(OPTIMIZE)   -DOSX -fPIC src/level.c src/game.c src/random.c src/renderer.c src/memory.c src/pathfind.c
	${CC} ${DEBUGFLAG} $(OPTIMIZE)  -DOSX   -shared -o $(LIBRARY_NAME) level.o game.o random.o renderer.o memory.o pathfind.o -lglew -framework OpenGL  $(SDL_LFLAGS)
	mv *.o $(OBJDIR)

gamelibrary-linux:
	mkdir -p $(OBJDIR)
	${CC} -c -I/usr/local/include  $(SDL_CFLAGS) $(WARNINGS) ${STD} ${DEBUGFLAG} $(OPTIMIZE)   -DLINUX -fPIC src/level.c src/game.c src/random.c src/renderer.c src/memory.c src/pathfind.c
	${CC} ${DEBUGFLAG} $(OPTIMIZE)  -DLINUX   -shared -o $(LIBRARY_NAME) level.o game.o random.o renderer.o memory.o pathfind.o -lGL  -lGLEW   $(SDL_LFLAGS)
	mv *.o $(OBJDIR)



test:
	gcc ${STD} ${DEBUG} spec/spec_runner.c  src/memory.c src/pathfind.c src/level.c  src/random.c  -lm  && ./a.out
	rm ./a.out

tags:
	find src/ -type f -iname "*.[chS]" | xargs etags -a
