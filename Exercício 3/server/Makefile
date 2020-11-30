# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2020-21

CC   = gcc
LD   = gcc
CFLAGS =-pthread -Wall -std=gnu99 -I../
LDFLAGS=-lm -lpthread

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs-server

tecnicofs-server: fs/state.o fs/operations.o tecnicofs-server.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-server fs/state.o fs/operations.o tecnicofs-server.o

fs/state.o: fs/state.c fs/state.h ../tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o fs/state.o -c fs/state.c

fs/operations.o: fs/operations.c fs/operations.h fs/state.h ../tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o fs/operations.o -c fs/operations.c

tecnicofs-server.o: tecnicofs-server.c fs/operations.h fs/state.h ../tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o tecnicofs-server.o -c tecnicofs-server.c

clean:
	@echo Cleaning...
	rm -f fs/*.o *.o tecnicofs-server outputs/*.txt 
