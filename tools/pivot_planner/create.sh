
echo './create.sh "resources/facing_north.ase resources/facing_east.ase resources/facing_south.ase resources/facing_west.ase resources/walking_north.ase resources/walking_east.ase resources/walking_south.ase resources/walking_west.ase"'
echo 'usage example (multiple ase files (note the " "!)): ./create.sh "resources/nieuwding.ase resources/l4.ase resources/schets2.ase"'
echo "the .ase file was : $1"


../../../../aseprite-beta/aseprite/build/bin/aseprite -b --format json-array --list-layers  --split-layers $1 --trim --sheet all.png  --sheet-pack --inner-padding 1 --data gen/all.json
../../../../aseprite-beta/aseprite/build/bin/aseprite -b --format json-array --list-layers --split-layers  --layer "pixels" $1 --trim --sheet gen/pixel.png --sheet-pack --inner-padding 1 --data gen/pixel.json
