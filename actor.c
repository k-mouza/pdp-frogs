#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "actor.h"
#include "process_pool/pool.h"

void send_mesg(void *buf, int count, MPI_Datatype type, int dest, int tag, MPI_Comm comm)
{
	MPI_Send(buf, count, type, dest, tag, comm);
}