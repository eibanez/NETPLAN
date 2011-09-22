// --------------------------------------------------------------
//    NETSCORE Version 2
//    global.cpp -- Implementation of global variables and functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#include <time.h>
#include <math.h>
#include "global.h"
extern int outputLevel;
time_t startTime, endTime;

// Structure for global parameters ****************************************************************
GlobalParam::GlobalParam() {
	// Node parameters
	NodeProp = vector<string>(0);
	NodeDefault = vector<string>(0);
	
	// Arc parameters
	ArcProp = vector<string>(0);
	ArcDefault = vector<string>(0);
	
	// Transportation variables
	TransStep = "";
	TransDummy = "XT";
	TransInfra = vector<string>(0);
	TransComm = vector<string>(0);
}

// Print error messages
void printError(const string& selector, const char* fileinput) {
	if (selector == "warning") {
		if (outputLevel < 3)
			cout << "\tWarning: File '" << fileinput << "' not found!\n";
	} else
		cout << "\tERROR: File '" << fileinput << "' not found!\n";
}

void printError(const string& selector, const string& field) {
	if      (selector == "noderead")  cout << "\tERROR: Invalid field '" << field << "' for a Node (reading mode)\n";
	else if (selector == "arcread")   cout << "\tERROR: Invalid field '" << field << "' for an Arc (reading mode)\n";
	else if (selector == "nodewrite") cout << "\tERROR: Invalid field '" << field << "' for a Node (writing mode)\n";
	else if (selector == "arcwrite")  cout << "\tERROR: Invalid field '" << field << "' for an Arc (writing mode)\n";
	else if (selector == "nodestep")  cout << "\tERROR: Node '" << field << "' without defined step\n";
	else if (selector == "arcstep")   cout << "\tERROR: Arc '" << field << "' without defined step\n";
	else if (selector == "parameter") cout << "\tERROR: General parameter '" << field << "' caused a problem\n";
	else                              cout << "\tERROR and error code '" << selector << "' not defined\n";
}

// Print header at the beginning of execution
void printHeader(HeaderOption selector) {
	switch (selector) {
	case H_Prep:
		cout << endl;
		cout << "==========================================" << endl;
		cout << "|  NETSCORE-21 Long-term planning model  |" << endl;
		cout << "|          Pre-processing module         |" << endl;
		cout << "==========================================" << endl;
		printHeader(H_Time);
		break;
	case H_Post:
		cout << endl;
		cout << "==========================================" << endl;
		cout << "|  NETSCORE-21 Long-term planning model  |" << endl;
		cout << "|         Post-processing module         |" << endl;
		cout << "==========================================" << endl;
		printHeader(H_Time);
		break;
	case H_PostNsga:
		cout << endl;
		cout << "==========================================" << endl;
		cout << "|  NETSCORE-21 Long-term planning model  |" << endl;
		cout << "|     NSGA-II post-processing module     |" << endl;
		cout << "==========================================" << endl;
		printHeader(H_Time);
		break;
	case H_Benders:
		cout << endl;
		cout << "==========================================" << endl;
		cout << "|  NETSCORE-21 Long-term planning model  |" << endl;
		cout << "|          Benders solver module         |" << endl;
		cout << "==========================================" << endl;
		printHeader(H_Time);
		break;
	case H_Nsga:
		cout << endl;
		cout << "==========================================" << endl;
		cout << "|  NETSCORE-21 Long-term planning model  |" << endl;
		cout << "|      NSGA-II multiobjective solver     |" << endl;
		cout << "==========================================" << endl;
		printHeader(H_Time);
		break;
	case H_NsgaParallel:
		cout << endl;
		cout << "==========================================" << endl;
		cout << "|  NETSCORE-21 Long-term planning model  |" << endl;
		cout << "|         NSGA-II parallel solver        |" << endl;
		cout << "==========================================" << endl;
		printHeader(H_Time);
		break;
	case H_Completed:
		cout << endl;
		printHeader(H_Elapsed);
		cout << "===========  Process completed  ==========" << endl << endl;
		break;
	case H_Time:
		time(&startTime);
		cout << "  Current time: " << ctime(&startTime) << endl;
		break;
	case H_Elapsed:
		{ time(&endTime);
		double difTime = difftime (endTime,startTime);
		double hours = floor(difTime/3600), mins = floor(difTime/60) - hours * 60;
		difTime -= hours * 3600 + mins * 60;
		cout << "  Current time: " << ctime(&endTime);
		cout << "  Elapsed time: ";
		if (hours > 0) cout << hours << " h ";
		if (mins > 0) cout << mins << " m ";
		cout << difTime << " s" << endl; }
		break;
	default:
		cout << endl;
		cout << "==========================================" << endl;
		cout << "|  NETSCORE-21 Long-term planning model  |" << endl;
		cout << "==========================================" << endl;
		printHeader(H_Time);
	}
}

// Remove comments and end of line characters
void CleanLine(char* line) {
	line = strtok(line, " %");
	char *nlptr = strchr(line, '\n');
	if (nlptr) *nlptr = '\0';
	char *nlptr2 = strchr(line, '\r');
	if (nlptr2) *nlptr2 = '\0';
}
