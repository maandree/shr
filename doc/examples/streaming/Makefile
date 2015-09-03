COMMANDS = send recv

all: ${COMMANDS}

%: %.c
	${CC} -Wall -Wextra -pedantic -std=c99 -lshr -o $@ $<

clean:
	-rm ${COMMANDS}


.PHONY: all clean

