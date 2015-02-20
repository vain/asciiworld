Having a little bit of fun.

![asciiworld](/asciiworld.png?raw=true)

Inspired by:

*  The world map in [blessed-contrib](https://github.com/yaronn/blessed-contrib)
*  Fancy stuff in [trehn's termtrack](https://github.com/trehn/termtrack)

asciiworld is just the core program that renders the map. You can combine it with other tools, some of which are included in this repository. See [README-tools.md](/README-tools.md).

Building
========

Dependencies
------------

If you're using Arch Linux, you're fine with installing these two packages:

*  Drawing algorithms: [gd](https://www.archlinux.org/packages/extra/x86_64/gd/)
*  Reading maps: [shapelib](https://www.archlinux.org/packages/community/x86_64/shapelib/)

asciiworld comes with a bundled map:

*  [Natural Earth Data, Land](http://www.naturalearthdata.com/downloads/110m-physical-vectors/110m-land/)

If you want to show country borders, you could use this map:

*  [Natural Earth Data, Countries](http://www.naturalearthdata.com/downloads/110m-cultural-vectors/110m-admin-0-countries/)

Since asciiworld can read any "ESRI Shapefile", the other maps available at [Natural Earth Data](http://www.naturalearthdata.com) might be interesting as well. Note that I only test asciiworld with the 1:110m physical map.

Compiling
---------

Running `make` should be fine in most cases. If you get stuck, just read the Makefile. It's simple.

Command line options
====================

`-w W`, `-h H`: Force output size. By default, the full size of the terminal is used.

`-m path`: Path to an ESRI Shapefile (.shx or .shp).

`-l path`: Path to a file containing locations, tracks and sphere circles to highlight. The file format looks like this:

    points
    50 8
    -33.9 18.4
    .
    
    track
    0 0
    2 -1
    4 -2
    6 -3
    8 -4
    .
    
    circles
    50 8 10
    -23 -20 90
    .

All values are degrees. Empty and non-matching lines are ignored.

Points: Latitude, longitude. They are simply marked with an "X".

Tracks: Latitude, longitude. Tracks are rendered with a special charset, all points on a track have the same color. Note that, for the time being, asciiworld draws tracks as points, too. To create the illusion of a connected line, you should simply provide enough points. :-)

Circles: Latitude, longitude, radius. Latitude and longitude specify the circle's center. Radius is measured in degree, too: It's the angle between two vectors A and B, where A points from the sphere's center to the circle's center and B points from the sphere's center to an arbitrary point on the circle. Thus, if you specify a radius of 90 degree, you get a Great Circle. For less than 90 degree, you get a Small Circle.

`-s`: Turn on "day-and-night" mode. Areas that are currently lit by the sun will be shown as green, whereas the other parts are shown as dark dark blue. Areas currently in the "twilight zone" will be shaded accordingly (see `-d`). The "position" of the sun itself will be marked with an "S". A yellow line marks the current sunset/sunrise locations.

`-d dusk`: Choose definition of "dusk" and thus the twilight span. Accepts "nau" for nautical dusk (12 degree twilight) or "ast" for astronomical dusk (18 degree twilight). By default, civil dusk (6 degree twilight) is used.

`-T`: Do not print the final newline to avoid scrolling. Using `-sT`, you can show a continuously updated map in "day-and-night" mode:

    tput civis; while ./asciiworld -sT; do sleep 5; tput home; done

`-p proj`: Choose projection. By default, a trivial equirectanguar projection will be used. The following other projections are available:

*  `kav`: Kavrayskiy VII
*  `lam`: Lambert cylindrical equal-area
*  `ham`: Hammer

`-b`: Draw a border around the projected world. This can be useful for some projections.

`-c n`: Set to n colors. Accepts 0 or 8. Default is 256.

`-o`: Draw land masses as filled polygons. You'll get garbled results when using this together with country borders.

`-W path`: Instead of creating ASCII output, you can save the resulting image as an "uninterpreted" PNG file. You will want to use `-w` and `-h` together with this option.

CREDITS
=======

*  [Equation of time](http://lexikon.astronomie.info/zeitgleichung/)
*  [Polygon vertex ordering](http://debian.fmi.uni-sofia.bg/~sergei/cgsr/docs/clockwise.htm)
