#ifndef DEFINES_H_
#define DEFINES_H_

#define INF		1.0e14
#define EPS		1.0e-14
#define RAND_SEED	1.0

// Typedefs
typedef struct {
	int    rank;
	double constr_violation;
	double *xreal;
	int    **gene;
	double *xbin;
	double *obj;
	double *constr;
	double crowd_dist;
} individual;

typedef struct {
	individual *ind;
} population;

#endif
