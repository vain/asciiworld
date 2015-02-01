# When running in a development environment, you don't need to change
# this. When building a package, though, you should supply a full path
# to a file that all users can access, i.e.:
#
#     make DEFAULT_MAP=/usr/share/asciiworld/ne_110m_land.shp
#
DEFAULT_MAP = `readlink -f ne_110m_land.shp`
CPPFLAGS += -DDEFAULT_MAP=\"$(DEFAULT_MAP)\"

CFLAGS += -Wall -Wextra -O3 `pkg-config --cflags gdlib`
LDLIBS += -lm -lshp `pkg-config --libs gdlib`

.PHONY: all clean
all: asciiworld

clean:
	rm -f asciiworld
