#ifndef _HELPER_FUNCTIONS_H
#define _HELPER_FUNCTIONS_H


void initialiseRNG(long *seed);

/*
 * returns the position of a frog after it has hopped
 * from position (x,y) in (x_new, y_new)
 */
void frogHop(float x, float y, float* x_new, float* y_new, long * state);

/*
 * given the average population of the cells visited in the last 300 hops,it returns a true-false value.
 * True (1) implies that the frog has given birth.
 * False (0) implies the frog has not given birth.
 */
int willGiveBirth(float avg_pop, long * state);

/*
 * given the average infection level of the cells visited in the last 500 hops, it returns a true- false value.
 * True implies that the frog has caught the disease.
 * False implies that the frog has not caught the disease.
 */
int willCatchDisease(float avg_inf_level, long * state);

/*
 * returns True with a fixed probability of 1 in 6.
 * A return value of True implies that the frog should die.
 */
int willDie(long * state);

/*
 * given an (x,y) co-ordinate pair, returns an integer representing
 * the land cell that this contains this position.
 * Return value is between 1 and 16 (corresponding to cell's rank)
 */
int getCellFromPosition(float x, float y);

#endif
