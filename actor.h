#ifndef _ACTOR_H_
#define _ACTOR_H_

#include <mpi.h>

void send_mesg(void *buf, int count, MPI_Datatype type, int dest, int tag, MPI_Comm comm);


#endif