CFLAGS += -Wall -Wextra -O3 `pkg-config --cflags gdlib`
LDLIBS += -lm -lshp `pkg-config --libs gdlib`

.PHONY: all clean
all: asciiworld

clean:
	rm -f asciiworld
