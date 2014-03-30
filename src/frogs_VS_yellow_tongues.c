#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <getopt.h>

#include "../include/bio_model.h"
#include "../include/frog-functions.h"
#include "../actor_framework/pool.h"


int main(int argc, char **argv)
{
	long seed;
	int c;
	int frogs = -1, infected = -1, total_years = -1, limit = -1;
	
	
	init_environment(&argc, &argv);
	
	/*
	 * Initialise the process pool.
	 * The return code is = 1 for worker to do some work, 0 for do nothing and stop and 2 for the master.
	 * For workers this subroutine will block until the master has woken it up to do some work.
	 */
	int statusCode = processPoolInit();
	
	init_default_values();
	
	// parse command line arguments
	while ((c = getopt(argc, argv, "f:i:y:l:h")) != -1)
		switch (c) {
		case 'f':
			frogs = atoi(optarg);
			break;
		case 'i':
			infected = atoi(optarg);
			break;
		case 'y':
			total_years = atoi(optarg);
			break;
		case 'l':
			limit = atoi(optarg);
			break;
		default:
			if (getRank() == 0) fprintf(stderr, "Unknown option -%c\n", c);
		case 'h':
			if (getRank() == 0) print_usage();
			force_terminate("");
			break;
		}
		
	parse_given_values(frogs, infected, total_years, limit);

	/*
	 * Get type of worker (frog or cell) or master type and
	 * call the corresponding function
	 */
	int mytype = getType(statusCode);
	
	if (mytype == TYPE_FROG)
	{
		// A frog worker
		frogCode();
	}
	else if (mytype == TYPE_CELL)
	{
		// A cell worker
		cellCode();
	}
	else if (mytype == TYPE_MASTER)
	{
		// The Master
		masterCode();
	}
	
	processPoolFinalise();
	MPI_Finalize();
	return 0;
}
