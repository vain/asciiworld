Note that all tools might rely on all other tools (including asciiworld itelsef) to be available in your `$PATH`.

Tracking satellites
===================

This, again, is very much influenced by [trehn's termtrack](https://github.com/trehn/termtrack).

![asciiworld-sat-track](/asciiworld-sat-track.png?raw=true)

asciiworld-sat-get
------------------

Get satellite data:

    asciiworld-sat-get 25544

Here's an index: [Master TLE Index](http://www.celestrak.com/NORAD/elements/master.asp).

The script simply prints the data on STDOUT.

Dependencies:

*  curl

asciiworld-sat-calc
-------------------

This scripts reads TLE data from STDIN.

    asciiworld-sat-get 25544 | asciiworld-sat-calc

It creates a point and two tracks for asciiworld: The current position of the satellite itself, the next full orbit and 5% of the previous orbit.

Dependencies:

*  Python 3
*  [pyephem](http://rhodesmill.org/pyephem/)

Combining the tools with asciiworld: asciiworld-sat-track
---------------------------------------------------------

`$1` is your location, followed by all satellites you want to track:

    asciiworld-sat-track '50 8' 25544 25078

Dependencies:

*  tput from ncurses

TCP/IP connections
==================

![asciiworld-tcp-monitor](/asciiworld-tcp-monitor.png?raw=true)

asciiworld-ip-geo
-----------------

Performs a GeoIP database lookup:

    asciiworld-ip-geo 91.198.174.192

It prints latitude and longitude on STDOUT.

Dependencies:

*  [pygeoip](http://pypi.python.org/pypi/pygeoip)
*  [The GeoIP City Database](http://www.maxmind.com). For Arch Linux, install [geoip-citydata](https://aur.archlinux.org/packages/geoip-citydata) from the AUR.

The script contains the path to the database. Packagers should replace that path during packaging.

Combining the tools with asciiworld: asciiworld-tcp-monitor
-----------------------------------------------------------

`$1` is your location:

    asciiworld-tcp-monitor '50 8'

Dependencies:

*  GNU awk
*  ss from iproute2
*  tput from ncurses

Shortest paths
==============

asciiworld-waypoints
--------------------

Given a series of locations, this script can calculate tracks between them. For example, this takes you from Mainz over New York City, Hawaii, Tokyo, Bangkok, and Perth to Cape Town:

    asciiworld-waypoints '50 8' '40 -74' '21 -157' '35 139' '13 100' '-31 115' '-33 18'

The script prints the resulting tracks on STDOUT.

Dependencies:

*  [GeographicLib (Python)](http://geographiclib.sourceforge.net/html/other.html#python)
