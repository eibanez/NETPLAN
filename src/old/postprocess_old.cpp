// --------------------------------------------------------------------------
// Based on Example File: examples/src/ilolpex2.cpp
// Version 10.1  
// --------------------------------------------------------------------------
//
// postprocess.cpp - Reading in and optimizing a problem, with minimum investment
//
// To run this example, command line arguments are required.
// i.e.,   post   mps_filename
// Example:
//     post  example.mps

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "netscore.h"
#include <ilcplex/ilocplex.h>
ILOSTLBEGIN

void WriteOutput(const char* fileinput, Index& idx, const IloNumArray Values, const int start, const string& header);

int main (int argc, char **argv) {
	printHeader("postprocessor");
	
	IloEnv env;
	FILE *file;
	char line [ 200 ];
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	// Import indices to export data
	Index IdxNode = ReadFile("prepdata/idx_node.csv");
	Index IdxUd = ReadFile("prepdata/idx_ud.csv");
	Index IdxRm = ReadFile("prepdata/idx_rm.csv");
	Index IdxArc = ReadFile("prepdata/idx_arc.csv");
	Index IdxInv = ReadFile("prepdata/idx_inv.csv");
	Index IdxCap = ReadFile("prepdata/idx_cap.csv");
	Index IdxEm = ReadFile("prepdata/idx_em.csv");
	vector<Node> ListNodes = ReadListNodes("data/nodes_List.csv");
	vector<Arc> ListArcs = ReadListArcs("data/arcs_List.csv");
	
	// Start of different variables
	const int startEm = 0;
	const int startRm = startEm + IdxEm.GetSize();
	const int startInv = startRm + IdxRm.GetSize();
	const int startCap = startInv + IdxInv.GetSize();
	const int startArc = startCap + IdxCap.GetSize();
	const int startUd = startArc + IdxArc.GetSize();
	int inv = startInv;
	
	try {
		IloModel model(env);
		IloCplex cplex(env);

		cplex.setParam(IloCplex::RootAlg, IloCplex::AutoAlg);

		IloObjective obj;
		IloNumVarArray var(env);
		IloRangeArray rng(env);
		if (argc < 2) {
			cplex.importModel(model, "netscore.mps", obj, var, rng);
		} else {
			cplex.importModel(model, argv[1], obj, var, rng);
		}


		// Import minimum
		file = fopen(argv[2], "r");
		if ( file != NULL ) {
			for (;;) {
				// Read a line from the file and finish if empty is read
				if ( fgets(line, sizeof line, file) == NULL ) {
					break;
				}
				
				double d1;
				d1 = strtod (line, NULL);
				
				model.add(var[inv] >= d1);
				
				++inv;
			}
		}
		  
		cplex.extract(model);
		if ( !cplex.solve() ) {
			env.error() << "Failed to optimize LP" << endl;
			throw(-1);
		}
		
		IloNumArray varsol(env);
		cplex.getValues(varsol, var);
		
		// *** Write investment information ***
		WriteOutput("prepdata/post_emissions.csv", IdxEm, varsol, startEm, "Emissions");
		WriteOutput("prepdata/post_node_rm.csv", IdxRm, varsol, startRm, "Reserve margins");
		WriteOutput("prepdata/post_arc_inv.csv", IdxInv, varsol, startInv, "Investments");
		WriteOutput("prepdata/post_arc_cap.csv", IdxCap, varsol, startCap, "Capacity");
		WriteOutput("prepdata/post_arc_flow.csv", IdxArc, varsol, startArc, "Arc flows");
		WriteOutput("prepdata/post_node_ud.csv", IdxUd, varsol, startUd, "Demand not served at nodes");
	}
	catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	}
	catch (...) {
		cerr << "Unknown exception caught" << endl;
	}

	env.end();
	return 0;
} 

// Write data output from an ILOG array
void WriteOutput(const char* fileinput, Index& idx, const IloNumArray Values, const int start, const string& header) {
	vector<string> ValuesStr(0);
	for (int i = start; i < idx.GetSize() + start; ++i) {
		ostringstream ss;
		ss << Values[i];
		ValuesStr.push_back( ss.str() );
	}
	WriteOutput(fileinput, idx, ValuesStr, header);
}
