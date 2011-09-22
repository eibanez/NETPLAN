// --------------------------------------------------------------
//    NETSCORE Version 2
//    postnsga.cpp - Recovering NGSA-II solutions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "netscore.h"
#include "solver.h"

int main (int argc, char **argv) {
	printHeader("postnsga");
	
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
	
	// Read min, max, number of bits
	double min[IdxNsga.size], max[IdxNsga.size];
	int nbits[IdxNsga.size];
	FILE *file = fopen("prepdata/param.in", "r");
	if (file != NULL) {
		char line[10000];
		for (int i=0; i < 10; ++i)
			fgets(line, sizeof line, file);
		
		for (int i = 0; i < IdxNsga.size; i++) {
			// The number of bits for binary variable i
			fgets(line, sizeof line, file);
			
			char* t_read = strtok(line," ");
			nbits[i] = strtol(t_read, NULL, 10);
			// Lower limit for binary variable i
			t_read = strtok(NULL," ");
			min[i] = strtod(t_read, NULL);
			// upper limit for binary variable i
			t_read = strtok(NULL," ");
			max[i] = strtod(t_read, NULL);
		}
		fclose(file);
	}
	
	// Open best_pop.out
	char* t_read;
	char line[256];
	if (argc == 1)
		file = fopen("nsgadata/best_pop.out", "r");
	else
		file = fopen(argv[1], "r");
	
	if (file != NULL) {
		// Output file
		ofstream myfile;
		myfile.open("bestdata/NSGA_summary.csv");
		
		// Header
		myfile << "Cost";
		for (int j=0; j < SustObj.size(); ++j)
			myfile << "," << SustObj[j];
		myfile << ",Resil"; //,ConstViol,Rank,Crowd";
		for (int j = 1; j <= Nevents; ++j)
			myfile << ",Event" << j;
		for (int j = 0; j < SustMet.size(); ++j)
			myfile << ",Total" << SustMet[j];
		/*myfile << ",inv" << IdxInv.GetName(0);
		for (int j=1; j < IdxInv.GetSize(); ++j)
			if (IdxInv.GetName(j) != IdxInv.GetName(j-1))
				myfile << ",inv" << IdxInv.GetName(j);*/
		myfile << endl;
		
		// Discard first two lines (comments)
		fgets(line, sizeof line, file);
		fgets(line, sizeof line, file);
		
		int candidate = 1;
		
		for (;;) {
			// Read a line from the file and finish if empty is read
			char tmp[80];
			int output = fscanf (file, "%s", tmp);
			
			if (output == EOF) {
				if (candidate ==1)
					cout << endl << "\tERROR: No valid NSGA-II solutions found" << endl;
				break;
			}
			
			cout << "- Solution #" << candidate << endl;
			
			// Copy objectives
			//myfile << tmp;
			for (int j=1; j < Nobj; ++j) {
				fscanf (file, "%s", tmp);
				//myfile << "," << tmp;
			}
			
			// Apply values in the array as LB for investment variables
			int k = Nobj;
			double lbValue[IdxNsga.size];
			
			for (int i = 0; i < IdxNsga.size; ++i) {
				double actual = 0, maxim = 0;
				float f;
				for (int j=0; j < nbits[i]; ++j) {
					fscanf (file, "%f", &f);
					actual = 2 * actual + f;
					maxim = 2 * maxim + 1;
					++k;
				}
				lbValue[i] = min[i] + (max[i]-min[i]) * actual / maxim;
			}
			
			netplan.ApplyMinInv(lbValue);
			
			
			// Copy last 3 parameters in the line
			for (int j=0; j < 3; ++j) {
				fscanf (file, "%s", tmp);
				//myfile << "," << tmp;
			}
			
			// Solve problem
			double objective[Nobj];
			string returnSolution = "";
			netplan.SolveIndividual(objective, events, false, &returnSolution);
			
			// Write objectives
			myfile << objective[0];
			for (int j=1; j < Nobj; ++j) {
				myfile << "," << objective[j];
			}
			
			// Write returned string on file
			myfile << returnSolution << endl;
			
			// Report solutions (should be made optional)
			if (true) {
				vector<string> solstring(netplan.SolutionString());
				string base_name = "bestdata/" + ToString<int>(candidate);
				WriteOutput((base_name + "_emissions.csv").c_str(), IdxEm, solstring, "% Emissions");
				WriteOutput((base_name + "_node_rm.csv").c_str(), IdxRm, solstring, "% Reserve margins");
				WriteOutput((base_name + "_arc_inv.csv").c_str(), IdxInv, solstring, "% Investments");
				WriteOutput((base_name + "_arc_cap.csv").c_str(), IdxCap, solstring, "% Capacity");
				WriteOutput((base_name + "_arc_flow.csv").c_str(), IdxArc, solstring, "% Arc flows");
				WriteOutput((base_name + "_node_ud.csv").c_str(), IdxUd, solstring, "% Demand not served at nodes");
			}
			
			++candidate;
		}
		
		// Close files
		myfile.close();
		fclose(file);
	} else {
		cout << endl;
		cout << "\tERROR: No NSGA-II result file found!" << endl;
		cout << "\t       Unsuccessfully tried to access '";
		if (argc == 1) cout << "nsgadata/best_pop.out"; else cout << argv[1];
		cout << "'" << endl;
		cout << "\t       File name is optional if different than default" << endl;
		cout << "\t       e.g.: ./postnsga [filename]" << endl;
	}
	
	printHeader("completed");
	return 0;
}
