Tracking satellites
===================

This, again, is very much influenced by [trehn's termtrack](https://github.com/trehn/termtrack).

You need [pyephem](http://rhodesmill.org/pyephem/).

![tracksats](/tracksats.png?raw=true)

getsat
------

Get satellite data:

    ./getsat 25544

Here's an index: [Master TLE Index](http://www.celestrak.com/NORAD/elements/master.asp).

The script simply prints the data on STDOUT.

calcsat
-------

This scripts reads TLE data from STDIN.

    ./getsat 25544 | ./calcsat

It creates a point and two tracks for asciiworld: The current position of the satellite itself, the next full orbit and 5% of the previous orbit.

Combining the tools with asciiworld: tracksats
----------------------------------------------

`$1` is your location, followed by all satellites you want to track:

    ./tracksats '50 8' 25544 25078
