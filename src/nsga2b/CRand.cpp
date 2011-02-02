#include "CRand.h"

CRand::CRand(double srand) {
	seed = srand;
}


CRand::~CRand(void) {
}

/* Get seed number for random and start it up */
void CRand::randomize() {
    for( int j1 = 0; j1 <= 54; j1++ )
        oldrand[j1] = 0.0;

	jrand = 0;
    warmup_random(seed);
}

/* Get randomize off and running */
void CRand::warmup_random (double seed) {
    int ii;
    double new_random, prev_random;
    oldrand[54] = seed;
    new_random = 0.000000001;
    prev_random = seed;
    for(int j1 = 1; j1 <= 54; j1++ ) {
        ii = (21*j1)%54;
        oldrand[ii] = new_random;
        new_random = prev_random-new_random;
        if( new_random < 0.0 )
            new_random += 1.0;

		prev_random = oldrand[ii];
    }
    advance_random ();
    advance_random ();
    advance_random ();
    jrand = 0;
}

/* Create next batch of 55 random numbers */
void CRand::advance_random () {
    int j1;
    double new_random;
    for(j1=0; j1<24; j1++) {
        new_random = oldrand[j1]-oldrand[j1+31];
        if(new_random<0.0)
            new_random = new_random+1.0;

        oldrand[j1] = new_random;
    }
    for(j1=24; j1<55; j1++) {
        new_random = oldrand[j1]-oldrand[j1-24];
        if(new_random<0.0)
            new_random = new_random+1.0;

        oldrand[j1] = new_random;
    }
}

/* Fetch a single random number between 0.0 and 1.0 */
double CRand::randomperc() {
    jrand++;
    if(jrand>=55) {
        jrand = 1;
        advance_random();
    }
    return((double)oldrand[jrand]);
}

/* Fetch a single random integer between low and high including the bounds */
int CRand::rnd (int low, int high) {
    int res;
    if (low >= high) res = low;
    else {
        res = low + (int)(randomperc()*(high-low+1));
        if (res > high) res = high;
    }
    return (res);
}

/* Fetch a single random real number between low and high including the bounds */
double CRand::rndreal (double low, double high) {
    return (low + (high-low)*randomperc());
}
