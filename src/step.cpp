// --------------------------------------------------------------
//    NETSCORE Version 2
//    step.cpp -- Implementation of step functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"

// Converts a string like 'y1m2' into the appropriate 'Step' (vector of integers)
Step Str2Step(const string& mystep) {
	Step output(0);
	size_t found;
	int zeros = 0;
	for (unsigned int k = 0; k < SName.size(); k++) {
		found = mystep.find(SName[k]);
		if (found!=string::npos) {
			output.push_back(atoi(mystep.substr(found+1).c_str()));
		} else {
			output.push_back(0);
			++zeros;
		}
	}
	if (zeros == SName.size()) {
		output[0] = -1;
	}
	return output;
}

// Converts the appropriate 'Step' (vector of integers) into its corresponding string (such as 'y1m2')
string Step2Str(const Step& mystep) {
	string output = "";
	char printstep[3];
	for (unsigned int k=0; k < SName.size(); k++) {
		if (mystep[k] > 0) {
			output += SName.substr(k,1) + ToString<int>(mystep[k]);
		}
	}
	return output;
}

// Given a 'Step', finds the next one
Step NextStep(const Step& mystep) {
	Step output = mystep;
	bool summed = false;
	int kk;
	for (unsigned int k = SLength.size(); k>0; k--) {
		kk = k-1;
		if ((output[kk] > 0) && (!summed)) {
			output[kk] += 1;
			summed = true;
		}
		if ((output[kk] > SLength[kk]) && (kk>0)) {
			output[kk] -= SLength[kk];
			output[kk-1] += 1;
		}
	};
	return output;
}

// Sum two 'Step' variables
Step StepSum(const Step& a, const Step& b) {
	Step output = a;
	for (int k = SLength.size()-1; k >= 0; --k) {
		output[k] += b[k];
		while ( (output[k] > SLength[k]) && (k>0) ) {
			output[k] -= SLength[k];
			output[k-1] += 1;
		}
	};
	return output;
}

// Given a 'Step', it determines the column position (for reading properties).
// It goes like this: 'const' 'y1' 'y1m1' 'y1m1h1' 'y1m1h2' ... 'y1m2' etc.
int Step2Pos(const Step& mystep) {
	int output = 0, temp_size;
	unsigned int j = 0;
	while ((j < mystep.size()) && (mystep[j] !=0)) {
		temp_size = 1;
		for (unsigned int k = mystep.size()-1; k > j; k--) {
			temp_size = temp_size * SLength[k] + 1;
		}
		output += (mystep[j]-1) * temp_size + 1;
		j++;
	}
	return output;
}

// Given a 'Step', it determines the column position (for writing output).
// It goes like this: 'y1' 'y2' ... 'y1m1' 'y1m2' ... 'y1m1h1' 'y1m1h2' etc.
int Step2Col(const Step& mystep) {
	int output = 1, temp, num = 0;
	for (int i = mystep.size()-2; i >= 0 ; --i) {
		if (mystep[i+1] != 0) {
			output = 1 + output * SLength[i];
			num++;
		}
	}
	output--;
	for (unsigned int i = 0; i <= num; ++i) {
		temp = 1;
		for (unsigned int k = i+1; k <= num; ++k) temp = temp * SLength[k];
		output += (mystep[i] - 1) * temp;
	}
	return output;
}

// Given a 'Step', find its length in hours, which is stored in the global variable StepHours
string Step2Hours(const Step& mystep) {
	int idx;
	if ( mystep[SName.size()-1] != 0) {
		idx = (SName.size() -1) + (mystep[SName.size()-1] - 1);
	} else {
		idx = -1;
		for (int j=0; j<SName.size()-1; ++j) {
			if (mystep[j] != 0) ++idx;
		}
	}
	return StepHours[idx];
}
