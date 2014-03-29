#ifndef _ACTOR_H_
#define _ACTOR_H_

#include <mpi.h>

typedef struct point point_t;
struct point
{
  float x,y;
};

/*
 * Sends a message to dest using an MPI send call
 */
void send_mesg(void *buf, int count, MPI_Datatype type, int dest, int tag, MPI_Comm comm);

/*
 * Receives a message from source using an MPI recv call
 */
void recv_mesg(void *buf, int count, MPI_Datatype type, int source, int tag, MPI_Comm comm, MPI_Status *status);

/**
 * Writes an error message to stderr and MPI Aborts
 */
void force_terminate(char *message);

/*
 * Initialises and commits the desired type
 */
void init_type(MPI_Datatype *mpi_t_point);

/*
 * Frees the given type
 */
void free_type(MPI_Datatype *mpi_t_point);

#endif