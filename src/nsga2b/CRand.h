#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

class CRand {
	public:
		CRand(double);
		~CRand(void);

		// Methods
		void 	randomize();
		void 	warmup_random(double seed);
		void 	advance_random();
		double 	randomperc();
		int 	rnd (int low, int high);
		double 	rndreal (double low, double high);

		// Variables
		double 	oldrand[55];
		int 	jrand;
		double 	seed;
};
