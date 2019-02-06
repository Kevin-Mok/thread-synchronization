all: carsim

CFILES=$(wildcard *.c)
ObjFILES=$(CFILES:.c=.o)
HFILES=$(wildcard *.h)

carsim: $(ObjFILES)
	$(CC) $(CFLAGS) -g -Wall -pthread -o $@ $^

%.o: %.c $(HFILES)
	gcc $(CFLAGS) -g -Wall -pthread -o $@ -c $<

clean:
	rm -f *.o
