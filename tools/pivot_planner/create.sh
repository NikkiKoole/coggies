
echo 'usage example (multiple ase files (note the " "!)): ./create.sh "resources/nieuwding.ase resources/l4.ase resources/schets2.ase"'
echo "the .ase file was : $1"


../../../aseprite-beta/aseprite/build/bin/aseprite -b --format json-array --list-layers  --split-layers $1 --trim --sheet all.png --data gen/all.json
../../../aseprite-beta/aseprite/build/bin/aseprite -b --format json-array --list-layers --split-layers  --layer "pixels" $1 --trim --sheet gen/pixel.png --data gen/pixel.json
