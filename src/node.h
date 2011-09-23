// --------------------------------------------------------------
//    NETSCORE Version 2
//    node.h -- Definition of node functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#ifndef _NODE_H_
#define _NODE_H_

#include "global.h"

enum Node_Prop { N_Code, N_ShortCode, N_Step, N_StepLength, N_Demand,
                 N_DemandPower, N_DemandRate, N_PeakPower, N_PeakPowerRate,
                 N_CostUD, N_Discount_Rate, N_InflationRate, N_SIZE, N_OFFSET = 4 };

// Declare class type to hold node information
struct Node {
	Node(GlobalParam *prop);
	Node(const Node& rhs);
	~Node();
	
	string Get(const Node_Prop selector) const;
	double GetDouble(const Node_Prop selector) const;
	void Set(const Node_Prop selector, const string& input);
	void Multiply(const Node_Prop selector, const double value);
	int Time() const;
	
	string NodeNames() const;
	string NodeUDColumns() const;
	string NodePeakRows() const;
	string NodeRMColumns() const;
	string NodeRMBounds() const;
	string NodeRhs() const;
	string DCNodesBounds() const;
	
	bool isFirstinYear() const;
	
	// Variables
	vector<string> Properties;
	bool isDCflow;
	GlobalParam *p;
};

#endif  // _NODE_H_
