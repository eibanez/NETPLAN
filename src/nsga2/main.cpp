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
#include "ilogfunctions.h"
#include "netscore.h"
#include <ilcplex/ilocplex.h>
ILOSTLBEGIN
#include "solver.h"

CNSGA2*	nsga2 = new CNSGA2();

// Indices for retreiving data
int startEm, startRm, startInv;


int main (int argc, char **argv) {
	printHeader("nsga");

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
	if ( useBenders ) startInv += SLength[0];
	
	// -- Initialization -- //
	nsga2->randgen->randomize();			// Initialize random number generator
	nsga2->Init("param.in");				// This sets all variables related to GA
	nsga2->InitMemory();					// This allocates memory for the populations
	nsga2->InitPop(nsga2->parent_pop);		// Initialize parent population randomly
	nsga2->fileio->recordConfiguration();	// Records all variables related to GA configuration
	
	// Read optimization problem and store it in memory
	cout << "- Reading problem" << endl;
	LoadProblem();
	
	cout << "- Initialization done, now performing first generation" << endl;
	
	// -- Go -- // 
	nsga2->decodePop(nsga2->parent_pop);
	nsga2->evaluatePop(nsga2->parent_pop);
	nsga2->assignRankCrowdingDistance(nsga2->parent_pop); 

	nsga2->fileio->report_pop (nsga2->parent_pop, nsga2->fileio->fpt1);	// Initial pop out

	fprintf(nsga2->fileio->fpt4,"# gen = 1\n");
	nsga2->fileio->report_pop(nsga2->parent_pop,nsga2->fileio->fpt4);		// All pop out

	cout << "=== Generation #1 ===" << endl;
	nsga2->fileio->flushIO();

	for ( int i = 2; i <= nsga2->ngen; i++ ) {
		nsga2->selection (nsga2->parent_pop, nsga2->child_pop);
		nsga2->mutatePop (nsga2->child_pop);
		nsga2->decodePop(nsga2->child_pop);
		nsga2->evaluatePop(nsga2->child_pop);
		nsga2->merge (nsga2->parent_pop, nsga2->child_pop, nsga2->mixed_pop);
		nsga2->fillNondominatedSort(nsga2->mixed_pop, nsga2->parent_pop);

		// Comment following three lines if information for all
		// generations is not desired, it will speed up the execution 
		fprintf(nsga2->fileio->fpt4,"# gen = %d\n",i);
		nsga2->fileio->report_pop(nsga2->parent_pop,nsga2->fileio->fpt4);
		nsga2->fileio->flushIO();  // TODO: this flushes everything, but really we only need to flush fpt4 here.

		cout << "=== Generation #" << i << " ===" << endl;
	}

	cout << endl << "- Generations finished, now reporting solutions" << endl;
	nsga2->fileio->report_pop(nsga2->parent_pop,nsga2->fileio->fpt2);
	nsga2->fileio->report_feasible(nsga2->parent_pop,nsga2->fileio->fpt3);
	if (nsga2->nreal!=0) {
		fprintf(nsga2->fileio->fpt5,"\n Number of crossover of real variable = %d",nsga2->nrealcross);
		fprintf(nsga2->fileio->fpt5,"\n Number of mutation of real variable = %d",nsga2->nrealmut);
	}
	if (nsga2->nbin!=0) {
		fprintf(nsga2->fileio->fpt5,"\n Number of crossover of binary variable = %d",nsga2->nbincross);
		fprintf(nsga2->fileio->fpt5,"\n Number of mutation of binary variable = %d",nsga2->nbinmut);
	}
	
	printHeader("completed");
	env.end();
	return (0);
}

// Function called by the NSGA-II method. It takes the minimum investement (xreal) and calculates the metrics (objective)
void SolveProblem(double *xreal, double *objective) {
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
		// Resiliency metric: Depend on electric reserve margins  /////////////////////////////////////////////////////////////////////////
		objective[1] = MinAverage(vals, IdxRm, startRm);
		// Sustainability metrics
		vector<double> emissions = SumByRow(vals, IdxEm, startEm);
		for (int i=0; i <= SustObj.size(); ++i) {
			if (SustObj[i] == "EmCO2" || SustObj[i] == "CO2")
				objective[2+i] = EmissionIndex(vals, startEm + SLength[0]*i);
			else
				objective[2+i] = emissions[i];
		}
	} else {
		// If no solution is found, return very large values
		cout << "    Failed to optimize LP!" << endl;
		for (int i=0; i < Nobj; ++i )
			objective[i] = 1.0e30;
	}
	// Eliminate cuts added
	EraseCuts(TempCuts);
	TempCuts.end();
}
