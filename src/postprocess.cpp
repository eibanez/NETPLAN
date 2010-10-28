// --------------------------------------------------------------
//    NETSCORE Version 1
//    postprocess.cpp - Reading in and optimizing a problem, with minimum investment
//    2009-2010 (c) Eduardo Ibáñez
// --------------------------------------------------------------
//
// To run this example, command line arguments are available.
// i.e., post  (min_filename)
// Examples:
//     post
//     post min.csv

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "netscore.h"
#include <ilcplex/ilocplex.h>
ILOSTLBEGIN
#include "solver.h"


int main (int argc, char **argv) {
	printHeader("postprocessor");
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	// Set output level so that Benders steps are reported on screen
	if (outputLevel == 2 ) outputLevel = 1;
	
	// Import indices to export data
	IdxNode = ReadFile("prepdata/idx_node.csv");
	IdxUd = ReadFile("prepdata/idx_ud.csv");
	IdxRm = ReadFile("prepdata/idx_rm.csv");
	IdxArc = ReadFile("prepdata/idx_arc.csv");
	IdxInv = ReadFile("prepdata/idx_inv.csv");
	IdxCap = ReadFile("prepdata/idx_cap.csv");
	IdxEm = ReadFile("prepdata/idx_em.csv");
	IdxDc = ReadFile("prepdata/idx_dc.csv");
	
	// Start of different variables for single MPS file
	int startEm = 0;
	int startRm = startEm + IdxEm.GetSize();
	int startInv = startRm + IdxRm.GetSize();
	int startCap = startInv + IdxInv.GetSize();
	int startArc = startCap + IdxCap.GetSize();
	int startUd = startArc + IdxArc.GetSize();
	int startDc = startUd + IdxUd.GetSize();
	
	// Read master and subproblems
	cout << "- Reading problem..." << endl;
	LoadProblem();
	
	// Import minimum (not tested)
	// ImportMin( argv[1], model, var )
	
	// Solve problem
	bool optimal, ResilOptimal;
	double objective, ResilObj[Nevents+1];
	IloRangeArray TempCuts(env, 0);
	OptimizeCost(optimal, objective, TempCuts);

	if (optimal) {
		// Write investment information
		cout << "- Reporting solution..." << endl;
		cout << "      Cost: " << objective << endl;
		vector<string> varsol( SolutionString() );
		
		WriteOutput("prepdata/post_emissions.csv", IdxEm, varsol, startEm, "% Emissions");
		WriteOutput("prepdata/post_node_rm.csv", IdxRm, varsol, startRm, "% Reserve margins");
		WriteOutput("prepdata/post_arc_inv.csv", IdxInv, varsol, startInv, "% Investments");
		WriteOutput("prepdata/post_arc_cap.csv", IdxCap, varsol, startCap, "% Capacity");
		WriteOutput("prepdata/post_arc_flow.csv", IdxArc, varsol, startArc, "% Arc flows");
		WriteOutput("prepdata/post_node_ud.csv", IdxUd, varsol, startUd, "% Demand not served at nodes");
		
		// Solve resiliency events
		TestResiliency(ResilOptimal, ResilObj);
		cout << "      Resil: ";
		for (int event=0; event <= Nevents; ++event) { cout << ResilObj[event] << "  "; }
		cout << endl;
		
		// Erase cuts created with Benders
		EraseCuts(TempCuts);
		TempCuts.end();
		
	} else {
		cout << "     Problem infeasible!" << endl;
	}
	
	env.end();
	printHeader("completed");
	return 0;
}
