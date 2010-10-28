// --------------------------------------------------------------
//    NETSCORE Version 1
//    global.cpp -- Implementation of global variables and functions
//    2009-2010 (c) Eduardo Ibáñez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <math.h>
extern int outputLevel;
time_t startTime, endTime;

// Print error messages
void printError(const string& selector, const char* fileinput) {
	if (selector == "warning") {
		if (outputLevel < 3 )    cout << "      Warning: File '" << fileinput << "' not found!\n";
	} else {                     cout << "      ERROR: File '" << fileinput << "' not found!\n"; }
}
void printError(const string& selector, const string& field) {
	if      (selector == "noderead")  cout << "      ERROR: Invalid field '" << field << "' for a Node (reading mode)\n";
	else if (selector == "arcread")   cout << "      ERROR: Invalid field '" << field << "' for an Arc (reading mode)\n";
	else if (selector == "nodewrite") cout << "      ERROR: Invalid field '" << field << "' for a Node (writing mode)\n";
	else if (selector == "arcwrite")  cout << "      ERROR: Invalid field '" << field << "' for an Arc (writing mode)\n";
	else if (selector == "nodestep")  cout << "      ERROR: Node '" << field << "' without defined step\n";
	else if (selector == "arcstep")   cout << "      ERROR: Arc '" << field << "' without defined step\n";
	else if (selector == "parameter") cout << "      ERROR: General parameter '" << field << "' caused a problem\n";
	else                              cout << "      ERROR and error code '" << selector << "' not defined\n";
}

// Print header at the beginning of execution
void printHeader(const string& selector) {
	if (selector == "default") {
		cout << endl;
		cout << "==========================================" << endl;
		cout << "|  NETSCORE-21 Long-term planning model  |" << endl;
		cout << "==========================================" << endl;
		printHeader("time");
	} else if (selector == "preprocessor") {
		cout << endl;
		cout << "==========================================" << endl;
		cout << "|  NETSCORE-21 Long-term planning model  |" << endl;
		cout << "|          Pre-processing module         |" << endl;
		cout << "==========================================" << endl;
		printHeader("time");
	} else if (selector == "postprocessor") {
		cout << endl;
		cout << "==========================================" << endl;
		cout << "|  NETSCORE-21 Long-term planning model  |" << endl;
		cout << "|         Post-processing module         |" << endl;
		cout << "==========================================" << endl;
		printHeader("time");
	} else if (selector == "benders") {
		cout << endl;
		cout << "==========================================" << endl;
		cout << "|  NETSCORE-21 Long-term planning model  |" << endl;
		cout << "|          Benders solver module         |" << endl;
		cout << "==========================================" << endl;
		printHeader("time");
	} else if (selector == "nsga") {
		cout << endl;
		cout << "==========================================" << endl;
		cout << "|  NETSCORE-21 Long-term planning model  |" << endl;
		cout << "|      NSGA-II multiobjective solver     |" << endl;
		cout << "==========================================" << endl;
		printHeader("time");
	} else if (selector == "completed") {
		time(&endTime);
		double difTime = difftime (endTime,startTime);
		double hours = floor(difTime/3600), mins = floor(difTime/60) - hours * 60;
		difTime -= hours * 3600 + mins * 60;
		cout << endl << "  Current time: " << ctime(&endTime);
		cout << "  Execution time: ";
		if (hours > 0) cout << hours << " h ";
		if (mins > 0) cout << mins << " m ";
		cout << difTime << " s" << endl;
		cout << "===========  Process completed  ==========" << endl << endl;
	} else if (selector == "time") {
		time(&startTime);
		cout << "  Current time: " << ctime(&startTime) << endl;
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
