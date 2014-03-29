CC = mpicc
CFLAGS = -lm 

INCDIR = include/
SOURCEDIR = src/
ACTORDIR = actor_framework/

OBJ = $(SOURCEDIR)frogs_VS_yellow_tongues.o $(SOURCEDIR)frog-functions.o $(SOURCEDIR)bio_model.o $(SOURCEDIR)ran2.o $(ACTORDIR)actor.o $(ACTORDIR)pool.o
EXEC = frogs_VS_yellow_tongues

all: exec

exec: $(OBJ) Makefile
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ)
		
%.o: %.c *.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -rf $(OBJ) $(EXEC)
