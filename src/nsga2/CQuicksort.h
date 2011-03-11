#pragma once

#include "CRand.h"
#include "defines.h"

class CQuicksort {
	public:
		CQuicksort(CRand* randgen);
		~CQuicksort(void);
		
		// Public quicksort methods
		void quicksort_front_obj(population *pop, int objcount, int obj_array[], int obj_array_size);
		void q_sort_front_obj(population *pop, int objcount, int obj_array[], int left, int right);
		void quicksort_dist(population *pop, int *dist, int front_size);
		void q_sort_dist(population *pop, int *dist, int left, int right);
		
		// Pointer to random number generator class
		CRand* rgen;
};
