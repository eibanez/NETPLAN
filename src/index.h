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
		
		int GetPosition(const int idx) const;
		int GetColumn(const int idx) const;
		int GetYear(const int idx) const;
		string GetName(const int idx) const;
		int GetSize() const;
		
		void Add(const int idx, const int col, const string& name);
		void Add(const int idx, const int col, const int year, const string& name);
		void Add(const int idx, const Step& col, const string& name);
		void WriteFile(const char* fileinput) const;
		
		// Variables
		int start, size;
		
		vector<int> Position;
		vector<int> Column;
		vector<int> Year;
		vector<string> Name;
};

Index ReadFile(const char* fileinput);
void ImportIndices();

#endif  // _INDEX_H_
