LLVM_INCLUDES=`llvm-config --includedir`
LLVM_LIBS=`llvm-config --libdir`
CXXFLAGS=-O2 -Wall -ggdb -I$(LLVM_INCLUDES) 

LDFLAGS=-L$(LLVM_LIBS) -Wl,-rpath,$(LLVM_LIBS) -lclang
test:

check:
	./test $(CFLAGS) test.c

clean:
	rm -f test
