CC := agcc

SOURCES := main.c sock.c client.c dbase.c controller.c syscall.c init.c

CFLAGS=-c -Wall -Werror
LDFLAGS= -llog

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=PowerArbiter

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(EXECUTABLE)

