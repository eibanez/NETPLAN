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
ILOSTLBEGIN

double EmissionIndex(const IloNumArray v, const int start);
double MinAverage(const IloNumArray v, Index Idx, const int start);

int main (int argc, char **argv) {
	IloEnv env;
	FILE *file;
	char line [ 200 ];
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	// Read indices
	Index IdxRm = ReadFile("prepdata/idx_rm.csv");
	Index IdxEm = ReadFile("prepdata/idx_em.csv");
	
	// Start of different variables
	const int startEm = 0;
	const int startRm = startEm + IdxEm.GetSize();
	const int startInv = startRm + IdxRm.GetSize();
	int inv = startInv;
		
	try {
		IloModel model(env);
		IloCplex cplex(env);

		cplex.setParam(IloCplex::RootAlg, IloCplex::AutoAlg);

		IloObjective   obj;
		IloNumVarArray var(env);
		IloRangeArray  rng(env);
		cplex.importModel(model, argv[1], obj, var, rng);

		// Import minimum
		file = fopen(argv[2], "r");
		if ( file != NULL ) {
			for (;;) {
				// Read a line from the file and finish if empty is read
				if ( fgets(line, sizeof line, file) == NULL ) {
					break;
				}
				
				double d1;
				d1 = strtod(line, NULL);
				
				model.add(var[inv] >= d1);
				
				++inv;
			}
		}
		
		// Solve problem
		cplex.extract(model);
		if ( !cplex.solve() ) {
			env.error() << "Failed to optimize LP" << endl;
			throw(-1);
		}
		
		// Recover variables
		IloNumArray vals(env);
		cplex.getValues(vals, var);
		
		// Find smallest average reserve margin
		double em_index = EmissionIndex(vals, startEm);
		double min_rm = MinAverage(vals, IdxRm, startRm);
		
		// Write objectives in output file
		file = fopen(argv[3], "w");
		fprintf(file, "%e\n", cplex.getObjValue());
		fprintf(file, "%e\n", em_index);
		fprintf(file, "-%e", min_rm);
		fclose(file); 
	}
	catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	}
	catch (...) {
		cerr << "Unknown exception caught" << endl;
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
