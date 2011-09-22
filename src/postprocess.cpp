// --------------------------------------------------------------
//    NETSCORE Version 2
//    postprocess.cpp - Reading in and optimizing a problem, with minimum investment
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "netscore.h"
#include "solver.h"
#include <ilcplex/ilocplex.h>
ILOSTLBEGIN

int main () {
	printHeader("postprocessor");
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	// Set output level so that Benders steps are reported on screen
	if (outputLevel == 2) outputLevel = 1;
	
	// Import indices to export data
	ImportIndices();
	
	// Declare variables to store the optimization model
	CPLEX netplan;
	
	// Read master and subproblems
	netplan.LoadProblem();
	
	// Vector of capacity losses for events
	double events[(SLength[0] + IdxCap.size) * (Nevents+1)];
	ReadEvents(events, "prepdata/bend_events.csv");
	
	// Solve problem
	double objective[Nobj];
	netplan.SolveIndividual(objective, events, true);
	
	// Report solutions if the problem is feasible
	if (objective[0] < 1.0e29) {
		vector<string> solstring(netplan.SolutionString());
		WriteOutput("prepdata/post_emissions.csv", IdxEm, solstring, "% Emissions");
		WriteOutput("prepdata/post_node_rm.csv", IdxRm, solstring, "% Reserve margins");
		WriteOutput("prepdata/post_arc_inv.csv", IdxInv, solstring, "% Investments");
		WriteOutput("prepdata/post_arc_cap.csv", IdxCap, solstring, "% Capacity");
		WriteOutput("prepdata/post_arc_flow.csv", IdxArc, solstring, "% Arc flows");
		WriteOutput("prepdata/post_node_ud.csv", IdxUd, solstring, "% Demand not served at nodes");
		
		for (int i=0; i <= Nevents; ++i) {
			vector<string> dualstring(netplan.SolutionDualString(i));
			string file_name = "prepdata/post_nodal_dual_e" + ToString<int>(i) + ".csv";
			WriteOutput(file_name.c_str(), IdxNode, dualstring, "% Dual variable at demand nodes");
		}
	}
	
	cout << "- Values returned:" << endl;
	for (int k = 0; k < Nobj; ++k)
		cout << "\t" << objective[k] << endl;
	
	printHeader("completed");
	return 0;
}
