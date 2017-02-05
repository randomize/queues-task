
CC=gcc
CFLAGS= -c -Wall -I. -Wall -Wextra -Wpedantic -std=gnu11
LDFLAGS=-lcmocka -lrt
SOURCES=main.c queue.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=queue

all: CFLAGS += -DNDEBUG -ggdb -O3
all: executable

debug: CFLAGS += -DDEBUG -ggdb -O0
debug: executable

executable: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
