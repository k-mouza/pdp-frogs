#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#include "bio_model.h"
#include "process_pool/pool.h"
#include "frog-functions.h"

void print_usage(void)
{
	fprintf(stderr, "\nThe arguments you can provide are:\n"
		"\t'-f' <initial number of frogs>\n"
		"\t'-i' <initial number of infected frogs>\n"
		"\t'-y' <years to simulate>\n"
		"\t'-h' Prints this message\n\n");
}

void init_default_values(void)
{
	NUM_OF_CELLS = 16;
	INIT_FROGS = 34;
	INIT_INFECTED = 4;
	YEARS = 100;
}

int getAliveFrogs(void)
{
	return getActiveWorkers() - NUM_OF_CELLS;
}

void masterCode(void)
{
	/*
	 * This is the master, each call to master poll will block until a message is received and then will handle it and return
	 * 1 to continue polling and running the pool and 0 to quit.
	 */
	
	long seed = -1-getRank();
	initialiseRNG(&seed);
	
	time_t end, seconds = 1;
	int all_dead = 0;
	
	int i, workerPid;	
	for (i=0; i<NUM_OF_CELLS; i++)
	{
		workerPid = startWorkerProcess();
		printf("Master started Cell worker %d on MPI process %d\n", i+1 , workerPid);
	}

	int infected;
	for (i=0; i<INIT_FROGS; i++)
	{
		workerPid = startWorkerProcess();

		infected = 0;
		if (i < INIT_INFECTED) infected = 1;
		printf("Master started Frog worker %d on MPI process %d (infected:%d)\n", i+1 , workerPid, infected);
			
		MPI_Send(&infected, 1, MPI_INT, workerPid, INF_TAG, MPI_COMM_WORLD);
	}

	int masterStatus = masterPoll();
	
	end = time(NULL) + seconds;
	while (getAliveFrogs() > 0)
	{
		masterStatus = masterPoll();

		if (time(NULL) >= end)
		{
			printf("Alive frogs: %d\n", getAliveFrogs());
			end = time(NULL) + seconds;
		}
		
		if ( (getAliveFrogs() == 0) && (masterStatus == 1) )
		{
			printf("ALL FROGS ARE DEAD...EXITING\n");
			break;	
		}
		
		if (getAliveFrogs() >= 100)
		{
			printf("MORE THAN 100 FROGS! EXITING...\n");
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
	}

	int stop_cell = 2;
	for (i=0; i<NUM_OF_CELLS; i++)
	{
		MPI_Send(&stop_cell, 1, MPI_INT, i+1, HOP_TAG, MPI_COMM_WORLD);
	}
}

frog_t* newFrog(float x, float y)
{
	int i;
	frog_t *frog  = (frog_t *)malloc(sizeof(frog_t));
	
	frog->pos.x = x;
	frog->pos.y = y;
	frog->infected = 0;
	frog->hops = 0;
	frog->sum_popInflux = 0;
	frog->sum_infLevel = 0;
	
	for(i=0; i<500; i++)
		frog->infLevel[i] = 0;
	
	return frog;
}

cell_t* newCell(void)
{
	cell_t *cell = (cell_t *)malloc(sizeof(cell_t));
	
	cell->populationInflux = 0;
	cell->infectionLevel = 0;
	
	return cell;
}


void frogCode(void)
{	
	long seed = -1-getRank();
	initialiseRNG(&seed);
	
	int workerStatus = 1;
	int cellnum, cell_values[2];
	point_t start_pos;
	frog_t *my_frog;
	int parent;
    MPI_Datatype mpi_t_point;
	MPI_Status status;
	
	init_type(&mpi_t_point);
	
	while (workerStatus)
	{		
		parent = getCommandData();
		if (parent == 0)
			frogHop(0, 0, &start_pos.x, &start_pos.y, &seed); // initial position
		else
		{
			MPI_Recv(&start_pos, 1, mpi_t_point, parent, POS_TAG, MPI_COMM_WORLD, &status);
			//printf("was born from %d and my pos is (%.4f, %.4f)\n", parent, start_pos.x, start_pos.y);
		}
	
		my_frog = newFrog(start_pos.x, start_pos.y);
	
		if (parent == 0)
			MPI_Recv(&my_frog->infected, 1, MPI_INT, 0, INF_TAG, MPI_COMM_WORLD, &status);
	
		//printf("Frog %d starting at position (%.4f, %.4f)\n", getRank(), my_frog->pos.x, my_frog->pos.y);
	
		while(1)
		{
			if (rand() < 0.666666666*((double)RAND_MAX + 1.0)) // hop with a 4/6 propability
			{
				my_frog->hops++;
			
				frogHop(start_pos.x, start_pos.y, &my_frog->pos.x, &my_frog->pos.y, &seed);
				cellnum = getCellFromPosition(my_frog->pos.x, my_frog->pos.y);

				MPI_Send(&my_frog->infected, 1, MPI_INT, cellnum, HOP_TAG, MPI_COMM_WORLD);
				MPI_Recv(cell_values, 2, MPI_INT, cellnum, HOP_TAG, MPI_COMM_WORLD, &status);

				if (cell_values[0] + cell_values[1] < 0)
				{
					free(my_frog);
					workerStatus = workerSleep();
					break;
				}
				
				my_frog->sum_popInflux += cell_values[0];
				my_frog->sum_infLevel -= my_frog->infLevel[my_frog->hops % 500];
				my_frog->infLevel[my_frog->hops % 500] = cell_values[1];
				my_frog->sum_infLevel += cell_values[1];
			
				if (my_frog->hops % 300 == 0)
				{
					//printf("avg_pop_infl = %.5f\n", my_frog->sum_popInflux/300.0);
					if (willGiveBirth(my_frog->sum_popInflux/300.0, &seed))
					{
						int child = startWorkerProcess();
						//printf("i am %d and my child is %d\n", getRank(), child);

						MPI_Send(&my_frog->pos, 1, mpi_t_point, child, POS_TAG, MPI_COMM_WORLD);
					}
					my_frog->sum_popInflux = 0;
				}
			
				if ( my_frog->hops >= 500 && !my_frog->infected)
				{
					//printf("avg_inf_lvl = %.5f\n", my_frog->sum_infLevel/500.0);
					if (willCatchDisease(my_frog->sum_infLevel/500.0, &seed))
						my_frog->infected = 1;
				}
			
				if ( (my_frog->hops % 700 == 0) && (my_frog->infected == 1) )
				{
					if (willDie(&seed))
					{
						//printf("Frog %d dying after %d hops...\n", getRank(), my_frog->hops);
						free(my_frog);
						workerStatus = workerSleep();
						break;
					}
				}
			
				start_pos.x = my_frog->pos.x;
				start_pos.y = my_frog->pos.y;
			}
		}
	}
	MPI_Type_free(&mpi_t_point);
}

void cellCode(void)
{
	long seed = -1-getRank();
	initialiseRNG(&seed);
	
	int year = 0, frog_infected, send_to_frog[2];
	int terminate = 0;
	MPI_Status status;
	MPI_Request request[2];
	time_t end, seconds = 2;
	
	cell_t *my_cell = newCell();
		
	while(!terminate)
	{
		end = time(NULL) + seconds;
		while(time(NULL) < end)
		{
			MPI_Recv(&frog_infected, 1, MPI_INT, MPI_ANY_SOURCE, HOP_TAG, MPI_COMM_WORLD, &status);
			if(frog_infected > 1)
			{
				terminate = 1;
				shutdownPool();
				break;
			}
			my_cell->populationInflux++;
			my_cell->infectionLevel += frog_infected;
	
			//printf("Cell pop = %d, inflev = %d\n", my_cell->populationInflux, my_cell->infectionLevel);
	
			send_to_frog[0] = my_cell->populationInflux;
			send_to_frog[1] = my_cell->infectionLevel;
			
			if (year >= YEARS)
			{
				send_to_frog[0] = -1;
				send_to_frog[1] = -1;
				shutdownPool();
			}
			
			MPI_Send(send_to_frog, 2, MPI_INT, status.MPI_SOURCE, HOP_TAG, MPI_COMM_WORLD);
		}
		
		if ( (year < YEARS) && terminate)
			printf("Unfinished year %d:\tCell %d: \tpopulationInflux = %d\tinfectionLevel = %d\n", year, getRank(), my_cell->populationInflux, my_cell->infectionLevel);
			
		else if (year < YEARS)
			printf("Year %d:\tCell %d: \tpopulationInflux = %d\tinfectionLevel = %d\n", year, getRank(), my_cell->populationInflux, my_cell->infectionLevel);
		
		my_cell->populationInflux = 0;
		my_cell->infectionLevel = 0;
		
		year++;
	}
	
	free(my_cell);
}

void init_type(MPI_Datatype *mpi_t_point)
{
    point_t my_point;    
   // Define and commit a new datatype
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
