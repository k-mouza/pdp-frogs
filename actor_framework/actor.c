#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "actor.h"
#include "pool.h"

#define MPI_BUFF_SIZE 10240

void init_environment(int *argc, char ***argv)
{
	MPI_Init(argc, argv);
	MPI_Buffer_attach(malloc(MPI_BUFF_SIZE), MPI_BUFF_SIZE);
}

void send_mesg(void *buf, int count, MPI_Datatype type, int dest, int tag, MPI_Comm comm)
{
	MPI_Bsend(buf, count, type, dest, tag, comm);
}

void recv_mesg(void *buf, int count, MPI_Datatype type, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
	MPI_Recv(buf, count, type, source, tag, comm, status);
}

void force_terminate(char *message)
{
	fprintf(stderr,"%s", message);
	MPI_Abort(MPI_COMM_WORLD, 1);
}

void init_type(MPI_Datatype *mpi_t_point)
{
    point_t my_point;    

    int          blocklength [2];
    MPI_Aint     displacement[2];
    MPI_Datatype datatypes   [2];

    MPI_Aint     startx,starty;
    MPI_Get_address(&(my_point.x),&startx);
    MPI_Get_address(&(my_point.y),&starty);

    blocklength [0] = 1;
    blocklength [1] = 1;
    displacement[0] = 0;
    displacement[1] = starty - startx;
    datatypes   [0] = MPI_INT;
    datatypes   [1] = MPI_INT;

    MPI_Type_create_struct(2, blocklength, displacement, datatypes, mpi_t_point);
    MPI_Type_commit(mpi_t_point);
}

void free_type(MPI_Datatype *mpi_t_point)
{
	MPI_Type_free(mpi_t_point);
}
