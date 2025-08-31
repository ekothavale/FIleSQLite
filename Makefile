main.o:
    gcc -c main.c -o main.o

main: main.o
    gcc main.o -o main
    rm *.o