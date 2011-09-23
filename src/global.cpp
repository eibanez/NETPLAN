// --------------------------------------------------------------
//    NETSCORE Version 2
//    global.cpp -- Implementation of global variables and functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#include <algorithm>
#include <time.h>
#include <math.h>
#include "global.h"
#include "arc.h"
#include "node.h"
extern int outputLevel;
time_t startTime, endTime;

// Global parameters ******************************************************************************
GlobalParam::GlobalParam() {
	// Node parameters
	NodeProp = vector<string>(N_SIZE);
	NodeDefault = vector<string>(N_SIZE);
	
	// Arc parameters
	ArcProp = vector<string>(0);
	ArcDefault = vector<string>(0);
	
	// Transportation variables
	TransStep = "";
	TransDummy = "XT";
	TransInfra = vector<string>(0);
	TransComm = vector<string>(0);
	
	// Common parameters
	string DefStep = "";
	
	// Steps
	s = NULL;
}

// Step variables *********************************************************************************
GlobalStep::GlobalStep(string text, vector<string>& shrs) {
	
	
	using std::replace;
	
	// Isolate characters that are not numbers
	string tChars = text;
	replace(tChars.begin(), tChars.end(), '0', '\0');
	replace(tChars.begin(), tChars.end(), '1', '\0');
	replace(tChars.begin(), tChars.end(), '2', '\0');
	replace(tChars.begin(), tChars.end(), '3', '\0');
	replace(tChars.begin(), tChars.end(), '4', '\0');
	replace(tChars.begin(), tChars.end(), '5', '\0');
	replace(tChars.begin(), tChars.end(), '6', '\0');
	replace(tChars.begin(), tChars.end(), '7', '\0');
	replace(tChars.begin(), tChars.end(), '8', '\0');
	replace(tChars.begin(), tChars.end(), '9', '\0');
	
	Chars = "";
	for (unsigned int k = 0; k < tChars.size(); ++k)
		if (tChars[k] != '\0')
			Chars.push_back(tChars[k]);
	
	// Store the values for the maximum step
	vector<int> MaxStep(Chars.size(), 0);
	size_t found;
	int zeros = 0;
	for (unsigned int k = 0; k < Chars.size(); ++k) {
		found = text.find(Chars[k]);
		if (found != string::npos)
			MaxStep[k] = atoi(text.substr(found + 1).c_str());
	}
	
	// Maximum position, lengths of different steps and num of years
	Length = vector<int>(Chars.size(), 0);
	MaxPos = MaxStep.front();
	Length[0] = MaxStep.front();
	NumYears = MaxStep.front();
	YearChar = Chars.substr(0, 1);
	for (unsigned int j = 1; j < Chars.size(); ++j) {
		Length[j] = Length[j - 1] * MaxStep[j];
		MaxPos += Length[j];
	}
	
	// What year does the position belong?
	Year = vector<int>(0);
	YearString = vector<string>(0);
	for (int i = 1; i <= NumYears; ++i) {
		for (int j = 0; j < MaxPos / NumYears; ++j) {
			Year.push_back(i);
			YearString.push_back(YearChar + ToString<int>(i));
		}
	}
	
	// Calculate how many hours are there for each step
	int laststep = MaxStep[Chars.size() - 1], temp_hour = 0;
	vector<int> stephours(laststep, 1);
	if (shrs.size() == 0) {
		temp_hour = laststep;
	} else if (shrs.size() == laststep) {
		for (int i = 0; i < laststep; ++i) {
			stephours[i] = atoi(shrs[i].c_str());
			temp_hour += stephours[i];
		}
	} else {
		if (shrs.size() > 1)
			printError("parameter", string("StepHours"));
		for (int i = 0; i < laststep; ++i)
			stephours[i] = atoi(shrs[0].c_str());
		temp_hour = laststep * stephours[0];
	}
	for (int j = Chars.size() - 2; j >= 0; --j) {
		stephours.insert(stephours.begin(), temp_hour);
		temp_hour = temp_hour * MaxStep[j];
	}
	
	// Columns, next steps, strings, and step hours
	NextStep = vector<int>(MaxPos);
	Col = vector<int>(MaxPos);
	Text = vector<string>(MaxPos);
	Hours = vector<int>(MaxPos);
	isFirstYear = vector<bool>(MaxPos);
	int pos, prevpos = -1;
	
	// i represents the number of digits
	for (int i = 0; i < Chars.size(); ++i) {
		vector<int> refStep(Chars.size(), 0);
		for (int j = 0; j <= i; ++j)
			refStep[j] = 1;
		
		pos = i;
		
		while (pos < MaxPos) {
			// String representation
			string str = "";
			for (int j = 0; j <= i; ++j)
				str += Chars.substr(j, 1) + ToString<int>(refStep[j]);
			Text[pos] = str;
			
			// Step length
			if (refStep.back() == 0)
				Hours[pos] = stephours[i];
			else
				Hours[pos] = stephours[i + refStep.back() - 1];
			
			// Is it the first in the year?
			bool isFirst = true;
			for (int j = 0; j <= i; ++j)
				isFirst = isFirst && (refStep[j] == 1);
			isFirstYear[pos] = isFirst;
			
			// Next step
			prevpos = pos;
			++refStep[i];
			for (int k = i; k >= 1; --k) {
				if (refStep[k] > MaxStep[k]) {
					refStep[k] -= MaxStep[k];
					++refStep[k - 1];
				}
			}
			
			// Next position
			pos = -1;
			for (int j = 0; j <= i; ++j) {
				int temp_size = 1;
				for (int k = Chars.size() - 1; k > j; --k)
					temp_size = temp_size * MaxStep[k] + 1;
				
				pos += (refStep[j] - 1) * temp_size + 1;
			}
			NextStep[prevpos] = pos;
		}
	}
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
	else if (selector == "step")      cout << "\tERROR: Missing step value 'StepLength'" << endl;
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
