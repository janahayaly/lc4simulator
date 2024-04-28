all: clean trace

trace: LC4.o loader.o trace.c
	clang -g LC4.o loader.o trace.c -o trace

LC4.o: 
	clang -c LC4.c -o LC4.o 

loader.o: 
	clang -c loader.c -o loader.o

clean:
	rm -rf *.o

clobber: clean
	rm -rf trace
