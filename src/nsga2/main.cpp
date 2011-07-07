// --------------------------------------------------------------
//    NETSCORE Version 2
//    Implementation NSGA-II
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

CNSGA2* nsga2 = new CNSGA2();

int main (int argc, char **argv) {
	printHeader("nsga");
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	// Read indices
	ImportIndices();
	
	// -- Initialization -- //
	nsga2->randgen->randomize();                    // Initialize random number generator
	nsga2->Init("prepdata/param.in");               // This sets all variables related to GA
	nsga2->InitMemory();                            // This allocates memory for the populations
	nsga2->InitPop(nsga2->parent_pop, Np_start);    // Initialize parent population randomly
	nsga2->fileio->recordConfiguration();           // Records all variables related to GA configuration
	if (argc > 1) {
		nsga2->ResumePop(nsga2->parent_pop, argv[1]);
		fprintf(nsga2->fileio->fpt4,"# imported values\n");
		nsga2->fileio->report_pop(nsga2->parent_pop, nsga2->fileio->fpt4);     // All pop out
	}
	
	// Vector of capacity losses for events
	double events[(SLength[0] + IdxCap.size) * (Nevents+1)];
	ReadEvents(events, "prepdata/bend_events.csv");
	
	// Declare variables to store the optimization model
	CPLEX netplan;
	
	// Read optimization problem and store it in memory
	netplan.LoadProblem();
	
	cout << "- Initialization done, now performing first generation" << endl;
	
	// -- Go -- //
	nsga2->decodePop(nsga2->parent_pop);
	nsga2->evaluatePop(nsga2->parent_pop, netplan, events);
	nsga2->assignRankCrowdingDistance(nsga2->parent_pop);
	
	nsga2->fileio->report_pop(nsga2->parent_pop, nsga2->fileio->fpt1);       // Initial pop out
	
	fprintf(nsga2->fileio->fpt4,"# gen = 1\n");
	nsga2->fileio->report_pop(nsga2->parent_pop, nsga2->fileio->fpt4);         // All pop out
	
	cout << "- Finished generation #1" << endl;
	nsga2->fileio->flushIO();

	for (int i = 2; i <= nsga2->ngen; i++) {
		printHeader("elapsed");
		nsga2->selection(nsga2->parent_pop, nsga2->child_pop);
		nsga2->mutatePop(nsga2->child_pop);
		nsga2->decodePop(nsga2->child_pop);
		nsga2->evaluatePop(nsga2->child_pop, netplan, events);
		nsga2->merge(nsga2->parent_pop, nsga2->child_pop, nsga2->mixed_pop);
		nsga2->fillNondominatedSort(nsga2->mixed_pop, nsga2->parent_pop);
		
		// Comment following three lines if information for all
		// generations is not desired, it will speed up the execution
		fprintf(nsga2->fileio->fpt4,"# gen = %d\n", i);
		nsga2->fileio->report_pop(nsga2->parent_pop,nsga2->fileio->fpt4);
		nsga2->fileio->flushIO();  // TODO: this flushes everything, but really we only need to flush fpt4 here.
		
		cout << "- Finished generation #" << i << endl;
	}
	
	cout << endl << "- Generations finished, now reporting solutions" << endl;
	nsga2->fileio->report_pop(nsga2->parent_pop, nsga2->fileio->fpt2);
	nsga2->fileio->report_feasible(nsga2->parent_pop, nsga2->fileio->fpt3);
	if (nsga2->nreal!=0) {
		fprintf(nsga2->fileio->fpt5, "\n Number of crossover of real variable = %d", nsga2->nrealcross);
		fprintf(nsga2->fileio->fpt5, "\n Number of mutation of real variable = %d", nsga2->nrealmut);
	}
	if (nsga2->nbin!=0) {
		fprintf(nsga2->fileio->fpt5, "\n Number of crossover of binary variable = %d", nsga2->nbincross);
		fprintf(nsga2->fileio->fpt5, "\n Number of mutation of binary variable = %d", nsga2->nbinmut);
	}
	
	printHeader("completed");
	return (0);
}
