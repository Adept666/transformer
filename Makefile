CFLAGS = $(shell pkg-config --cflags gtk+-3.0)

LIBS = $(shell pkg-config --libs gtk+-3.0)
LIBS += -lm

trans: main.c graph.c
	gcc $(CFLAGS) -g -o trans main.c graph.c $(LIBS)
