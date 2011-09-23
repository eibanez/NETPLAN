// --------------------------------------------------------------
//    NETSCORE Version 2
//    node.cpp -- Implementation of node functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#include "node.h"

// Contructors and destructor for the Node class
Node::Node(GlobalParam *prop) : Properties(prop->NodeDefault) {
	ShortCode = "";
	isDCflow = false;
	p = prop;
}

Node::Node(const Node& rhs) : Properties(rhs.Properties) {
	ShortCode = rhs.ShortCode;
	isDCflow = rhs.isDCflow;
	p = rhs.p;
}

Node::~Node() {}


// Read a node property in string format
string Node::Get(const Node_Prop selector) const {
	switch (selector) {
	case N_ShortCode:
		return ShortCode;
		break;
	default:
		return Properties[selector];
	}
};

// Read a property as a double
double Node::GetDouble(const Node_Prop selector) const {
	double output;
	output = (Properties[selector] != "X")
	         ? atof(Properties[selector].c_str())
	         : 0;
	return output;
}

// Modify a property
void Node::Set(const Node_Prop selector, const string& input){
	switch (selector) {
	case N_ShortCode:
		ShortCode = input;
		isDCflow = (input.substr(0,2) == DCCode) && useDCflow;
		break;
	default:
		Properties[selector] = input;
	}
};

// Multiply stored values by 'value'
void Node::Multiply(const Node_Prop selector, const double value) {
	double actual = GetDouble(selector);
	if (actual != 0)
		Set(selector, ToString<double>(actual * value));
};

// Get what time the node belongs to (i.e., year)
int Node::Time() const {
	return Str2Step(Properties[N_Step])[0];
}

// ****** MPS output functions ******
string Node::NodeNames() const {
	string temp_output = "";
	// Create constraint for ach node with a valid demand
	if ((Properties[N_Demand] != "X") && (Properties[N_Code][0] != 'X')) {
		temp_output += " E " + Properties[N_Code] + "\n";
	} else {
		temp_output += " N " + Properties[N_Code] + "\n";
	}
	return temp_output;
}

string Node::NodeUDColumns() const {
	string temp_output = "";
	// If unserved demand is allowed, write the appropriate cost
	if (Properties[N_CostUD] != "X") {
		temp_output += "    UD_" + Properties[N_Code] + " obj " + Properties[N_CostUD] + "\n";
		temp_output += "    UD_" + Properties[N_Code] + " " + Properties[N_Code] + " 1\n";
	}
	return temp_output;
}

string Node::NodePeakRows() const {
	string temp_output = "";
	// If peak demand is available, write the appropriate row
	if ((Properties[N_PeakPower] != "X") && isFirstinYear()) {
		temp_output += " E pk" + Properties[N_Code] + "\n";
	}
	return temp_output;
}

string Node::NodeRMColumns() const {
	string temp_output = "";
	// If peak demand is available, write reserve margin variable
	if ((Properties[N_PeakPower] != "X") && isFirstinYear())
		temp_output += "    RM_" + Properties[N_Code] + " pk" + Properties[N_Code] + " -" + Properties[N_PeakPower] + "\n";
	
	return temp_output;
}

string Node::NodeRMBounds() const {
	string temp_output = "";
	// If peak demand is available, write lower bound for reserve margin
	if ((Properties[N_PeakPower] != "X") && isFirstinYear())
		temp_output += " LO bnd RM_" + Properties[N_Code] + " 1\n";
	
	return temp_output;
}

string Node::NodeRhs() const {
	string temp_output = "";
	// Demand RHS if it's valid
	if ((Properties[N_Demand] != "X") && (Properties[N_Demand] != "0"))
		temp_output = " rhs " + Properties[N_Code] + " " + Properties[N_Demand] + "\n";
	
	return temp_output;
}

string Node::DCNodesBounds() const {
	string temp_output = "";
	// Write minimum and max for DC Power flow anges (-pi and pi)
	temp_output += " LO bnd th" + Properties[N_Code] + " -3.14\n";
	temp_output += " UP bnd th" + Properties[N_Code] + " 3.14\n";
	return temp_output;
}

// ****** Boolean functions ******
// Is this the first node in a year?
bool Node::isFirstinYear() const {
	bool output = true;
	Step tempstep = Str2Step(Properties[N_Step]);
	for (unsigned int k = 1; k < SName.size(); k++)
		output = output && ((tempstep[k]==0) || (tempstep[k]==1));
	
	return output;
}
