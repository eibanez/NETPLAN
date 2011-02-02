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

	int Size = idx.GetSize();
	if (Size > 0) {
		// Find min and max columns
		int maximum = idx.GetColumn(0), minimum = maximum;
		for (unsigned int i = 1; i < Size; ++i) {
			if (idx.GetColumn(i) > maximum ) maximum = idx.GetColumn(i);
			if (idx.GetColumn(i) < minimum ) minimum = idx.GetColumn(i);
		}
		
		Step start(SName.size(), 0), guide( SName.size() );
		bool got_start = false;
		start[0] = 1;
		for (unsigned int i=1; i < SLength.size(); ++i) {
			if (!got_start) {
				start[i] = 1;
				if ( Step2Col(start) > minimum) {
					got_start = true;
					start[i] = 0;
				}
			}
		}
		minimum = Step2Col(start);
		
		// Write header description
		myfile << header << endl;
		
		// Write line of time values
		int k = -1;
		for (unsigned int i=0; i < SLength.size(); ++i) k += start[i];
		
		guide = start;
		while ( Step2Col(start) <= maximum) {
			myfile << "," << Step2Str(start);
			start = NextStep(start);
			if ( (start > SLength) && (k < SLength.size()-1) ) {
				k++;
				guide[k] = 1;
				start = guide;
			}
		}
		
		// Write one line of information
		int prevpos = -1, prevcol = maximum;
		for (int i = 0; i < Size; ++i) {
			if (idx.GetPosition(i) != prevpos) {
				for (int j = prevcol; j < maximum; ++j) myfile << ",";
				myfile << "\n" << idx.GetName(i);
				for (int j = minimum; j < idx.GetColumn(i); ++j) myfile << ",";
				prevpos = idx.GetPosition(i);
				prevcol = idx.GetColumn(i)-1;
			}
			for (int j=prevcol; j<idx.GetColumn(i); ++j)  myfile << ",";
			myfile << values[i];
			prevcol = idx.GetColumn(i);
		}
	}
	
	myfile << endl;
	
	// Close file
	myfile.close();
}

// Write data output from an array (partially)
void WriteOutput(const char* fileinput, Index& idx, vector<string>& values, const int start, const string& header) {
	vector<string> ValuesStr(0);
	for (int i = start; i < idx.GetSize() + start; ++i)
		ValuesStr.push_back( values[i] );
	WriteOutput(fileinput, idx, ValuesStr, header);
}

// Write data output for a collection of nodes
void WriteOutput(const char* fileinput, Index& idx, vector<Node>& Nodes, const string& selector, const string& header) {
	vector<string> values(0);
	for (int i = 0; i < idx.GetSize(); ++i) {
		values.push_back( Nodes[i].Get(selector) );
	}
	WriteOutput(fileinput, idx, values, header);
}

// Write data output for a collection of arcs
void WriteOutput(const char* fileinput, Index& idx, vector<Arc>& Arcs, const string& selector, const string& header) {
	vector<string> values(0);
	for (int i = 0; i < idx.GetSize(); ++i) {
		values.push_back( Arcs[i].Get(selector) );
	}
	WriteOutput(fileinput, idx, values, header);
}
