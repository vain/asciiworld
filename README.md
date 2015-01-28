Having a little bit of fun.

![asciiworld](/asciiworld.png?raw=true)

Inspired by the world map in [blessed-contrib](https://github.com/yaronn/blessed-contrib).

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

`-s`: Turn on "day-and-night" mode. Areas that are currently lit by the sun will be shown as yellow, whereas the other parts are shown as dark grey. The "position" of the sun itself will be marked with an "S".

`-T`: Do not print the final newline to avoid scrolling. Using `-sT`, you can show a continuously updated map in "day-and-night" mode:

    tput civis; while ./asciiworld -sT; do sleep 5; tput home; done

`-p proj`: Choose projection. By default, a trivial equirectanguar projection will be used. The following other projections are available:

*  `kav`: Kavrayskiy VII
*  `lam`: Lambert cylindrical equal-area

`-b`: Draw a border around the projected world. This can be useful for some projections.

`-C`: Disable colors.

`-o`: Draw land masses as filled polygons. You'll get garbled results when using this together with country borders.

More ideas
==========

*  Zoom/pan/crop.
*  Highlight areas.
*  Highlight countries.

CREDITS
=======

*  [Bresenham](https://de.wikipedia.org/wiki/Bresenham-Algorithmus#C-Implementierung)
*  [Equation of time](http://lexikon.astronomie.info/zeitgleichung/)
