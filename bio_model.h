#ifndef _BIO_MODEL_H
#define _BIO_MODEL_H

#include "actor.h"

// types of processes
#define TYPE_UNUSED	-1
#define TYPE_CELL	0
#define TYPE_FROG	1
#define TYPE_MASTER	2

#define HOP_TAG 0	// tag for messages used to indicate a hop
#define INF_TAG 1	// tag for messages used to indicate if a frog is infected
#define POS_TAG 2	// tag for messages used to indicate the position of a spawned frog

#define STOP_CELL	-1	// cell command for termination
#define PRINT_CELL	2	// cell command for printing cell's data
#define STOP_FROGS	3	// cell command for stopping frogs polling to that cell

#define CHECK_BIRTH 300
#define CHECK_DEATH 1200
#define CHECK_INFEC	500

typedef struct frog frog_t;
struct frog
{
	point_t pos;				// frog position
	int infected;				// 0 if the frog is healthy or 1 if the frog is infected 
	int hops;					// number of hops so far
	int inf_hops;				// number of infected hops
	int sum_popInflux;			// the sum of populationInfluxes of the last 300 cells 
	int infLevel[CHECK_INFEC];	// the infectionLevel of each one of the last 500 cells 
	int sum_infLevel;			// the sum of infectionLevels of the last 500 cells 
};


typedef struct cell cell_t;
struct cell
{
	int populationInflux;
	int infectionLevel;
};

/*
 * Prints the usage for the executable
 */
void print_usage(void);

/*
 * Initialises default values for the model's variables
 */
void init_default_values(void);

/*
 * Sets the model's variables according to cmd line arguments
 */
void parse_given_values(int frogs, int infected, int total_years, int limit);

/*
 * Returns the type of the process (frog, cell, master)
 */
int getType(int status);

/*
 * Returns the number of alive frogs
 */
static int getAliveFrogs(void);

/*
 * Returns a new frog struct
 */
static frog_t* newFrog(float x, float y);

/*
 * Returns a new cell struct
 */
static cell_t* newCell(void);

/*
 * Function called by the master
 */
void masterCode(void);

/*
 * Function called by frog workers
 */
void frogCode(void);

/*
 * Function called by cell workers
 */
void cellCode(void);


#endif