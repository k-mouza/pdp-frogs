CC = mpicc
CFLAGS = 

OBJ = frogs_VS_yellow_tongues.o frog-functions.o bio_model.o ran2.o process_pool/pool.o
EXEC = frogs_VS_yellow_tongues

all: exec

exec: $(OBJ) Makefile
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ)
		
%.o: %.c *.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -rf $(OBJ) $(EXEC)