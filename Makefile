
main: main.o parser.o
	gcc main.o parser.o -o main
	rm *.o

parser.o: parser.c parser.h
	gcc -c parser.c -o parser.o

main.o: main.c parser.h common.h
	gcc -c main.c -o main.o

