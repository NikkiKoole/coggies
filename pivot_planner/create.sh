echo "the .ase file was : $1"

../../../aseprite-beta/aseprite/build/bin/aseprite -b --format json-array --list-layers  --split-layers $1 --trim --sheet all.png --data all.json
../../../aseprite-beta/aseprite/build/bin/aseprite -b --format json-array --list-layers --split-layers  --layer "pixels" $1 --trim --sheet pixel.png --data pixel.json
