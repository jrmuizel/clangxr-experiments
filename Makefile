CFLAGS=-O2 -Wall -I$(HOME)/usr/include/ -std=gnu99
LDFLAGS=-L$(HOME)/usr/lib -Wl,-rpath,$(HOME)/usr/lib -lclang
test:
