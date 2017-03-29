## Ok I will need a skeleton system again.

    ```
                +---+
                |   |
                |   |
                |   |
                +-+-+
                  |

           0  +---0---+  0
           |  |       |  |
           |  |       |  |
           |  |       |  |
           |   \     /   |
           |   |     |   |
          0    |     |    0
          |    +-----+    |
          |               |
          |    0     0    |
          0    |     |    0
          |    |     |    |
               |     |
               |     |
               |     |
               |     |
               0     0
               |     |
               |     |
               |     |
               |     |
               |     |
            ---0     0---
    ```

I was hoping it didnt come to this, but helas ;)
My initial thought was to just have an upper-body, lower-body and head and call it a day,
But looking at a character from the side will give a problem thats quite unsolvable:
1 arm is behind the body, 1 arm in front
the arm that is behind the body is also behind the legs,
while at the same time the other arm should be drawn over the legs...

Anyway, my plan now is to have
- head
- torso
- left arm
- right arm
- left leg
- right leg
In the runtime and stitch em up there.

As a separate 'animation' tool I'd like to use all separate bodyparts
(upper-arm, lower-arm, hand) instead of arm
and (upper-leg, lower-leg, foot) instead of leg

Then this separate tool would be used to generate the spritesheets (containing arms/legs/torsos)
This would make generating animations a lot easier because i don't have to draw these parts manually.

What I would like is feeding the system separate images for the smallest parts,
somehow I need to tell what the origin and end of a part is
(first idea, half transparant pixels, second idea separate data file)

The system also needs to be told what combined animations to make: for example:
given an upperleg part, lowerleg part and footpart
it will generate these combined images,

When you would have 5 upper leg images and 5 lower leg images and 5 feet, this would be generating:
2 images * (125 combinations). 250 images
this adds up quite hard so maybe its better to not allow some combinations, for example an upperleg for a baby doesnt need combining with a lowerleg for a huge bodybuilder..

```
    \                    -------O
     \                          |
      \                         |
       \                        |
        O                       O----
        |
        |
        |
        O-----

```

Anyway, the plan now is as this:

make a tool using vanilla SDL2 to see
- if you can rotate parts and have it look allright
- stitch the 3 parts together to form a leg or arm
- export aptly named PNG files, just separate files for now, making an atlas is outside the scope of this tool.
- export data for parts too, you want to know what their origin is and their endpoint (origin = shoulder endpoint = end of hand)

It might make sense to let this tool be my animation 'definer' too.

I think i want to be able to at runtime use simple rotation (90degs discrete) and scale(mirror) on the sprites.
this way i can reuse a lot of sprites in multiple ways.
