// --------------------------------------------------------------
//    NETSCORE Version 1
//    postnsga.cpp - Recovering NGSA-II solutions
//    2009-2010 (c) Eduardo Ibáñez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "netscore.h"
#include <ilcplex/ilocplex.h>
ILOSTLBEGIN
#include "solver.h"


int main () {
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
	
	// Open best_pop.out
	char* t_read;
	string prop, value, discount = "0", inflation = "0", demandrate = "0", peakdemandrate = "0";
	char line [ CharLine ];
	FILE *file = fopen("best_pop.out", "r");
	if ( file != NULL ) {
		// Output file
		ofstream myfile;
		myfile.open("NSGA_summary.csv");
		
		// Header
		myfile << "Cost";
		for (int j=0; j < SustObj.size(); ++j)
			myfile << "," << SustObj[j];
		myfile << ",Resil,ConstViol,Rank,Crowd,TotalCO2";
		myfile << ",inv" << IdxInv.GetName(0);
		for (int j=1; j < IdxInv.GetSize(); ++j)
			if (IdxInv.GetName(j) != IdxInv.GetName(j-1))
				myfile << ",inv" << IdxInv.GetName(j);
		myfile << endl;
		
		// Discard first two lines (comments)
		fgets(line, sizeof line, file); fgets(line, sizeof line, file);
		
		for (;;) {
			// Read a line from the file and finish if empty is read
			if ( fgets(line, sizeof line, file) == NULL ) {
				break;
			}
			// Remove comments and end of line characters
			CleanLine(line);
			
			// Store line in a vector
			vector<string> StoredLine(0);
			t_read = strtok(line,"\t");
			while (t_read != NULL) {
				StoredLine.push_back( t_read );
				t_read = strtok(NULL, "\t");
			};
			
			// Copy objectives
			myfile << StoredLine[0];
			for (int j=1; j < Nobj; ++j)
				myfile << "," << StoredLine[j];
			for (int j=StoredLine.size()-3; j < StoredLine.size(); ++j)
				myfile << "," << StoredLine[j];
			
			// Solve problem
			bool optimal;
			double objective;
			for (int i = 0; i < IdxInv.GetSize(); ++i)
				var[0][startInv + i].setLB( atof(StoredLine[i+Nobj].c_str()) );
			OptimizeCost(optimal, objective);

			if (optimal) {
				// Solution found, First recover variables
				vector<double> vals( SolutionDouble() );
				vector<double> emissions = SumByRow(vals, IdxEm, startEm);
				myfile << "," << emissions[0];
				vector<double> Investments = SumByRow(vals, IdxInv, startInv);
				for (int j=0; j < Investments.size(); ++j)
					myfile << "," << Investments[j];
			} else {
				cout << "     Problem infeasible!";
			}
			myfile << endl;
		}
		
		// Close output file
		myfile.close();
	}
	
	fclose(file);
	env.end();
	printHeader("completed");
	return 0;
}
