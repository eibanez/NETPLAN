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

CNSGA2* nsga2a = new CNSGA2(true, 1.0);
CNSGA2* nsga2b = new CNSGA2(false, 0.33);

int main (int argc, char **argv) {
	printHeader("nsga-parallel");
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	// Read indices
	ImportIndices();
	
	// -- Initialization of A -- //
	nsga2a->randgen->randomize();					// Initialize random number generator
	nsga2a->Init("prepdata/param.in");				// This sets all variables related to GA
	nsga2a->InitMemory();							// This allocates memory for the populations
	nsga2a->InitPop(nsga2a->parent_pop, Np_start);	// Initialize parent population randomly
	nsga2a->fileio->recordConfiguration();			// Records all variables related to GA configuration
	
	// -- Initialization of B -- //
	nsga2b->randgen->randomize();					// Initialize random number generator
	nsga2b->Init("prepdata/param.in");				// This sets all variables related to GA
	nsga2b->InitMemory();							// This allocates memory for the populations
	nsga2b->InitPop(nsga2b->parent_pop, Np_start);	// Initialize parent population randomly
	
	// Vector of capacity losses for events
	double events[(SLength[0] + IdxCap.GetSize()) * (Nevents+1)];
	ReadEvents(events, "prepdata/bend_events.csv");
	
	cout << "- Initialization done, now performing first generation" << endl;
	
	// -- Start evaluating 1A -- // 
	nsga2a->decodePop(nsga2a->parent_pop);
	nsga2a->evaluatePop(nsga2a->parent_pop, events);
	nsga2a->assignRankCrowdingDistance(nsga2a->parent_pop); 
	
	nsga2a->fileio->report_pop (nsga2a->parent_pop, nsga2a->fileio->fpt1);		// Initial pop out
	fprintf(nsga2a->fileio->fpt4,"# gen = 1A\n");
	nsga2a->fileio->report_pop(nsga2a->parent_pop,nsga2a->fileio->fpt4);		// All pop out
	
	cout << "- Finished generation #1A" << endl;
	nsga2a->fileio->flushIO();
	
	// -- Start evaluating 1B -- // 
	nsga2b->decodePop(nsga2b->parent_pop);
	nsga2b->evaluatePop(nsga2b->parent_pop, events);
	nsga2b->assignRankCrowdingDistance(nsga2b->parent_pop); 
	
	nsga2a->fileio->report_pop(nsga2b->parent_pop,nsga2a->fileio->fpt1);	// Initial pop out
	fprintf(nsga2a->fileio->fpt4,"# gen = 1B\n");
	nsga2a->fileio->report_pop(nsga2b->parent_pop,nsga2a->fileio->fpt4);	// All pop out
	
	cout << "- Finished generation #1B" << endl;
	nsga2a->fileio->flushIO();
	
	for ( int i = 2; i <= nsga2a->ngen; i++ ) {
		printHeader("elapsed");
		// Next generation for A
		nsga2a->selection(nsga2a->parent_pop, nsga2a->child_pop);
		nsga2a->mutatePop(nsga2a->child_pop);
		nsga2a->decodePop(nsga2a->child_pop);
		nsga2a->evaluatePop(nsga2a->child_pop, events);
		nsga2a->merge(nsga2b->parent_pop, nsga2a->child_pop, nsga2a->mixed_pop);
		nsga2a->fillNondominatedSort(nsga2a->mixed_pop, nsga2a->parent_pop);
		
		// Comment following three lines if information for all
		// generations is not desired, it will speed up the execution 
		fprintf(nsga2a->fileio->fpt4,"# gen = %dA\n",i);
		nsga2a->fileio->report_pop(nsga2a->parent_pop,nsga2a->fileio->fpt4);
		nsga2a->fileio->flushIO();  // TODO: this flushes everything, but really we only need to flush fpt4 here.
		cout << "- Finished generation #" << i << "A" << endl;
		
		// Next generation for B
		nsga2b->selection(nsga2b->parent_pop, nsga2b->child_pop);
		nsga2b->mutatePop(nsga2b->child_pop);
		nsga2b->decodePop(nsga2b->child_pop);
		nsga2b->evaluatePop(nsga2b->child_pop, events);
		nsga2b->merge(nsga2a->parent_pop, nsga2b->child_pop, nsga2b->mixed_pop);
		nsga2b->fillNondominatedSort(nsga2b->mixed_pop, nsga2b->parent_pop);
		
		// Comment following three lines if information for all
		// generations is not desired, it will speed up the execution 
		fprintf(nsga2a->fileio->fpt4,"# gen = %dB\n",i);
		nsga2a->fileio->report_pop(nsga2b->parent_pop,nsga2a->fileio->fpt4);
		nsga2a->fileio->flushIO();  // TODO: this flushes everything, but really we only need to flush fpt4 here.
		
		cout << "- Finished generation #" << i << "B" << endl;
	}
	
	cout << endl << "- Generations finished, now reporting solutions" << endl;
	nsga2a->fileio->report_pop(nsga2b->parent_pop,nsga2a->fileio->fpt2);
	nsga2a->fileio->report_feasible(nsga2b->parent_pop,nsga2a->fileio->fpt3);
	if (nsga2a->nreal!=0) {
		fprintf(nsga2a->fileio->fpt5,"\n Number of crossover of real variable = %d",nsga2a->nrealcross);
		fprintf(nsga2a->fileio->fpt5,"\n Number of mutation of real variable = %d",nsga2a->nrealmut);
	}
	if (nsga2a->nbin!=0) {
		fprintf(nsga2a->fileio->fpt5,"\n Number of crossover of binary variable = %d",nsga2a->nbincross);
		fprintf(nsga2a->fileio->fpt5,"\n Number of mutation of binary variable = %d",nsga2a->nbinmut);
	}
	
	printHeader("completed");
	return (0);
}
