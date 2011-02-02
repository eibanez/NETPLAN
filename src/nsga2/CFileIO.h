#pragma once

#include <cstdio>
#include "CNSGA2.h"
#include "defines.h"

class CNSGA2;

class CFileIO {
	public:
		CFileIO(CNSGA2* nsga2);
		~CFileIO(void);
		
		void flushIO();
		void recordConfiguration();
		void report_pop(population *pop, FILE *fpt);
		void report_feasible (population *pop, FILE *fpt);
		
		// File pointers
		FILE *fpt1;
	    FILE *fpt2;
	    FILE *fpt3;
	    FILE *fpt4;
	    FILE *fpt5;
		FILE *fpt6;

	private:
		// Pointer to CNSGA2 class for access to NSGA2 variables
		CNSGA2* p_nsga2;
};

