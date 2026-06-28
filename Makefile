
SRC  = src
SQL  = src/SQL_interpreter
STOR = src/storage_engine
CFLAGS = -I$(SRC) -I$(SQL) -I$(STOR)

main: main.o bplus.o scanner.o debug.o chunk.o page.o memory.o tableIO.o testing.o parser.o
	clang -fsanitize=address main.o bplus.o scanner.o debug.o chunk.o memory.o page.o tableIO.o testing.o parser.o -o main
	rm *.o

bplus.o: $(STOR)/bplus.c $(STOR)/bplus.h $(SRC)/common.h
	clang $(CFLAGS) -c $(STOR)/bplus.c -o bplus.o

chunk.o: $(SQL)/chunk.c $(SQL)/chunk.h $(SRC)/common.h
	clang $(CFLAGS) -c $(SQL)/chunk.c -o chunk.o

debug.o: $(SRC)/debug.c $(SRC)/debug.h $(SRC)/common.h
	clang $(CFLAGS) -c $(SRC)/debug.c -o debug.o

main.o: $(SRC)/main.c $(SQL)/scanner.h $(SRC)/common.h $(SRC)/debug.h $(STOR)/testing.h
	clang $(CFLAGS) -c $(SRC)/main.c -o main.o

memory.o: $(SQL)/memory.c $(SQL)/memory.h
	clang $(CFLAGS) -c $(SQL)/memory.c -o memory.o

page.o: $(STOR)/page.c $(STOR)/page.h $(SRC)/common.h
	clang $(CFLAGS) -c $(STOR)/page.c -o page.o

parser.o: $(SQL)/parser.c $(SQL)/scanner.h
	clang $(CFLAGS) -c $(SQL)/parser.c -o parser.o

scanner.o: $(SQL)/scanner.c $(SQL)/scanner.h $(SRC)/common.h $(SQL)/memory.h
	clang $(CFLAGS) -c $(SQL)/scanner.c -o scanner.o

tableIO.o: $(STOR)/tableIO.c $(STOR)/tableIO.h
	clang $(CFLAGS) -c $(STOR)/tableIO.c -o tableIO.o

testing.o: $(STOR)/testing.c $(STOR)/testing.h $(STOR)/bplus.h $(STOR)/page.h
	clang $(CFLAGS) -c $(STOR)/testing.c -o testing.o
