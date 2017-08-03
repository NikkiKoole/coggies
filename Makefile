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
SUPERSTRICT := $(STRICT_RPI2) -Wundef -Wbad-function-cast -Wstrict-prototypes -Wredundant-decls  -Wcast-qual -Wshadow
WARNINGS = $(STRICT_RPI2)


OBJDIR = ./objs/
LIBRARY_NAME := gamelibrary.so

PROGRAM_NAME := coggies.out
CHK_SOURCES := src/main.c

ASEPRITE := ../aseprite-beta/aseprite/build/bin/aseprite

DEBUG:= #-g3 -fsanitize=address -fno-omit-frame-pointer

OPTIMIZE:= #-O3
STD:= -std=gnu99

CC:= gcc #g++ -x c

BACKEND_FILES:=src/main.c src/resource.c src/random.c src/memory.c src/renderer.c src/game.c src/pathfind.c src/level.c

linux:
	${CC} -I/usr/local/include/  $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) -DLINUX ${STD} -lSDL2_mixer   ${DEBUG} -lGL  -lGLEW  ${BACKEND_FILES} -lm -o $(PROGRAM_NAME)

osx:
	${CC} -I/usr/local/include/ $(SDL_CFLAGS) $(SDL_LFLAGS) $(WARNINGS) $(OPTIMIZE) -DOSX ${STD} -lSDL2_mixer ${DEBUG} -lglew -framework OpenGL ${BACKEND_FILES} -o $(PROGRAM_NAME)


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


blocks:
	@echo "Lets use the block_ase_files to create block data"
	@${ASEPRITE}  -b --format json-array --list-layers  --split-layers resources/block_ase_files/* --trim --sheet gen/all_generated_blocks.png  --sheet-pack --inner-padding 1 --data gen/all_generated_blocks.json
	@${ASEPRITE} -b --format json-array --list-layers --split-layers  --layer "pixels" resources/block_ase_files/* --trim --sheet gen/pixel_blocks.png --sheet-pack --inner-padding 1 --data gen/pixel_blocks.json
	@tools/pivot_planner/pivot-planner "blocks" gen/pixel_blocks.json gen/all_generated_blocks.json
	@mv blocks.txt src/blocks.h
	@mv gen/pixel_blocks.png resources/textures/blocks.png

bodies:
	@echo "Lets use the body_ase_files to create block data"
	@${ASEPRITE}  -b --format json-array --list-layers  --split-layers resources/body_ase_files/* --trim --sheet gen/all_generated_bodies.png  --sheet-pack --inner-padding 1 --data gen/all_generated_bodies.json
	@${ASEPRITE} -b --format json-array --list-layers --split-layers  --layer "pixels" resources/body_ase_files/* --trim --sheet gen/pixel_bodies.png --sheet-pack --inner-padding 1 --data gen/pixel_bodies.json
	@tools/pivot_planner/pivot-planner "bodies" gen/pixel_bodies.json gen/all_generated_bodies.json
	@mv bodies.txt src/body.h
	@mv gen/pixel_bodies.png resources/textures/body.png


texture-atlas-blocks:
	@echo "This will convert some .js output from shoebox into a blocks.h file"
	@tools/pivot_planner/pivot-planner resources/sprites.js
	@mv output_program.txt src/blocks.h

test:
	gcc ${STD} ${DEBUG} spec/spec_runner.c  src/memory.c src/pathfind.c src/level.c  src/random.c  -lm  && ./a.out
	rm ./a.out

tags:
	find src/ -type f -iname "*.[chS]" | xargs etags -a
