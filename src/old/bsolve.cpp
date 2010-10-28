// --------------------------------------------------------------------------
// Based on Example File: examples/src/ilolpex2.cpp
// Version 10.1  
// --------------------------------------------------------------------------
//
// solve.cpp - Reading in and optimizing a problem, with minimum investment
//
// To run this example, command line arguments are required.
// i.e.,   solve   mps_filename  min_filename  objective_filename
// Example:
//     solve  example.mps  min.dat  obj.dat


using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "netscore.h"
#include <ilcplex/ilocplex.h>
#include "solver.h"
ILOSTLBEGIN

double EmissionIndex(const IloNumArray v, const int start);
double MinAverage(const IloNumArray v, Index Idx, const int start);

int main (int argc, char **argv) {
	printHeader("benders");
	
	IloEnv env;
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	// Read indices
	Index IdxEm = ReadFile("prepdata/idx_em.csv");
	Index IdxRm = ReadFile("prepdata/idx_rm.csv");
	Index IdxInv = ReadFile("prepdata/idx_inv.csv");
	Index IdxCap = ReadFile("prepdata/idx_cap.csv");
	
	// Number of years = how many subproblems
	int nyears = SLength[0];
	int ncap = IdxCap.GetSize()/nyears;
	
	// Start of different variables for master problem
	const int MstartCost = 0;
	const int MstartEm = MstartCost + nyears;
	const int MstartRm = MstartEm + IdxEm.GetSize();
	const int MstartInv = MstartRm + IdxRm.GetSize();
	const int MstartCap = MstartInv + IdxInv.GetSize();
	
	// Start of different variables for subproblems
	const int SstartEm = 0;
	const int SstartUb = SstartEm + 1;
	const int SstartNode = SstartUb + ncap;
	
	// Read vector of step lengths for capacitated arcs
	vector<double> steplength = ReadBendersStepLength("prepdata/bend_steplengths.csv");
	
	
	// Read master and subproblems
	cout << "- Reading problem" << endl;
	IloArray<IloModel> model(env, nyears+1);
	IloArray<IloCplex> cplex(env, nyears+1);
	IloArray<IloObjective> obj(env, nyears+1);
	IloArray<IloNumVarArray> var(env, nyears+1);
	IloArray<IloRangeArray> rng(env, nyears+1);
	LoadProblem(env, model, cplex, obj, var, rng, "prepdata/bend_");
	
	
	// Import minimum
	// ImportMin( argv[2], model, var )
	
	bool optimal;
	double objective;
	IloNumArray vals;
	SolveProblem( env, model, cplex, obj, var, rng, optimal, objective, vals, IdxCap, steplength, MstartCap);

	if ( optimal ) {
		cout << "Master obj: " << objective << endl;
	} else {
		cout << "Problem infeasible" << endl;
	}

	env.end();
	return 0;
}  // END main


double MinAverage(const IloNumArray v, Index Idx, const int start) {
	bool first = true;
	int last_index = -1, j=0;
	double sum = 0;
	vector<double> avg(0);
	
	for (int i = 0; i < Idx.GetSize(); ++i) {
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
	double em_zero = v[start], max = v[start], min = v[start], reduction = 0.02 * v[start], increase = 0.02, avg = 0;
	int first_year = 5, j = 0;
	vector<double> index(0);
	
	for (int i = 1; i < SLength[0]; ++i) {
		max = max * (1 + increase);
		min -= reduction;
		if (i > first_year) {
			avg += (v[start+i]-min)/(max-min);
			++j;
		}
	}
	
	return avg/j;	
}
