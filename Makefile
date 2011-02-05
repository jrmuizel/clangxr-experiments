LLVM_INCLUDES=`llvm-config --includedir`
LLVM_LIBS=`llvm-config --libdir`
CFLAGS=-O2 -Wall -ggdb -I$(LLVM_INCLUDES) -std=gnu99

LDFLAGS=-L$(LLVM_LIBS) -Wl,-rpath,$(LLVM_LIBS) -lclang
test:

check:
	./test $(CFLAGS) test.cpp

clean:
	rm -f test
