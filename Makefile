CFLAGS += -Wall -Wextra -O3
LDLIBS += -lm -lshp

.PHONY: all clean
all: asciiworld

clean:
	rm -f asciiworld
