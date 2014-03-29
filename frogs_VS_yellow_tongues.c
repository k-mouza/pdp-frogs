#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <getopt.h>

#include "bio_model.h"
#include "frog-functions.h"
#include "process_pool/pool.h"


int main(int argc, char **argv)
{
	long seed;
	int c;
	
	init_default_values();
	
	while ((c = getopt(argc, argv, "f:i:y:h")) != -1)
		switch (c) {
		case 'f':
			INIT_FROGS = atoi(optarg);
			break;
		case 'i':
			INIT_INFECTED = atoi(optarg);
			break;
		case 'y':
			YEARS = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Unknown option -%c\n", c);
		case 'h':
			print_usage();
			exit(-1);
			break;
		}
	
	MPI_Init(&argc, &argv);
	
	/*
	 * Initialise the process pool.
	 * The return code is = 1 for worker to do some work, 0 for do nothing and stop and 2 for this is the master so call master poll
	 * For workers this subroutine will block until the master has woken it up to do some work
	 */
	int statusCode = processPoolInit();
	
	int mytype = getType(statusCode);
	if (mytype == TYPE_FROG)
	{
		// A frog worker so do the worker tasks
		frogCode();
	}
	else if (mytype == TYPE_CELL)
	{
		// A cell worker so do the worker tasks
		cellCode();
	}
	else if (mytype == TYPE_MASTER)
	{
		masterCode();
	}
	
	//printf("\nRank %d exiting....................................\n", getRank());
	processPoolFinalise();
	MPI_Finalize();
	return 0;
}
