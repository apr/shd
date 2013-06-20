
.PHONY: all clean test


SOURCES = \
	main.cc \
	buffered-connection.cc \
	io-buffer.cc \
	plm-connection.cc \
	plm-util.cc \
	select-server.cc \
	test-connection.cc \

all:
	g++ -g -o shd $(SOURCES)



test: io-buffer_test plm-util_test
	./io-buffer_test
	./plm-util_test

# Ugliness abounds, need to refactor the makefile to make libs.
io-buffer_test: io-buffer_test.cc io-buffer.h io-buffer.cc
	g++ -g -o $@ $< io-buffer.cc -lgtest -lgtest_main

plm-util_test: plm-util_test.cc plm-util.h plm-util.cc
	g++ -g -o $@ $< plm-util.cc -lgtest -lgtest_main

clean:
	rm -f shd *.o core
	rm -f io-buffer_test
	rm -f plm-util_test

