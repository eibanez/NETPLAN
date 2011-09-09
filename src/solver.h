// --------------------------------------------------------------
//    NETSCORE Version 2
//    solver.cpp -- Implementation of solver functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#ifndef _SOLVER_H_
#define _SOLVER_H_

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include <ilcplex/ilocplex.h>

// Declares a structure to store and manipulate problem information
struct CPLEX {
	IloEnv env;
	IloModel *model, *modelB;
	IloCplex *cplex, *cplexB;
	IloObjective *obj, *objB;
	IloNumVarArray *var, *varB;
	IloRangeArray *rng, *rngB;
	IloNumArray *solution, *dualsolution, *TempArray;
	
	// Constructor and destructor
	CPLEX();
	~CPLEX();
	
	// Loads the problem from MPS files into memory
	void LoadProblem();
	
	// Solves current model
	void SolveIndividual(double *objective, const double events[], string & returnString);
	void SolveIndividual(double *objective, const double events[]);
	
	// Store complete solution vector
	void StoreSolution();
	void StoreDualSolution(int event=0);
	
	// Function called by the NSGA-II method. It takes the minimum investement (x) and calculates the metrics (objective)
	void SolveProblem(double *x, double *objective, const double events[]);
	
	// Apply minimum investments to the master problem
	void ApplyMinInv(double *x);
	
	// Provide solution as a string vector
	vector<string> SolutionString();
	vector<string> SolutionDualString(int event);
	
	// Apply capacities from master to subproblems
	void CapacityConstraints(const double events[], const int event = 0);
};

// Metrics
double EmissionIndex(const IloNumArray v, const int start);
vector<double> SumByRow(const IloNumArray v, Index Idx);

#endif  // _SOLVER_H_
