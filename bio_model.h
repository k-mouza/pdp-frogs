#ifndef __BIO_MODEL_H
#define __BIO_MODEL_H

#define HOP_TAG 0
#define INF_TAG 1
#define POS_TAG 2

int NUM_OF_CELLS;
int INIT_FROGS;
int INIT_INFECTED;
int YEARS;

typedef struct point point_t;
struct point
{
  float x,y;
};

typedef struct frog frog_t;
struct frog
{
	point_t pos;			/* frog position */
	int infected;			/* 0 if the frog is healthy or 1 if the frog is infected */
	int hops;				/* number of hops so far */
	int sum_popInflux;		/* the sum of populationInfluxes of the last 300 cells */
	int infLevel[500];		/* the infectionLevel of each one of the last 500 cells */
	int sum_infLevel;		/* the sum of infectionLevels of the last 500 cells */
};


typedef struct cell cell_t;
struct cell
{
	int populationInflux;
	int infectionLevel;
};

void print_usage(void);

void init_default_values(void);

int getAliveFrogs(void);

void masterCode(void);

frog_t* newFrog(float x, float y);

cell_t* newCell(void);

void frogCode(void);

void cellCode(void);

void init_type(MPI_Datatype *mpi_t_point);


#endif