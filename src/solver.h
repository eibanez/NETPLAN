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
	IloArray<IloModel> model;
	IloArray<IloCplex> cplex;
	IloArray<IloObjective> obj;
	IloArray<IloNumVarArray> var;
	IloArray<IloRangeArray> rng;
	IloNumArray solution;
	IloArray<IloNumArray> dualsolution;
	
	// Variable to store temporary master cuts
	IloRangeArray MasterCuts;
	
	// Constraints to apply capacities to subproblems
	IloArray<IloRangeArray> CapCuts;
	
	CPLEX(): env(), model(env, 0), cplex(env, 0), obj(env, 0), var(env, 0), rng(env, 0),
		solution(env, 0), dualsolution(env, 0), MasterCuts(env, 0), CapCuts(env, 0) {};
	
	~CPLEX() {
		// Remove optimization elements from memory
		dualsolution.end(); solution.end(); rng.end(); var.end(); obj.end(); model.end(); cplex.end();
		env.end();
	};
	
	// Loads the problem from MPS files into memory
	void LoadProblem();
	
	// Solves current model
	void SolveIndividual(double *objective, const double events[], string & returnString);
	void SolveIndividual(double *objective, const double events[]);
	
	// Store complete solution vector
	void StoreSolution();
	void StoreDualSolution();
	void StoreDualSolution(int event, double *years);
	
	// Function called by the NSGA-II method. It takes the minimum investement (x) and calculates the metrics (objective)
	void SolveProblem(double *x, double *objective, const double events[]);
	
	// Apply minimum investments to the master problem
	void ApplyMinInv(double *x);
	
	// Provide solution as a string vector
	vector<string> SolutionString();
	vector<string> SolutionDualString(int event);
	
	// Apply capacities from master to subproblems
	void CapacityConstraints(const double events[], const int event, const int offset);
};

// Metrics
double EmissionIndex(const IloNumArray& v, const int start);
vector<double> SumByRow(const IloNumArray& v, Index Idx, const int start);

/*

// Resets models to improve memory management
void ResetProblem(IloArray<IloModel>& model, IloArray<IloCplex>& cplex);

// Import Minimum investment into the model from file (not tested)
void ImportMin( const char* filename, const int MstartInv );

*/

#endif  // _SOLVER_H_
