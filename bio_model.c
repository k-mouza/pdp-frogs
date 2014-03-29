#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#include "bio_model.h"
#include "process_pool/pool.h"
#include "frog-functions.h"
#include "actor.h"

static int NUM_OF_CELLS;	// total number of cells
static int INIT_FROGS;		// initial number of frogs
static int FROG_LIMIT;		// frog limit
static int INIT_INFECTED;	// initial number of infected frogs
static int YEARS;			// total years for simulation


void print_usage(void)
{
	fprintf(stderr, "\nThe arguments you can provide are:\n"
		"\t'-f' <initial number of frogs>\n"
		"\t'-i' <initial number of infected frogs>\n"
		"\t'-y' <years to simulate>\n"
		"\t'-l' <frog limit>\n"
		"\t'-h' Prints this message\n\n");
}


void init_default_values(void)
{
	NUM_OF_CELLS = 16;
	INIT_FROGS = 34;
	INIT_INFECTED = 4;
	YEARS = 100;
	FROG_LIMIT = 100;
}


void parse_given_values(int frogs, int infected, int total_years, int limit)
{
	if (frogs >= 0) INIT_FROGS = frogs;
	if (infected >= 0) INIT_INFECTED = infected;
	if (total_years >= 0) YEARS = total_years;
	if (limit >= 0) FROG_LIMIT = limit;
}


int getType(int status)
{
	if (status == 2)
		return TYPE_MASTER;
	else if (status == 1 && getRank() <= NUM_OF_CELLS)
		return TYPE_CELL;
	else if (status == 1)
		return TYPE_FROG;
	else
		return TYPE_UNUSED;
}


static int getAliveFrogs(void)
{
	return getActiveWorkers() - NUM_OF_CELLS;
}


static frog_t* newFrog(float x, float y)
{
	int i;
	frog_t *frog  = (frog_t *)malloc(sizeof(frog_t));
	
	frog->pos.x = x;
	frog->pos.y = y;
	frog->infected = 0;
	frog->hops = 0;
	frog->inf_hops = 0;
	frog->sum_popInflux = 0;
	frog->sum_infLevel = 0;
	
	for(i=0; i<500; i++)
		frog->infLevel[i] = 0;
	
	return frog;
}


static cell_t* newCell(void)
{
	cell_t *cell = (cell_t *)malloc(sizeof(cell_t));
	
	cell->populationInflux = 0;
	cell->infectionLevel = 0;
	
	return cell;
}


void masterCode(void)
{	
	long seed = -1-getRank();
	initialiseRNG(&seed);
	
	time_t print_time, print_secs = 1;	// time variables for printing alive frogs
	time_t year_time, year_secs = 2;	// time variables to count the years
	int curr_year = 0;					// current year
	
	/*
	 * command to send to cell:
	 * STOP_CELL for termination,
	 * PRINT_CELL to print its data,
	 * STOP_FROGS to stop remaining polling frogs and exit
	 */
	int cell_command;
										

	int i, workerPid;
	
	// starting cell workers
	for (i=0; i<NUM_OF_CELLS; i++)
	{
		workerPid = startWorkerProcess();
		printf("Master started Cell worker %d on MPI process %d\n", i+1 , workerPid);
	}

	//starting frog workers
	int infected;
	for (i=0; i<INIT_FROGS; i++)
	{
		workerPid = startWorkerProcess();

		// the first INIT_INFECTED frogs are the initially infected frogs
		infected = 0;
		if (i < INIT_INFECTED) infected = 1;
		printf("Master started Frog worker %d on MPI process %d (infected:%d)\n", i+1 , workerPid, infected);
		
		// send the frog a message telling him if he is infected or not
		send_mesg(&infected, 1, MPI_INT, workerPid, INF_TAG, MPI_COMM_WORLD);
	}

	int masterStatus = masterPoll();
	
	print_time = time(NULL) + print_secs;
	year_time = time(NULL) + year_secs;
	
	while (getAliveFrogs() > 0)
	{
		// Each call to master poll will block until a message is received and then will handle it
		masterStatus = masterPoll();

		if (time(NULL) >= year_time)
		{
			// A year has passed. Instruct cells to print their data.
			cell_command = PRINT_CELL;
			
			printf("YEAR %d\n", curr_year);
			
			for (i=0; i<NUM_OF_CELLS; i++)
			{
				send_mesg(&cell_command, 1, MPI_INT, i+1, HOP_TAG, MPI_COMM_WORLD);
			}
			
			curr_year++;
			year_time = time(NULL) + year_secs;
		}
		
		if (curr_year == YEARS)
		{
			// Simulation end
			printf("SIMULATION END. NUMBER OF FROGS LEFT: %d\n", getAliveFrogs());
			
			// instruct cell to stop frogs polling to them
			cell_command = STOP_FROGS;
			for (i=0; i<NUM_OF_CELLS; i++)
			{
				send_mesg(&cell_command, 1, MPI_INT, i+1, HOP_TAG, MPI_COMM_WORLD);
			}
			curr_year++; // trick to prevent master from entering this if statement more than once
		}

		if (time(NULL) >= print_time && curr_year <= YEARS)
		{
			// A second has passed so print alive frogs
			printf("Alive frogs: %d\n", getAliveFrogs());
			print_time = time(NULL) + print_secs;
		}
		
		if ( (getAliveFrogs() == 0) && (curr_year < YEARS) )
		{
			// All frogs died before the end of simulation
			printf("ALL FROGS ARE DEAD. EXITING...\n");
			break;	
		}
		
		// if the limit is reached, abort
		if (getAliveFrogs() >= FROG_LIMIT)
			force_terminate("\tFROG LIMIT REACHED. EXITING...\n");
	}

	// stop cells, which are polling for frog hops
	int stop_cell = STOP_CELL;
	for (i=0; i<NUM_OF_CELLS; i++)
	{
		send_mesg(&stop_cell, 1, MPI_INT, i+1, HOP_TAG, MPI_COMM_WORLD);
	}
}


void frogCode(void)
{	
	long seed = -1-getRank();
	initialiseRNG(&seed);
	
	int workerStatus = 1;		// status of process after a frog assigned to it dies
	int cellnum;				// rank of the cell that the frog has hopped on
	int cell_values[2];			// data received from cell
	point_t start_pos;			// starting position of frog
	frog_t *my_frog;			// main frog struct
	int parent;					// parent of frog
    MPI_Datatype mpi_t_point;
	MPI_Status status;
	
	init_type(&mpi_t_point);
	
	while (workerStatus)
	{
		parent = getCommandData(); // find out the parent
		if (parent == 0)
			// initial position if the parent is the master
			frogHop(0, 0, &start_pos.x, &start_pos.y, &seed);
		else
			// receive the initial position from my parent
			recv_mesg(&start_pos, 1, mpi_t_point, parent, POS_TAG, MPI_COMM_WORLD, &status);
	
		my_frog = newFrog(start_pos.x, start_pos.y);
	
		if (parent == 0)
			// master tells me if I am infected or not
			recv_mesg(&my_frog->infected, 1, MPI_INT, 0, INF_TAG, MPI_COMM_WORLD, &status);

		// main frog loop
		while(1)
		{
			if (rand() < 0.666666666*((double)RAND_MAX + 1.0)) // hop with a 4/6 propability
			{			
				frogHop(start_pos.x, start_pos.y, &my_frog->pos.x, &my_frog->pos.y, &seed);
				cellnum = getCellFromPosition(my_frog->pos.x, my_frog->pos.y);

				// inform the cell I have hopped on it and tell it if I am infected
				send_mesg(&my_frog->infected, 1, MPI_INT, cellnum, HOP_TAG, MPI_COMM_WORLD);
				// receive the values I need to know from the cell
				recv_mesg(cell_values, 2, MPI_INT, cellnum, HOP_TAG, MPI_COMM_WORLD, &status);

				if (cell_values[0] + cell_values[1] < 0)
				{
					// if the values are negative, it means the simulation has ended
					free(my_frog);
					workerStatus = workerSleep();
					break;
				}
				
				// update the sum population influxes
				my_frog->sum_popInflux += cell_values[0];
				
				/*
				 * Instead of adding all the elements of the array every step,
				 * we subtract, from the sum, the oldest infection level, from 500 hops ago,
				 * replace it with the new one and add it to the sum.
				 */
				my_frog->sum_infLevel -= my_frog->infLevel[my_frog->hops % 500];
				my_frog->infLevel[my_frog->hops % 500] = cell_values[1];
				my_frog->sum_infLevel += cell_values[1];
			
				if ( (my_frog->hops >= 300) && (my_frog->hops % 300 == 0) )
				{
					// every 300 hops check if I will reproduce
					if (willGiveBirth(my_frog->sum_popInflux/300.0, &seed))
					{
						int child = startWorkerProcess();
						// send my position to the child
						send_mesg(&my_frog->pos, 1, mpi_t_point, child, POS_TAG, MPI_COMM_WORLD);
					}
					my_frog->sum_popInflux = 0;
				}
			
				if ( my_frog->hops >= 500 && !my_frog->infected)
				{
					// if I am not infected, check to see if I will catch the disease
					if (willCatchDisease(my_frog->sum_infLevel/500.0, &seed))
						my_frog->infected = 1;
				}
			
				if ( (my_frog->inf_hops >= 1700) && (my_frog->inf_hops % 1700 == 0) )
				{
					// every 700 infected hops check to see if I will die
					if (willDie(&seed))
					{
						free(my_frog);
						workerStatus = workerSleep();
						break;
					}
				}

				// update the position
				start_pos.x = my_frog->pos.x;
				start_pos.y = my_frog->pos.y;
				
				// update the hop counters
				my_frog->hops++;
				if (my_frog->infected) my_frog->inf_hops++;
			}
		}
	}
	free_type(&mpi_t_point);
}


void cellCode(void)
{
	long seed = -1-getRank();
	initialiseRNG(&seed);
	
	int frog_infection, send_to_frog[2];
	int stop_frog = 0;
	MPI_Status status;
	MPI_Request request[2];
	
	cell_t *my_cell = newCell();
		
	while(1)
	{
		recv_mesg(&frog_infection, 1, MPI_INT, MPI_ANY_SOURCE, HOP_TAG, MPI_COMM_WORLD, &status);
		
		if (frog_infection == STOP_CELL)
		{
			break;
		}
		else if (frog_infection == PRINT_CELL)
		{
			printf("Cell %d: \tpopulationInflux = %d\tinfectionLevel = %d\n", getRank(), my_cell->populationInflux, my_cell->infectionLevel);
			my_cell->populationInflux = 0;
			my_cell->infectionLevel = 0;
			continue;
		}
		else if (frog_infection == STOP_FROGS)
		{
			stop_frog = 1;
			continue;
		}
		
		my_cell->populationInflux++;
		my_cell->infectionLevel += frog_infection;

		//printf("Cell pop = %d, inflev = %d\n", my_cell->populationInflux, my_cell->infectionLevel);

		send_to_frog[0] = my_cell->populationInflux;
		send_to_frog[1] = my_cell->infectionLevel;
		
		if (stop_frog)
		{
			send_to_frog[0] = -1;
			send_to_frog[1] = -1;
		}
		
		send_mesg(send_to_frog, 2, MPI_INT, status.MPI_SOURCE, HOP_TAG, MPI_COMM_WORLD);
	}					
	
	free(my_cell);
}
