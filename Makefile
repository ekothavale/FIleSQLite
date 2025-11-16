
main: main.o bplus.o scanner.o debug.o chunk.o memory.o
	gcc -fsanitize=address main.o bplus.o scanner.o debug.o chunk.o memory.o -o main
	rm *.o

bplus.o: bplus.c bplus.h common.h
	gcc -c bplus.c -o bplus.o

chunk.o: chunk.c chunk.h common.h
	gcc -c chunk.c -o chunk.o

debug.o: debug.c debug.h common.h
	gcc -c debug.c -o debug.o

main.o: main.c scanner.h common.h debug.h
	gcc -c main.c -o main.o

memory.o: memory.c memory.h
	gcc -c memory.c -o memory.o

scanner.o: scanner.c scanner.h common.h memory.h
	gcc -c scanner.c -o scanner.o

