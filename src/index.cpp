// --------------------------------------------------------------
//    NETSCORE Version 2
//    index.cpp -- Implementation of index functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "index.h"

// Contructors and destructor for the Index class
Index::Index() :
	position(0), column(0), year(0), name(0) { start=0; size=0; }

Index::Index(const Index& rhs) :
	position(rhs.position),
	column(rhs.column),
	year(rhs.year),
	name(rhs.name) { start=rhs.start; size=rhs.size; }

Index::~Index() {}

Index& Index::operator=(const Index& rhs) {
	position = rhs.position;
	column = rhs.column,
	year = rhs.year,
	name = rhs.name;
	start = rhs.start;
	size = rhs.size;
	return *this;
}

// Add one more element to the index collection
void Index::Add(const int newpos, const int newcol, const string& newname) {
	position.push_back(newpos);
	column.push_back(newcol);
	year.push_back(newcol);
	name.push_back(newname);
	++size;
}
void Index::Add(const int newpos, const int newcol, const int newyear, const string& newname) {
	position.push_back(newpos);
	column.push_back(newcol);
	year.push_back(newyear);
	name.push_back(newname);
	++size;
}
void Index::Add(const int newpos, const Step& newcol, const string& newname) {
	Add(newpos, Step2Col(newcol), newcol[0], newname);
}

// Write a file 
void Index::WriteFile(const char* fileinput) const {
	ofstream myfile;
	myfile.open(fileinput);
	for (int i = 0; i < size; ++i) {
		myfile << position[i] << "\n";
		myfile << column[i] << "\n";
		myfile << year[i] << "\n";
		myfile << name[i] << "\n";
	}
	myfile.close();
}

// Read a file
Index ReadFile(const char* fileinput) {
	Index TempIndex;
	char line [ 200 ];
	FILE *file = fopen(fileinput, "r");
	
	if ( file != NULL ) {
		int idx, col, year;
		string name;
		for (;;) {
			// Read a line from the file and finish if empty is read
			if ( fgets(line, sizeof line, file) == NULL ) break;
			idx = strtol(line, NULL, 10);
			if ( fgets(line, sizeof line, file) == NULL ) break;
			col = strtol(line, NULL, 10);
			if ( fgets(line, sizeof line, file) == NULL ) break;
			year = strtol(line, NULL, 10);
			if ( fgets(line, sizeof line, file) == NULL ) break;
			CleanLine(line);
			name = string(line);
			
			TempIndex.Add(idx, col, year, name);
		}
	} else {
		printError("error", fileinput);
	}
	return TempIndex;
}

// Loads all necessary index files in memory
void ImportIndices() {
	IdxNode = ReadFile("prepdata/idx_node.csv");
	IdxUd   = ReadFile("prepdata/idx_ud.csv");
	IdxRm   = ReadFile("prepdata/idx_rm.csv");
	IdxArc  = ReadFile("prepdata/idx_arc.csv");
	IdxInv  = ReadFile("prepdata/idx_inv.csv");
	IdxNsga = ReadFile("prepdata/idx_nsga.csv");
	IdxCap  = ReadFile("prepdata/idx_cap.csv");
	IdxUb   = ReadFile("prepdata/idx_ub.csv");
	IdxEm   = ReadFile("prepdata/idx_em.csv");
	IdxDc   = ReadFile("prepdata/idx_dc.csv");
	
	IdxCap.start = 0;
	IdxInv.start = IdxCap.start + IdxCap.size;
	IdxEm.start  = IdxInv.start + IdxInv.size;
	IdxRm.start  = IdxEm.start + IdxEm.size;
	IdxArc.start = IdxRm.start + IdxRm.size;
	IdxUd.start  = IdxArc.start + IdxArc.size;
	IdxDc.start  = IdxUd.start + IdxUd.size;
}
