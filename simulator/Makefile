#
# Makefile for pipeline
#

CC=g++

CFLAGS=-c -Wall
LDFLAGS=
SOURCES=main.cpp error_handling.cpp instruction_decoding.cpp stage.cpp tools.cpp
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=pipeline
INCLUDE=main.h

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.c ${INCLUDES}
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o $(EXECUTABLE)
