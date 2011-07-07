// --------------------------------------------------------------
//    NETSCORE Version 2
//    Implementation of parallel NSGA-II
//    2009-2011 (c) Eduardo Ibanez and others
//    For more info:
//        http://natek85.blogspot.com/2009/07/c-nsga2-code.html
//        http://www.iitk.ac.in/kangal/codes.shtml
// --------------------------------------------------------------

using namespace std;
#include "CNSGA2.h"
#include <fstream>
#include <string>
#include <vector>
#include "../netscore.h"
ILOSTLBEGIN

CNSGA2* nsga2a = new CNSGA2(true, 1.0);
CNSGA2* nsga2b = new CNSGA2(false, 0.33);

int main (int argc, char **argv) {
	printHeader("nsga-parallel");
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	// Read indices
	ImportIndices();
	
	// -- Initialization of A -- //
	nsga2a->randgen->randomize();                   // Initialize random number generator
	nsga2a->Init("prepdata/param.in");              // This sets all variables related to GA
	nsga2a->InitMemory();                           // This allocates memory for the populations
	nsga2a->InitPop(nsga2a->parent_pop, Np_start);  // Initialize parent population randomly
	nsga2a->fileio->recordConfiguration();          // Records all variables related to GA configuration
	
	// -- Initialization of B -- //
	nsga2b->randgen->randomize();                   // Initialize random number generator
	nsga2b->Init("prepdata/param.in");              // This sets all variables related to GA
	nsga2b->InitMemory();                           // This allocates memory for the populations
	nsga2b->InitPop(nsga2b->child_pop, Np_start);   // Initialize child population randomly
	
	// Vector of capacity losses for events
	double events[(SLength[0] + IdxCap.size) * (Nevents+1)];
	ReadEvents(events, "prepdata/bend_events.csv");
	
	// Declare variables to store the optimization model
	CPLEX netplan;
	
	// Read optimization problem and store it in memory
	netplan.LoadProblem();
	
	cout << "- Initialization done, now performing first generation" << endl;
	
	// -- Evaluate 1A -- //
	nsga2a->decodePop(nsga2a->parent_pop);
	nsga2a->evaluatePop(nsga2a->parent_pop, netplan, events);
	nsga2a->assignRankCrowdingDistance(nsga2a->parent_pop);
	fprintf(nsga2a->fileio->fpt1,"# gen = 1A\n");
	nsga2a->fileio->report_pop(nsga2a->parent_pop, nsga2a->fileio->fpt1);
	
	for (int i = 1; i <= nsga2a->ngen; i++) {
		printHeader("elapsed");
		
		// -- Evaluate (i)A -- //
		if (i > 1) {
			nsga2a->selection(nsga2a->parent_pop, nsga2a->child_pop);
			nsga2a->mutatePop(nsga2a->child_pop);
			nsga2a->decodePop(nsga2a->child_pop);
			nsga2a->evaluatePop(nsga2a->child_pop, netplan, events);
			nsga2a->merge(nsga2b->parent_pop, nsga2a->child_pop, nsga2a->mixed_pop);
			nsga2a->fillNondominatedSort(nsga2a->mixed_pop, nsga2a->parent_pop);
		}
		
		// -- Report(i)A -- //
		fprintf(nsga2a->fileio->fpt4,"# gen = %dA\n", i);
		nsga2a->fileio->report_pop(nsga2a->parent_pop, nsga2a->fileio->fpt4);
		nsga2a->fileio->flushIO();
		cout << "- Finished generation #" << i << "A" << endl;
		
		// -- Evaluate (i)B -- //
		if (i > 1) {
			nsga2b->selection(nsga2b->parent_pop, nsga2b->child_pop);
			nsga2b->mutatePop(nsga2b->child_pop);
		}
		nsga2b->decodePop(nsga2b->child_pop);
		nsga2b->evaluatePop(nsga2b->child_pop, netplan, events);
		if (i == 1) {
			nsga2b->assignRankCrowdingDistance(nsga2b->child_pop);
			fprintf(nsga2a->fileio->fpt1,"# gen = 1B\n");
			nsga2a->fileio->report_pop(nsga2b->child_pop, nsga2a->fileio->fpt1);     // Initial pop out
		}
		nsga2b->merge(nsga2a->parent_pop, nsga2b->child_pop, nsga2b->mixed_pop);
		nsga2b->fillNondominatedSort(nsga2b->mixed_pop, nsga2b->parent_pop);
		
		// -- Report(i)B -- //
		fprintf(nsga2a->fileio->fpt4,"# gen = %dB\n", i);
		nsga2a->fileio->report_pop(nsga2b->parent_pop, nsga2a->fileio->fpt4);
		nsga2a->fileio->flushIO();
		
		cout << "- Finished generation #" << i << "B" << endl;
	}
	
	// -- Report final solution -- //
	nsga2a->fileio->report_pop(nsga2b->parent_pop, nsga2a->fileio->fpt2);
	nsga2a->fileio->report_feasible(nsga2b->parent_pop, nsga2a->fileio->fpt3);
	if (nsga2a->nreal!=0) {
		fprintf(nsga2a->fileio->fpt5, "\n Number of crossover of real variable = %d", nsga2a->nrealcross + nsga2b->nrealcross);
		fprintf(nsga2a->fileio->fpt5, "\n Number of mutation of real variable = %d", nsga2a->nrealmut + nsga2b->nrealmut);
	}
	if (nsga2a->nbin!=0) {
		fprintf(nsga2a->fileio->fpt5, "\n Number of crossover of binary variable = %d", nsga2a->nbincross + nsga2b->nbincross);
		fprintf(nsga2a->fileio->fpt5, "\n Number of mutation of binary variable = %d", nsga2a->nbinmut + nsga2b->nbinmut);
	}
	
	printHeader("completed");
	return (0);
}
