// --------------------------------------------------------------
//    NETSCORE Version 2
//    node.h -- Definition of node functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#ifndef _NODE_H_
#define _NODE_H_

// Declare class type to hold node information
struct Node {
	Node();
	Node(const Node& rhs);
	~Node();
	Node& operator=(const Node& rhs);
	
	string Get(const string& selector) const;
	double GetDouble(const string& selector) const;
	void Set(const string& selector, const string& input);
	void Multiply(const string& selector, const double value);
	int Time() const;
	
	string NodeNames() const;
	string NodeUDColumns() const;
	string NodePeakRows() const;
	string NodeRMColumns() const;
	string NodeRMBounds() const;
	string NodeRhs() const;
	string DCNodesBounds() const;
	
	bool isDCflow() const;
	bool isFirstinYear() const;
	
	// Variables
	vector<string> Properties;
};

// Find the index for a node property selector
int FindNodeSelector(const string& selector);

#endif  // _NODE_H_
