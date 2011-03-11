// --------------------------------------------------------------
//    NETSCORE Version 2
//    arc.cpp -- Implementation of arc functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "arc.h"

// Contructors and destructor for the Arc class
Arc::Arc() :
	Properties(ArcDefault),
	Energy2Trans(false),
	Trans2Energy(0) {}

Arc::Arc(const Arc& rhs) :
	Properties(rhs.GetVecStr("Properties")),
	Energy2Trans(rhs.GetBool("Energy2Trans")),
	Trans2Energy(rhs.GetVecStr("Trans2Energy")) {}

// This constructor creates an arc going in the opposite direction
Arc::Arc(const Arc& rhs, const bool reverse) :
	Properties(rhs.GetVecStr("Properties")),
	Energy2Trans(rhs.GetBool("Energy2Trans")),
	Trans2Energy(rhs.GetVecStr("Trans2Energy")) {
		if (reverse) {
			if ( !isTransport() ) {
				string temp = Get("To");
				Set("To", Get("From"));
				Set("From", temp);
			} else {
				string temp = Get("From");
				Set( "From", temp.substr(0,2) + temp.substr(4,2) + temp.substr(2,2) );
				temp = Get("To");
				Set( "To", temp.substr(0,2) + temp.substr(4,2) + temp.substr(2,2) );
			}
		}
	}

Arc::~Arc() {}

Arc& Arc::operator=(const Arc& rhs) {
    Properties = rhs.GetVecStr("Properties");
    Energy2Trans = rhs.GetBool("Energy2Trans");
    Trans2Energy = rhs.GetVecStr("Trans2Energy");
    return *this;
}

// Read a property in string format
string Arc::Get(const string& selector) const {
	string temp_output;
	int index = FindArcSelector(selector);
	if (index >= 0) temp_output = Properties[index];
	else {
		temp_output = "ERROR";
		printError("arcread", selector);
	}
	return temp_output;
}

// Read the year from the step
string Arc::GetYear() const {
	int temp = Str2Step(Get("FromStep"))[0];
	return SName.substr(0,1) + ToString<int>(temp);
}

// Read a property and convert to double
double Arc::GetDouble(const string& selector) const {
	double output;
	output = (Get(selector) != "X") ? atof( Get(selector).c_str() ) : 0;
	return output;
}

// Read a boolean property
bool Arc::GetBool(const string& selector) const {
	bool temp_output;
	if (selector == "Energy2Trans") temp_output = Energy2Trans;
	else printError("arcread", selector);
	
	return temp_output;
};

// Read an entire vector of string properties
vector<string> Arc::GetVecStr(const string& selector) const {
	vector<string> temp_output;
	if (selector == "Properties") temp_output = Properties;
	else if (selector == "Trans2Energy") temp_output = Trans2Energy;
	else printError("arcread", selector);
	return temp_output;
};

// Modify a propery
void Arc::Set(const string& selector, const string& input){
	int index = FindArcSelector(selector);	
	if (index >= 0) Properties[index] = input;
	else printError("arcwrite", selector);
};

// Modify a boolean property
void Arc::Set(const string& selector, const bool input){
	if (selector == "Energy2Trans") Energy2Trans = input;
	else printError("arcwrite", selector);
};

// Add a new line to the Trans2Energy vector, where the consumption of energy
// for a transportation link is stored
void Arc::Add(const string& selector, const string& input) {
	if (selector == "Trans2Energy") Trans2Energy.push_back(input);
	else printError("arcwrite", selector);
};

// Multiply a value or a vector by a given value
void Arc::Multiply(const string& selector, const double value) {
	int index = FindArcSelector(selector);
	if (selector == "Trans2Energy") {
		// Adjust values
		for (unsigned int i = 1; i < Trans2Energy.size(); i += 2) {
			Trans2Energy[i] = ToString<double>( value * atof(Trans2Energy[i].c_str()) );
		}
	} else if (index >= 0) {
		double actual = GetDouble(selector);
		if (actual != 0) {
			Set(selector, ToString<double>(actual * value));
		}
	} else printError("arcwrite", selector);
};


// ****** MPS output functions ******
string Arc::ArcUbNames() const {
	string temp_output = "";
	// Create upper bound constraint
	if ( isTransport()  && Get("TransInfr") == "" ) {
		// Transportation arc
		if ( Get("OpMax") != "Inf" ) {
			temp_output += " L ub" + Get("Code") + "\n";
		} else {
			temp_output += " N ub" + Get("Code") + "\n";
		}
	} else if ( !isTransport() ) {
		// Energy arc
		if ( Get("OpMax") != "Inf" ) {
			temp_output += " L ub" + Get("Code") + "\n";
		}
	}
	return temp_output;
}

string Arc::ArcCapNames() const {
	string temp_output = "";
	// Create capacity-investment constraint for arcs with valid investment
	if ( isFirstinYear() && isTransport()  && Get("TransInfr") == "" ) {
		// Transportation arc
		if ( Get("OpMax") != "Inf" ) {
			temp_output += " E inv2cap" + Get("Code") + "\n";
		}
	} else if ( isFirstinYear() && !isTransport() ) {
		// Energy arc
		if ( Get("OpMax") != "Inf" ) {
			temp_output += " E inv2cap" + Get("Code") + "\n";
		}
	}
	return temp_output;
}

string Arc::ArcDcNames() const {
	string temp_output = "";
	// Create a constraint for DC power flow branches
	if ( isDCflow() && (Get("From") < Get("To")) ) {
		temp_output += " E dcpf" + Get("Code") + "\n";
	}
	return temp_output;
}

string Arc::ArcColumns() const {
	string temp_output = "";
	string temp_code;
	if ( !isTransport() || Get("TransInfr") != "" ) {
		// Cost objective function
		if ( Get("OpCost") != "0" ) {
			temp_output += "    " + Get("Code") + " obj " + Get("OpCost") + "\n";
		}
		// Sustainability metrics
		for (int j = 0; j < SustMet.size(); ++j)		
			if ( Get("Op" + SustMet[j]) != "0" )
				temp_output += "    " + Get("Code") + " " + SustMet[j] + GetYear() + " " + Get("Op" + SustMet[j]) + "\n";
	}
	
	if ( !isTransport() ) {
		// Put arc in the constraint of the origin node
		if ( Get("From")[0] != 'X' ) {
			temp_output += "    " + Get("Code") + " " + Get("From") + Get("FromStep") + " -1\n";
		}
		// Put arc in the constraint of the destination node
		if ( Get("To")[0] != 'X' ) {
			if ( InvertEff() ) {
				temp_output += "    " + Get("Code") + " " + Get("To") + Get("ToStep") + " 1\n";
			} else {
				temp_output += "    " + Get("Code") + " " + Get("To") + Get("ToStep") + " " + Get("Eff") + "\n";
			}
		}
		// Upper limit for flows
		if ( Get("OpMax") != "Inf" ) {
			temp_output += "    " + Get("Code") + " ub" + Get("Code") + " 1\n";
		}
	} else if ( Get("TransInfr") != "" ) {
		// Put arc in the constraint of the destination node
		if ( Get("To")[0] != 'X' ) {
			temp_output += "    " + Get("Code") + " " + Get("To") + Get("ToStep") + " 1\n";
		}
		// Upper limit due to fleet
		if ( Get("OpMax") != "Inf" ) {
			string fleetcode = Get("From") + Get("FromStep");
			fleetcode[1] = fleetcode[0];
			temp_output += "    " + Get("Code") + " ub" + fleetcode + " 1\n";
		}		
		// Upper limit due to infrastructure
		if ( Get("OpMax") != "Inf" ) {
			string infcode = Get("From") + Get("FromStep");
			infcode[0] = Get("TransInfr")[0];
			infcode[1] = Get("TransInfr")[0];
			temp_output += "    " + Get("Code") + " ub" + infcode + " 1\n";
		}
	}
	if ( !isTransport() || Get("TransInfr") != "" ) {
		temp_output += WriteEnergy2Trans();
		temp_output += WriteTrans2Energy();
	}
	// Put arc in DC power flow constraint if appropriate
	if ( isDCflow() ) {
		if ( Get("From") < Get("To") ) {
			temp_output += "    " + Get("Code") + " dcpf" + Get("Code") + " -1\n";
		} else {
			temp_code = Get("To") + Get("ToStep") + "_" + Get("From") + Get("FromStep");
			temp_output += "    " + Get("Code") + " dcpf" + temp_code + " 1\n";
		}
	}
	return temp_output;
}

string Arc::InvArcColumns() const {
	string temp_output = "";
	string temp_code;
	// If investment allowed
	if ( InvArc() && Get("TransInfr") == "" ) {
		// Cost of investment
		temp_output += "    inv" + Get("Code") + " obj " + Get("InvCost") + "\n";
		
		// Investment added to the next upper bound contraints
		Step step1, step2, stepguide, maxstep;
		step1 = Str2Step( Get("FromStep") );
		step2 = Str2Step( Get("ToStep") );
		if ( Get("LifeSpan") != "X" ) {
			maxstep = StepSum( step1, Str2Step(Get("LifeSpan")) );
			maxstep = (maxstep > SLength) ? SLength : maxstep;
		} else {
			maxstep = SLength;
		}
		
		stepguide = (step1 > step2) ? step1 : step2;
		while (stepguide <= maxstep) {
			temp_output += "    inv" + Get("Code") + " inv2cap";
			temp_output += Get("From") + Step2Str(step1);
			if ( !isTransport() ) {
				temp_output += "_" + Get("To") + Step2Str(step2);
			}
			temp_output += " -1\n";
			if ( isFirstBidirect() || isFirstTransport() ) {
				Arc Arc2(*this, true);
				temp_output += "    inv" + Get("Code") + " inv2cap";
				temp_output += Arc2.Get("From") + Step2Str(step1);
				if ( !isTransport() ) {
					temp_output += "_" + Arc2.Get("To") + Step2Str(step2);
				}
				temp_output += " -1\n";
			}
			
			// Move to the next year
			++stepguide[0]; ++step1[0]; ++step2[0];
		}
	}
	return temp_output;
}

string Arc::CapArcColumns(int selector) const {
	string temp_output = "";
	
	// If investment is allowed,
    if ( isFirstinYear() && Get("OpMax") != "Inf" && Get("TransInfr") == "" ) {
		if ( selector != 1 ) {
			// Add capacity as an upper bound for flows withing that year
			Step step1, step2, stepguide, maxstep;
			step1 = Str2Step( Get("FromStep") );
			step2 = Str2Step( Get("ToStep") );
			stepguide = (step1 > step2) ? step1 : step2;
			maxstep = ( isStorage() ) ? step1 : stepguide;
			++maxstep[0];
			
			string common = "    cap" + Get("Code") + " ub";
			while (stepguide < maxstep) {
				temp_output += common;
				temp_output += Get("From") + Step2Str(step1);
				if ( !isTransport() ) {
					temp_output += "_" + Get("To") + Step2Str(step2);
				}
				if ( InvertEff() ) {
					string reduced_cap = ToString<double>( GetDouble("Eff") * atof( Step2Hours(stepguide).c_str() ) );
					temp_output += " -" + reduced_cap + "\n";
				} else if ( Get("InvertEff") == "1" ) {
					temp_output += " -1\n";
				} else {
					temp_output += " -" + Step2Hours(stepguide) + "\n";
				}
				
				// Move to the next year
				stepguide = NextStep(stepguide);
				if (NextStep(step1) <= stepguide)
					step1 = NextStep(step1);
				if (NextStep(step2) <= stepguide)
					step2 = NextStep(step2);
			}
		}
		if (selector != 2) {
			// Add current investment to the capacity of the arc
			temp_output += "    cap" + Get("Code") + " inv2cap" + Get("Code") + " 1\n";
			
			// Contribution to peak load
			if ( Get("CapacityFactor") != "0" ) {
				temp_output += "    cap" + Get("Code");
				temp_output += " pk" + Get("To") + Get("ToStep") + " " + Get("CapacityFactor") + "\n";
			}
		}
	}
	return temp_output;
}

vector<string> Arc::Events() const {
	vector<string> temp_output(0);
	
	// If investment is allowed,
	if ( isFirstinYear() && Get("OpMax") != "Inf" && Get("TransInfr") == "" ) {
		// Base case
		temp_output.push_back( "1" );
		for (int event = 1; event <= Nevents; ++event) {
			// For events
			string property = "CapacityLoss" + ToString<int>(event);
			temp_output.push_back( Get(property)  );
		}
	}
	return temp_output;
}

string Arc::ArcRhs() const {
	string temp_output = "";
	// RHS in the upper bound constraints, for the capacity existing at t=0
	if ( isFirstinYear() && Get("OpMax") != "Inf"  && Get("TransInfr") == "" ) {
		temp_output += " rhs inv2cap" + Get("Code") + " " + Get("OpMax") + "\n";
	}
	return temp_output;
}

string Arc::ArcBounds() const {
	string temp_output = "";
	// Write minimum for operational flow
	if ( Get("OpMin") != "0"  && Get("TransInfr") == "" ) {
		temp_output += " LO bnd " + Get("Code") + " " + Get("OpMin") + "\n";
	}
	return temp_output;
}

string Arc::ArcInvBounds() const {
	string temp_output = "";
	if ( InvArc()  && Get("TransInfr") == "" ) {
		// Investment min and maximum when investment is allowed
		if ( Get("InvMin") != "0" ) {
			temp_output += " LO bnd inv" + Get("Code") + " " + Get("InvMin") + "\n";
		}
		if ( Get("InvMax") != "Inf" ) {
			temp_output += " UP bnd inv" + Get("Code") + " " + Get("InvMax") + "\n";
		}
	}
	return temp_output;
}

string Arc::WriteEnergy2Trans() const {
	string temp_output = "";
	// Load on the transportation side created by a coal/energy arc
	if (Energy2Trans) {
		temp_output += "    " + Get("Code") + " " + Get("From") + Get("To").substr(2,2) + Get("ToStep") + " -1\n";
	}
	return temp_output;
}

string Arc::WriteTrans2Energy() const {
	string temp_output = "";
	unsigned int k=0;
	// Energy demand for a transportation node that requires it
	while ( k+1 < Trans2Energy.size() ) {
		temp_output += "    " + Get("Code") + " " + Trans2Energy[k] + " -" + Trans2Energy[k+1] + "\n";
		k++; k++;
	}
	return temp_output;
}

// Get what time the arc belongs to (i.e., year)
int Arc::Time() const {
	return Str2Step(Get("FromStep"))[0];
}


// ****** Boolean functions ******

// Is it the first arc in a year?
bool Arc::isFirstinYear() const {
	bool output = true;
	Step step1, step2, stepguide;
	step1 = Str2Step( Get("FromStep") );
	step2 = Str2Step( Get("ToStep") );
	stepguide = ( (step1 > step2) || isStorage() ) ? step1 : step2;
	for ( unsigned int k = Get("InvStep").size(); k < SName.size(); k++) {
		output = output && ( (stepguide[k]==0) || (stepguide[k]==1) );
	}
	return output;
}

// Is investment allowed for current arc?
bool Arc::InvArc() const {
	// Inv. cost is declared and it's the first arc in each investment period
	bool output = (Get("InvCost") != "X") && isFirstinYear() && (Get("InvMax") != "0");
	// It's the first if the arc is bidirectional
	output = output && (!isTransport() && (!isBidirect() || isFirstBidirect()) || isFirstTransport() );
	// Technology is available
	output = output && ( Str2Step(Get("FromStep")) >= Str2Step(Get("InvStart")) );
	return output;
}

// Is efficiency inverted? (Used with electrical generators)
bool Arc::InvertEff() const {
	return ( Get("InvertEff") == "Y" || Get("InvertEff") == "y" );
}

// Is the arc part of DC flow constraints?
bool Arc::isDCflow() const {
	bool output = (Get("From").substr(0,2) == DCCode) && (Get("To").substr(0,2) == DCCode);
	return output && useDCflow;
}

// Is it a storage arc?
bool Arc::isStorage() const {
	return (Get("From").substr(1,1) == StorageCode) && (Get("From") == Get("To"));
}

// Is the arc bidirectional? (excludes storage nodes)
bool Arc::isBidirect() const {
	bool output = Get("From").substr(0,2) == Get("To").substr(0,2);
	output = output && !isDCflow() && !isStorage();
	return output;
}

// Is the arc bidirectional and and the first alphabetically? 
bool Arc::isFirstBidirect() const {
	return isBidirect() && (Get("From") < Get("To") );
}

// Is is a transportation arc?
bool Arc::isTransport() const {
	return (Get("From").size() > 4) && (Get("To").size() > 4);
}

// Is it a transportation arc and the first alphabetically?
bool Arc::isFirstTransport() const {
	return isTransport() && (Get("From").substr(2,2) < Get("From").substr(4,2));
}


// ****** Other functions ******
// Find the index for a arc property selector
int FindArcSelector(const string& selector) {
	int index = -1;
	for (unsigned int k = 0; k < ArcProp.size(); ++k) {
		if (selector == ArcProp[k]) index = k;
	}
	return index;
}
