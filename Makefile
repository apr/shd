
.PHONY: all clean test

all:
	make -C src

test:
	make -C src test

clean:
	make -C src clean

