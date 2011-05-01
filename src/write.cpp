// --------------------------------------------------------------
//    NETSCORE Version 2
//    write.cpp -- Implementation of file writing functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "node.h"
#include "arc.h"
#include "index.h"

// Write data from an array of values
void WriteOutput(const char* fileinput, Index& idx, vector<string>& values, const string& header) {
	// Open file
	ofstream myfile;
	myfile.open(fileinput);
	
	if (idx.size > 0) {
		// Get the start and the end that is going to be written
		int begin = (idx.size == values.size()) ? 0 : idx.start;
		
		// Find min and max columns
		int maximum = idx.column[0], minimum = maximum;
		
		for (unsigned int i = 0 + 1; i < idx.size; ++i) {
			if (idx.column[i] > maximum) maximum = idx.column[i];
			if (idx.column[i] < minimum) minimum = idx.column[i];
		}
		
		Step start(SName.size(), 0), guide(SName.size());
		bool got_start = false;
		start[0] = 1;
		for (unsigned int i=1; (i < SLength.size()) && !got_start; ++i) {
			start[i] = 1;
			if (Step2Col(start) > minimum) {
				got_start = true;
				start[i] = 0;
			}
		}
		minimum = Step2Col(start);
		
		// Write header description
		myfile << header << endl;
		
		// Write line of time values
		int k = -1;
		for (unsigned int i=0; i < SLength.size(); ++i)
			k += start[i];
		
		guide = start;
		while (Step2Col(start) <= maximum) {
			myfile << "," << Step2Str(start);
			start = NextStep(start);
			if ((start > SLength) && (k < SLength.size()-1)) {
				k++;
				guide[k] = 1;
				start = guide;
			}
		}
		
		// Write one line of information
		int prevpos = -1, prevcol = maximum;
		for (int i = 0; i < idx.size; ++i) {
			// This produces a new line if necessary
			if (idx.position[i] != prevpos) {
				for (int j = prevcol; j < maximum; ++j) myfile << ",";
				myfile << "\n" << idx.name[i];
				for (int j = minimum; j < idx.column[i]; ++j) myfile << ",";
				prevpos = idx.position[i];
				prevcol = idx.column[i] - 1;
			}
			for (int j=prevcol; j < idx.column[i]; ++j)  myfile << ",";
			myfile << values[begin + i];
			prevcol = idx.column[i];
		}
	}
	
	myfile << endl;
	
	// Close file
	myfile.close();
}

// Write data output for a collection of nodes
void WriteOutput(const char* fileinput, Index& idx, vector<Node>& Nodes, const string& selector, const string& header) {
	vector<string> values(0);
	for (int i = 0; i < idx.size; ++i)
		values.push_back(Nodes[i].Get(selector));
		
	WriteOutput(fileinput, idx, values, header);
}

// Write data output for a collection of arcs
void WriteOutput(const char* fileinput, Index& idx, vector<Arc>& Arcs, const string& selector, const string& header) {
	vector<string> values(0);
	for (int i = 0; i < idx.size; ++i)
		values.push_back(Arcs[i].Get(selector));
	
	WriteOutput(fileinput, idx, values, header);
}
