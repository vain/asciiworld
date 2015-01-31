Having a little bit of fun.

![asciiworld](/asciiworld.png?raw=true)

Inspired by:

*  The world map in [blessed-contrib](https://github.com/yaronn/blessed-contrib)
*  Fancy stuff in [trehn's termtrack](https://github.com/trehn/termtrack)

Dependencies
============

You need a map. I suggest you use one of those:

*  [Natural Earth Data, Land](http://www.naturalearthdata.com/downloads/110m-physical-vectors/110m-land/)
*  [Natural Earth Data, Countries](http://www.naturalearthdata.com/downloads/110m-cultural-vectors/110m-admin-0-countries/)

Since NED distributes their data as "ESRI Shapefiles", you also need [shapelib](http://shapelib.maptools.org/).

For rendering filled polygons, [libgd](http://www.libgd.org/) is used.

Long story short: If you're using Arch Linux, you're fine with installing these two packages:

*  [gd](https://www.archlinux.org/packages/extra/x86_64/gd/)
*  [shapelib](https://www.archlinux.org/packages/community/x86_64/shapelib/)

Command line options
====================

`-w W`, `-h H`: Force output size. By default, the full size of the terminal is used.

`-m path`: Path to an ESRI Shapefile (.shx or .shp).

`-l path`: Path to a file containing locations to highlight. For example, the following file would highlight Mainz and Cape Town:

    50 8
    -33.9 18.4

`-t path`: Path to a file containing one or more tracks to highlight. A track consists of series of spherical coordinates, terminated by a dot. asciiworld will project each point and then render a line, it will NOT interpolate between two points. An example track:

    50 8
    40 20
    30 40
    20 70
    10 90
    1 104
    .
    -33.9 18.4
    -40 30
    -50 50
    -60 80
    -50 110
    -40 130
    -33.9 151.1
    .

`-s`: Turn on "day-and-night" mode. Areas that are currently lit by the sun will be shown as green, whereas the other parts are shown as dark dark blue. Areas currently in the "twilight zone" will be shaded accordingly (see `-d`). The "position" of the sun itself will be marked with an "S". A yellow line marks the current sunset/sunrise locations.

`-d dusk`: Choose definition of "dusk" and thus the twilight span. Accepts "nau" for nautical dusk (12 degree twilight) or "ast" for astronomical dusk (18 degree twilight). By default, civil dusk (6 degree twilight) is used.

`-T`: Do not print the final newline to avoid scrolling. Using `-sT`, you can show a continuously updated map in "day-and-night" mode:

    tput civis; while ./asciiworld -sT; do sleep 5; tput home; done

`-p proj`: Choose projection. By default, a trivial equirectanguar projection will be used. The following other projections are available:

*  `kav`: Kavrayskiy VII
*  `lam`: Lambert cylindrical equal-area

`-b`: Draw a border around the projected world. This can be useful for some projections.

`-c n`: Set to n colors. Accepts 0 or 8. Default is 256.

`-o`: Draw land masses as filled polygons. You'll get garbled results when using this together with country borders.

`-W path`: Instead of creating ASCII output, you can save the resulting image as an "uninterpreted" PNG file. You will want to use `-w` and `-h` together with this option.

More ideas
==========

*  Zoom/pan
*  Render arbitrary lines or orbits

CREDITS
=======

*  [Equation of time](http://lexikon.astronomie.info/zeitgleichung/)
*  [Polygon vertex ordering](http://debian.fmi.uni-sofia.bg/~sergei/cgsr/docs/clockwise.htm)
