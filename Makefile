
.PHONY: all clean test


CXXFLAGS = -g -O -Wall -Werror -std=c++11


LIBCORE_SRCS = \
	buffered-connection.cc \
	ini-file-parser.cc \
	io-buffer.cc \
	logger.cc \
	plm-connection.cc \
	plm-endpoint.cc \
	plm-util.cc \
	select-server.cc \
	shd-app.cc \
	shd-config.cc \
	sunrise-sunset.cc


SHD_SRCS = main.cc

ONOFF_SRCS = on-off.cc


TESTS = io-buffer_test.cc \
	ini-file-parser_test.cc \
	plm-util_test.cc \
	buffered-connection_test.cc \
	plm-connection_test.cc \
	plm-endpoint_test.cc


LIBCORE_OBJS = $(LIBCORE_SRCS:.cc=.o)
SHD_OBJS = $(SHD_SRCS:.cc=.o)
ONOFF_OBJS = $(ONOFF_SRCS:.cc=.o)
TESTS_OBJS = $(TESTS:.cc=.o)

DEPS = $(LIBCORE_SRCS:.cc=.d)
DEPS += $(SHD_SRCS:.cc=.d)
DEPS += $(ONOFF_SRCS:.cc=.d)
DEPS += $(TESTS:.cc=.d)


all: shd on-off


libcore.a: $(LIBCORE_OBJS)
	ar -rcs $@ $?


shd: $(SHD_OBJS) libcore.a
	g++ $(CXXFLAGS) -o $@ $^

on-off: $(ONOFF_OBJS) libcore.a
	g++ $(CXXFLAGS) -o $@ $^


TEST_TGTS = $(TESTS:.cc=)

test: $(TEST_TGTS)
	@for t in $^; do \
		./$$t; \
	done

%_test: %_test.o libcore.a
	g++ $(CXXFLAGS) -pthread -o $@ $^ -lgtest -lgtest_main


%.o: %.cc
	g++ $(CXXFLAGS) -c -o $@ $<

%.d: %.cc
	g++ $(CXXFLAGS) -MM -o $@ $<


clean:
	rm -f shd on-off libcore.a
	rm -f $(DEPS)
	rm -f $(LIBCORE_OBJS) $(SHD_OBJS) $(ONOFF_OBJS) $(TESTS_OBJS)
	rm -f $(TEST_TGTS)


-include $(DEPS)

