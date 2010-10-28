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

CNSGA2*	nsga2  	= new CNSGA2();

// Indices for retreiving data
Index IdxRm, IdxEm, IdxInv;
int startEm, startRm, startInv;

// Metric function definition
double EmissionIndex(const IloNumArray v, const int start);
double MinAverage(const IloNumArray v, Index Idx, const int start);

// Declare variables to store the optimization model
IloEnv env;
IloModel model(env);
IloCplex cplex(env);
IloObjective   obj;
IloNumVarArray var(env);
IloRangeArray  rng(env);


int main (int argc, char **argv) {
	printHeader("nsga");

	FILE *file;
	char line [ 200 ];
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	// Read indices
	IdxRm = ReadFile("prepdata/idx_rm.csv");
	IdxEm = ReadFile("prepdata/idx_em.csv");
	IdxInv = ReadFile("prepdata/idx_inv.csv");
	
	// Start of different variables
	startEm = 0;
	startRm = startEm + IdxEm.GetSize();
	startInv = startRm + IdxRm.GetSize();
	
	// -- Initialization -- //
	nsga2->randgen->randomize();			// Initialize random number generator
	nsga2->Init("param.in");				// This sets all variables related to GA
	nsga2->InitMemory();					// This allocates memory for the populations
	nsga2->InitPop(nsga2->parent_pop);		// Initialize parent population randomly
	nsga2->fileio->recordConfiguration();	// Records all variables related to GA configuration
	
	// Read optimization problem and store it in memory
	try {
		cplex.setParam(IloCplex::RootAlg, IloCplex::AutoAlg);
		cplex.importModel(model, "netscore.mps", obj, var, rng);
	}
	catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	}
	catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
	
	cout << "\nInitialization done, now performing first generation\n";
	
	// -- Go -- // 
	nsga2->decodePop(nsga2->parent_pop);
	nsga2->evaluatePop(nsga2->parent_pop);
	nsga2->assignRankCrowdingDistance(nsga2->parent_pop); 

	nsga2->fileio->report_pop (nsga2->parent_pop, nsga2->fileio->fpt1);	// Initial pop out

	fprintf(nsga2->fileio->fpt4,"# gen = 1\n");
	nsga2->fileio->report_pop(nsga2->parent_pop,nsga2->fileio->fpt4);		// All pop out

	cout << endl << "gen = 1" << endl;
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
		nsga2->fileio->flushIO();  // TODO: this flushes everything, but reall we only need to flush fpt4 here.

		cout << "gen = " << i << endl;
	}

	cout << endl << "Generations finished, now reporting solutions\n";
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
	return (0);
	
	env.end();
}

void SolveProblem(double *xreal, double *objective) {
	// Function called by the NSGA-II method. It takes the minimum investement (xreal) and calculates
	// the metrics (objective)
	try {
		// Force minimum invesment (xreal) as lower bound
		for (int i = 0; i < IdxInv.GetSize(); ++i)
			var[startInv + i].setLB(xreal[i]);
		
		// Solve problem with Cplex
		cplex.extract(model);
		if ( !cplex.solve() ) {
			// If no solution is found, return very large values
			env.error() << "Failed to optimize LP" << endl;
			throw(-1);
			objective[0] = INF;
			objective[1] = INF;
			objective[2] = INF;
		} else {
			// Solution found, First recover variables
			IloNumArray vals(env);
			cplex.getValues(vals, var);
			
			// Cost metric: Objective value
			objective[0] = cplex.getObjValue();
			// Emission metric: Depends on yearly emissions
			objective[1] = EmissionIndex(vals, startEm);
			// Resiliency metric: Depend on electric reserve margins
			objective[2] = MinAverage(vals, IdxRm, startRm);
		}
	}
	catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	}
	catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

double MinAverage(const IloNumArray v, Index Idx, const int start) {
	// This function calculates the average value of reserve margin for each year and then selects the minimum
	bool first = true;
	int last_index = -1, j=0;
	double sum = 0;
	vector<double> avg(0);
	
	for (int i = 0; i < Idx.GetSize(); ++i) {
		//
		if ( (last_index != Idx.GetPosition(i)) && !first) {
			avg.push_back(sum/j);
			sum = 0; j=0;
			last_index = Idx.GetPosition(i);
		} else {
			if (first) {
				last_index = Idx.GetPosition(i);
				first = false;
				}
			sum += v[start + i];
			++j;
		}
	}
	avg.push_back(sum/j);
	
	double min = avg[0];
	for (unsigned int i = 1; i < avg.size(); ++i) {
		if (avg[i] < min) min = avg[i];
	}
	
	return min;	
}

double EmissionIndex(const IloNumArray v, const int start) {
	// This function calculates an emission index
	double em_zero = v[start], max = v[start], min = v[start], reduction = 0.02 * v[start], increase = 0.02, sum = 0;
	int first_year = 5, j = 0;
	vector<double> index(0);
	
	for (int i = 1; i < SLength[0]; ++i) {
		// max: worst case scenario emissions
		max = max * (1 + increase);
		// min: best case scenario emissions
		min -= reduction;
		if (i > first_year) {
			// Find index for the emissions at year i and carry a sum
			sum += (v[start+i]-min)/(max-min);
			++j;
		}
	}
	// Find average value for the index
	return sum/j;	
}
