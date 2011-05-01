// --------------------------------------------------------------
//    NETSCORE Version 2
//    index.h -- Definition of index functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#ifndef _INDEX_H_
#define _INDEX_H_

// Declare class type to hold index information
class Index {
	public:
		Index();
		Index(const Index& rhs);
		~Index();
		Index& operator=(const Index& rhs);
		
		void Add(const int newpos, const int newcol, const string& newname);
		void Add(const int newpos, const int newcol, const int newyear, const string& newname);
		void Add(const int newpos, const Step& newcol, const string& newname);
		void WriteFile(const char* fileinput) const;
		
		// Variables
		int start, size;
		
		vector<int> position;
		vector<int> column;
		vector<int> year;
		vector<string> name;
};

Index ReadFile(const char* fileinput);
void ImportIndices();

#endif  // _INDEX_H_
