// --------------------------------------------------------------
//    NETSCORE Version 1
//    node.cpp -- Implementation of node functions
//    2009-2010 (c) Eduardo Ibáñez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "node.h"

// Contructors and destructor for the Node class
Node::Node() : Properties(NodeDefault) {}

Node::Node(const Node& rhs) : Properties(rhs.GetVecStr()) {}

Node::~Node() {}

Node& Node::operator=(const Node& rhs) {
    Properties = rhs.GetVecStr();
    return *this;
}

// Read a node property in string format
string Node::Get(const string& selector) const {
    string temp_output;
	int index = FindNodeSelector(selector);
    if (index >= 0) temp_output = Properties[index];
    else {
        temp_output = "ERROR";
        printError("noderead", selector);
    };
    return temp_output;
};

// Read a property as a double
double Node::GetDouble(const string& selector) const {
	double output;
	output = (Get(selector) != "X") ? atof( Get(selector).c_str() ) : 0;
	return output;
}

// Copy the entire vector of properties
vector<string> Node::GetVecStr() const {
	return Properties;
};

// Modify a property
void Node::Set(const string& selector, const string& input){
	int index = FindNodeSelector(selector);
    if (index >= 0) Properties[index] = input;
    else printError("nodewrite", selector);
};

// Multiply stored values by 'value'
void Node::Multiply(const string& selector, const double value) {
	int index = FindNodeSelector(selector);
	if (index >= 0) {
		double actual = GetDouble(selector);
		if (actual != 0) {
			Set(selector, ToString<double>(actual * value));
		}
	} else printError("nodewrite", selector);
};

// Get what time the node belongs to
int Node::Time() const {
	return Str2Step(Get("Step"))[0];
}


// ****** MPS output functions ******
string Node::NodeNames() const {
    string temp_output = "";
    // Create constraint for ach node with a valid demand
    if ( Get("Demand") != "X" && Get("Code")[0] != 'X' ) {
        temp_output += " E " + Get("Code") + "\n";
    } else {
        temp_output += " N " + Get("Code") + "\n";
    }
    return temp_output;
}

string Node::NodeColumns() const {
    string temp_output = "";
    // If unserved demand is allowed, write the appropriate cost
    if ( Get("CostUD") != "X" ) {
        temp_output += "    UD_" + Get("Code") + " obj " + Get("CostUD") + "\n";
        temp_output += "    UD_" + Get("Code") + " " + Get("Code") + " 1\n";
    }
    return temp_output;
}

string Node::NodePeakRows() const {
    string temp_output = "";
    // If peak demand is available, write the appropriate row
    if ( Get("PeakPower") != "X" ) {
        temp_output += " E pk" + Get("Code") + "\n";
    }
    return temp_output;
}

string Node::NodeRMColumns() const {
    string temp_output = "";
    // If peak demand is available, write reserve margin variable
    if ( Get("PeakPower") != "X" ) {
        temp_output += "    RM_" + Get("Code") + " pk" + Get("Code") + " -" + Get("PeakPower") + "\n";
    }
    return temp_output;
}

string Node::NodeRMBounds() const {
    string temp_output = "";
    // If peak demand is available, write lower bound for reserve margin
    if ( Get("PeakPower") != "X" ) {
        temp_output += " LO bnd RM_" + Get("Code") + " 1\n";
    }
    return temp_output;
}

string Node::NodeRhs() const {
    string temp_output = "";
    // Demand RHS if it's valid
    if ( (Get("Demand") != "X") && (Get("Demand") != "0") ) {
        temp_output = " rhs " + Get("Code") + " " + Get("Demand") + "\n";
    }
    return temp_output;
}

string Node::DCNodesBounds() const {
    string temp_output = "";
    // Write minimum and max for DC Power flow anges (-pi and pi)
    temp_output += " LO bnd th" + Get("Code") + " -3.14\n";
    temp_output += " UP bnd th" + Get("Code") + " 3.14\n";
    return temp_output;
}

// ****** Boolean functions ******
// Is Node a DC node?
bool Node::isDCelect() const {
    return ( Get("ShortCode").substr(0,2) == DCCode );
}

// Is Node and DC node and are we considering DC flow in the model?
bool Node::isDCflow() const {
    return isDCelect() && useDCflow;
}

// ****** Other functions ******
// Find the index for a node property selector
int FindNodeSelector(const string& selector) {
	int index = -1;
	for (unsigned int k = 0; k < NodeProp.size(); ++k) {
		if (selector == NodeProp[k]) index = k;
	}
	return index;
}
