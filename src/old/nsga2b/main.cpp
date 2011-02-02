// --------------------------------------------------------------
//    NETSCORE Version 1
//    Implementation NSGA-II
//    2009-2010 (c) Eduardo Ibáñez and others
//    For more info:
//      http://natek85.blogspot.com/2009/07/c-nsga2-code.html
//      http://www.iitk.ac.in/kangal/codes.shtml
// --------------------------------------------------------------

#include "CNSGA2.h"
#include <fstream>
#include <string>
#include <vector>
// #include "ilogfunctions.h"
// #include "netscore.h"
// #include <ilcplex/ilocplex.h>
// ILOSTLBEGIN
// #include "solver.h"

CNSGA2*	nsga2a = new CNSGA2(true);
CNSGA2*	nsga2b = new CNSGA2(false);

// Indices for retreiving data
// int startEm, startRm, startInv;


int main () {
	/*printHeader("nsga");

	FILE *file;
	char line [ 200 ];
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	// Read indices
	IdxNode = ReadFile("prepdata/idx_node.csv");
	IdxUd = ReadFile("prepdata/idx_ud.csv");
	IdxRm = ReadFile("prepdata/idx_rm.csv");
	IdxArc = ReadFile("prepdata/idx_arc.csv");
	IdxInv = ReadFile("prepdata/idx_inv.csv");
	IdxCap = ReadFile("prepdata/idx_cap.csv");
	IdxEm = ReadFile("prepdata/idx_em.csv");
	IdxDc = ReadFile("prepdata/idx_dc.csv"); 
	
	// Start of different variables
	startEm = 0;
	startRm = startEm + IdxEm.GetSize();
	startInv = startRm + IdxRm.GetSize();
	if ( useBenders ) startInv += SLength[0];*/
	
	// -- Initialization of A -- //
	nsga2a->randgen->randomize();			// Initialize random number generator
	nsga2a->Init("param.in");				// This sets all variables related to GA
	nsga2a->InitMemory();					// This allocates memory for the populations
	nsga2a->InitPop(nsga2a->parent_pop);	// Initialize parent population randomly
	nsga2a->fileio->recordConfiguration();	// Records all variables related to GA configuration
	
	/*// Read optimization problem and store it in memory
	cout << "- Reading problem" << endl;
	LoadProblem();*/
	
	cout << "- Initialization of a done, now performing first generation" << endl;
	
	// -- Start Evaluating 1A -- // 
	nsga2a->decodePop(nsga2a->parent_pop);
	nsga2a->evaluatePop(nsga2a->parent_pop);		// This is to be performed in parallel
	nsga2a->assignRankCrowdingDistance(nsga2a->parent_pop);
	
	nsga2a->fileio->report_pop(nsga2a->parent_pop,nsga2a->fileio->fpt1);	// Initial pop out
	fprintf(nsga2a->fileio->fpt4,"# gen = 1A\n");
	nsga2a->fileio->report_pop(nsga2a->parent_pop,nsga2a->fileio->fpt4);	// All pop out
	
	// -- Initialization of B -- //
	nsga2b->randgen->randomize();			// Initialize random number generator
	nsga2b->Init("param.in");				// This sets all variables related to GA
	nsga2b->InitMemory();					// This allocates memory for the populations
	nsga2b->InitPop(nsga2b->parent_pop);	// Initialize parent population randomly
	
	// -- Start Evaluating 1B -- // 
	nsga2b->decodePop(nsga2b->parent_pop);
	nsga2b->evaluatePop(nsga2b->parent_pop);		// This is to be performed in parallel
	nsga2b->assignRankCrowdingDistance(nsga2b->parent_pop);
	
	nsga2a->fileio->report_pop(nsga2b->parent_pop,nsga2a->fileio->fpt1);	// Initial pop out
	fprintf(nsga2a->fileio->fpt4,"# gen = 1B\n");
	nsga2a->fileio->report_pop(nsga2b->parent_pop,nsga2a->fileio->fpt4);	// All pop out

	cout << "=== Generation #1 ===" << endl;
	nsga2a->fileio->flushIO();
	
	for ( int i = 2; i <= nsga2a->ngen; i++ ) {
		// Next generation for A
		nsga2a->selection(nsga2a->parent_pop, nsga2a->child_pop);
		nsga2a->mutatePop(nsga2a->child_pop);
		nsga2a->decodePop(nsga2a->child_pop);
		nsga2a->evaluatePop(nsga2a->child_pop);		// This is to be performed in parallel
		if (i==2) {
			nsga2a->merge(nsga2a->parent_pop, nsga2a->child_pop, nsga2a->mixed_pop);
		} else {
			nsga2a->merge(nsga2b->parent_pop, nsga2a->child_pop, nsga2a->mixed_pop);
		}
		nsga2a->fillNondominatedSort(nsga2a->mixed_pop, nsga2a->parent_pop);

		// Comment following three lines if information for all
		// generations is not desired, it will speed up the execution 
		fprintf(nsga2a->fileio->fpt4,"# gen = %dA\n",i);
		nsga2a->fileio->report_pop(nsga2a->parent_pop,nsga2a->fileio->fpt4);
		nsga2a->fileio->flushIO();  // TODO: this flushes everything, but really we only need to flush fpt4 here.
		
		// Next generation for B
		nsga2b->selection(nsga2b->parent_pop, nsga2b->child_pop);
		nsga2b->mutatePop(nsga2b->child_pop);
		nsga2b->decodePop(nsga2b->child_pop);
		nsga2b->evaluatePop(nsga2b->child_pop);		// This is to be performed in parallel
		if (i==2) {
			nsga2b->merge(nsga2b->parent_pop, nsga2b->child_pop, nsga2b->mixed_pop);
		} else {
			nsga2b->merge(nsga2a->parent_pop, nsga2b->child_pop, nsga2b->mixed_pop);
		}
		nsga2b->fillNondominatedSort(nsga2b->mixed_pop, nsga2b->parent_pop);

		// Comment following three lines if information for all
		// generations is not desired, it will speed up the execution 
		fprintf(nsga2a->fileio->fpt4,"# gen = %dB\n",i);
		nsga2a->fileio->report_pop(nsga2b->parent_pop,nsga2a->fileio->fpt4);
		nsga2a->fileio->flushIO();  // TODO: this flushes everything, but really we only need to flush fpt4 here.
		
		cout << "=== Generation #" << i << " ===" << endl;
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
	return (0);
	
//	env.end();
}

/*void SolveProblem(double *xreal, double *objective) {
	// Function called by the NSGA-II method. It takes the minimum investement (xreal) and calculates the metrics (objective)
	
	// Force minimum investment (xreal) as lower bound
	for (int i = 0; i < IdxInv.GetSize(); ++i)
		var[0][startInv + i].setLB(xreal[i]);
	
	// Solve problem with Cplex
	bool optimal;
	double totalcost;	
	IloRangeArray TempCuts(env, 0);
	OptimizeCost(optimal, totalcost, TempCuts);
	if ( optimal ) {
		// Solution found, First recover variables
		vector<double> vals( SolutionDouble() );
		// Cost metric: Objective value
		objective[0] = totalcost;
		// Emission metric: Depends on yearly emissions
		objective[1] = EmissionIndex(vals, startEm);
		// Resiliency metric: Depend on electric reserve margins
		objective[2] = MinAverage(vals, IdxRm, startRm);
	} else {
		// If no solution is found, return very large values
		cout << "    Failed to optimize LP!" << endl;
		objective[0] = INF;
		objective[1] = INF;
		objective[2] = INF;
	}
	// Eliminate cuts added
	EraseCuts(TempCuts);
	TempCuts.end();
}*/
