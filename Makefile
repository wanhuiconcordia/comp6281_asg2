INCLPATH +=/usr/include
#CFLAGS += -std=c99
CFLAGS += -std=gnu11
OBJECTS = tools.o main.o

utility: $(OBJECTS)
	mpicc $(OBJECTS) $(LIBS) -o main

tools.o: tools.c tools.h
	mpicc $(CFLAGS) -o tools.o -c tools.c -I$(INCLPATH)
main.o: main.c
	gcc $(CFLAGS) -o main.o -c main.c -I$(INCLPATH)
clean:
	rm *.o

