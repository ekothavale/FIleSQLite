
main: main.o bplus.o scanner.o debug.o chunk.o page.o memory.o tableIO.o testing.o
	clang -fsanitize=address main.o bplus.o scanner.o debug.o chunk.o memory.o page.o tableIO.o testing.o -o main
	rm *.o

bplus.o: bplus.c bplus.h common.h
	clang -c bplus.c -o bplus.o

chunk.o: chunk.c chunk.h common.h
	clang -c chunk.c -o chunk.o

debug.o: debug.c debug.h common.h
	clang -c debug.c -o debug.o

main.o: main.c scanner.h common.h debug.h testing.h 
	clang -c main.c -o main.o

memory.o: memory.c memory.h
	clang -c memory.c -o memory.o

page.o: page.c page.h common.h

scanner.o: scanner.c scanner.h common.h memory.h
	clang -c scanner.c -o scanner.o

tableIO.o: tableIO.c tableIO.h
	clang -c tableIO.c -o tableIO.o

testing.o: testing.c testing.h bplus.h page.h
	clang -c testing.c -o testing.o

