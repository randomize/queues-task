
CC=gcc
CFLAGS=-c -Wall -I. -Wall -Wextra -Wpedantic -std=c11
LDFLAGS=
SOURCES=main.c queue.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=queue

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
