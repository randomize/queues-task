
CC=gcc
CFLAGS= -c -Wall -I. -Wall -Wextra -Wpedantic -std=c11
LDFLAGS=-lcmocka
SOURCES=main.c queue2.c
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
