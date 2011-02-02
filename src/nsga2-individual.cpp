// --------------------------------------------------------------
//    NETSCORE Version 2
//    nsga2-individual.cpp - Evaluation of an individual
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "netscore.h"
#include "solver.h"

int main () {
	printHeader("postprocessor");
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	// Import indices to export data
	ImportIndices();
	
	// Declare variables to store the optimization model
	CPLEX netplan;
	
	// Read master and subproblems
	netplan.LoadProblem();
	
	// Vector of capacity losses for events
	double events[(SLength[0] + IdxCap.GetSize()) * (Nevents+1)];
	ReadEvents(events, "prepdata/bend_events.csv");
	
	// JINXU: BEGIN LOOP WHILE THERE ARE INSTANCES TO BE SOLVED
		
		// JINXU: GET variables FROM CENTRAL NODE
		
		// Solve problem
		double variables[IdxInv.GetSize()];
		double objective[Nobj];
		netplan.SolveProblem(variables, objective, events);
		
		// JINXU: TRANSFER  objective TO CENTRAL NODE
		
	// JINXU: CLOSE LOOP
	
	return 0;
}
