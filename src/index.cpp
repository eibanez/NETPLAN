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
	Position(0), Column(0), Year(0), Name(0) { start=0; size=0; }

Index::Index(const Index& rhs) :
	Position(rhs.Position),
	Column(rhs.Column),
	Year(rhs.Year),
	Name(rhs.Name) { start=rhs.start; size=rhs.size; }

Index::~Index() {}

Index& Index::operator=(const Index& rhs) {
	Position = rhs.Position;
	Column = rhs.Column,
	Year = rhs.Year,
	Name = rhs.Name;
	start = rhs.start;
	size = rhs.size;
	return *this;
}

// Recover information
int Index::GetPosition(const int idx) const { return Position[idx]; }
int Index::GetColumn(const int idx) const { return Column[idx]; }
int Index::GetYear(const int idx) const { return Year[idx]; }
string Index::GetName(const int idx) const { return Name[idx]; }
int Index::GetSize() const { return size; }

// Add one more element to the index collection
void Index::Add(const int idx, const int col, const string& name) {
	Position.push_back(idx);
	Column.push_back(col);
	Year.push_back(col);
	Name.push_back(name);
	++size;
}
void Index::Add(const int idx, const int col, const int year, const string& name) {
	Position.push_back(idx);
	Column.push_back(col);
	Year.push_back(year);
	Name.push_back(name);
	++size;
}
void Index::Add(const int idx, const Step& col, const string& name) {
	Add(idx, Step2Col(col), col[0], name);
}

// Write a file 
void Index::WriteFile(const char* fileinput) const {
	ofstream myfile;
	myfile.open(fileinput);
	
	for (unsigned int i = 0; i < Position.size(); ++i) {
		myfile << Position[i];
		myfile << "\n";
		myfile << Column[i];
		myfile << "\n";
		myfile << Year[i];
		myfile << "\n";
		myfile << Name[i];
		myfile << "\n";
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
