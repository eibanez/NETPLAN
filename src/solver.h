// --------------------------------------------------------------
//    NETSCORE Version 1
//    solver.cpp -- Implementation of solver functions
//    2009-2010 (c) Eduardo Ibáñez
// --------------------------------------------------------------

#ifndef _SOLVER_H_
#define _SOLVER_H_

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "metrics.h"
#include <ilcplex/ilocplex.h>

// Declare variables to store the optimization model
extern IloEnv env;
extern IloArray<IloModel> model;
extern IloArray<IloCplex> cplex;
extern IloArray<IloObjective> obj;
extern IloArray<IloNumVarArray> var;
extern IloArray<IloRangeArray> rng;

// Vector of step lengths for capacitated arcs
extern vector< vector<double> > steplength;

// Loads the problem from MPS files into memory
void LoadProblem();

// Solves current model
void OptimizeCost( bool& optimal, double& objective);
void OptimizeCost( bool& optimal, double& objective, IloRangeArray& TempCuts );

// Recover complete solution vector
vector<double> SolutionDouble();
vector<string> SolutionString();

// Apply capacities from master to subproblems
void ApplyCapacities(int event);

// Eliminate cuts created during Benders
void EraseCuts( IloRangeArray& TempCuts );

// Evaluate resiliency
void TestResiliency(bool& optimal, double objectives[]);

// Import Minimum investment into the model from file (not tested)
void ImportMin( const char* filename, const int MstartInv );

#endif  // _SOLVER_H_
