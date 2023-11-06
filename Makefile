#SHELL = /bin/bash

CC = gcc

INCLUDE = -I ./includes
CFLAGS = -Wall -pedantic -std=c99 -pthread $(INCLUDE)

EXE1 = farm
EXE2 = collector 
Q = -q 8

N = -n 4

T = -t 1000

D = -d testdir

DIR = testdir
OBJ = obj/masterWorkerMain.o obj/threadpool.o obj/util.o obj/worker.o obj/collector.o 
FILE = file1.dat file2.dat file3.dat file4.dat file5.dat file10.dat file12.dat file13.dat file14.dat file15.dat file16.dat file17.dat file18.dat file20.dat file100.dat file116.dat file117.dat

.PHONY : clean  cleanall test generafile mytest exec valg looptest

$(EXE1) : obj/masterWorkerMain.o obj/threadpool.o obj/util.o obj/worker.o
	$(CC)  $(CFLAGS) $^ -o $(EXE1)

$(EXE2) : obj/collector.o  obj/util.o 
	$(CC) $(CFLAGS) $^ -o $(EXE2)

generafile: generafile.o
	$(CC) -std=c99 generafile.o -o generafile

generafile.o: generafile.c
	$(CC) -std=c99 generafile.c -c generafile.c

obj/util.o : src/util.c includes/util.h 
	$(CC) $(CFLAGS) -c $< -o obj/util.o

obj/threadpool.o : src/threadpool.c includes/threadpool.h 
	$(CC) $(CFLAGS) -c $< -o obj/threadpool.o 

obj/worker.o : src/worker.c includes/worker.h includes/threadpool.h includes/communication.h
	$(CC) $(CFLAGS) -c $< -o obj/worker.o 

obj/collector.o : src/collector.c  includes/util.h includes/communication.h
	$(CC) $(CFLAGS) -c $< -o obj/collector.o

obj/masterWorkerMain.o : src/masterWorkerMain.c includes/util.h includes/communication.h includes/threadpool.h includes/worker.h
	$(CC) $(CFLAGS) -c $< -o obj/masterWorkerMain.o


test: farm collector 
	@chmod +x ./test.sh
	./test.sh

mytest: farm collector 
	@chmod +x ./my_test.sh
	./my_test.sh

looptest: farm collector 
	@chmod +x ./loop.sh
	./loop.sh

clean :
	-rm -f *~ *.dat farm.sck expected.txt core \
	rm -r $(DIR)

cleanall :
	-rm -f $(EXE1) $(EXE2) generafile $(OBJ) generafile.o *~ *.dat testdir ./farm.sck expected.txt core \
	rm -r $(DIR)

exec: 
	./$(EXE1) $(D) $(T) $(N) $(Q) $(FILE)

valg:
	valgrind --error-exitcode=1 --leak-check=full ./$(EXE1) $(D) $(T) $(N) $(Q) $(FILE)