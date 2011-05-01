// --------------------------------------------------------------
//    NETSCORE Version 2
//    netscore.h -- Library of definitions and functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#include "global.h"
#include "node.h"
#include "arc.h"
#include "index.h"
#include "read.h"
#include "write.h"

// Global variables
string SName;
Step SLength;
bool useDCflow = false, useBenders = false;
string DefStep = "", StorageCode = "S", DCCode = "", TransStep = "", TransDummy = "XT", TransCoal = "";
int Npopsize = 20, Nngen = 200, Nobj = 1, Nevents = 0;
string Npcross_real = "0.75", Npmut_real = "0.2", Neta_c = "7", Neta_m = "20", Npcross_bin = "0.4", Npmut_bin = "0.7", Nstages = "2";
double Np_start = 0.5;
vector<string> ArcProp(0), ArcDefault(0), NodeProp(0), NodeDefault(0), TransInfra(0), TransComm(0), StepHours(0), SustObj(0), SustMet(0);
int NodePropOffset = 0, ArcPropOffset = 0, outputLevel = 2;
// Store indices to recover data after optimization
Index IdxNode, IdxUd, IdxRm, IdxArc, IdxInv, IdxCap, IdxUb, IdxEm, IdxDc, IdxNsga;
// int startCap, startInv, startEm, startRm, startArc, startUd, startDc;

int FindCode(const string& mystr, const MatrixStr mymatrix);
int FindCode(const string& strfrom, const string& strto, const MatrixStr mymatrix);
int FindCode(const Node& mynode, const MatrixStr mymatrix);
int FindCode(const Arc& myarc, const MatrixStr mymatrix);


// Write DC Power flow columns in the MPS file
vector<string> DCFlowColumns(const vector<Node>& v, const vector<Arc>& w) {
	vector<string> temp_output(SLength[0]+1, "");
	for (unsigned int i = 0; i < v.size(); ++i) {
		for (unsigned int j = 0; j < w.size(); ++j) {
			if ( w[j].Get("From") < w[j].Get("To") ) {
				if ( v[i].Get("Code") == (w[j].Get("From") + w[j].Get("FromStep")) ) {
					temp_output[ v[i].Time() ] += "    th" + v[i].Get("Code") + " dcpf" + w[j].Get("Code") + " " + w[j].Get("Suscep") + "\n";
				} else if ( v[i].Get("Code") == (w[j].Get("To") + w[j].Get("ToStep")) ) {
					temp_output[ v[i].Time() ] += "    th" + v[i].Get("Code") + " dcpf" + w[j].Get("Code") + " -" + w[j].Get("Suscep") + "\n";
				}
			}
		}
	}
	return temp_output;
}


// Given a matrix of values, finds the row that fits best to a code. It tries to match the whole code, two or one letters.
int FindCode(const string& mystr, const MatrixStr mymatrix) {
	int output = -1, one_char = -1, two_char = -1, k = 0;
	
	if (mymatrix.size() > 0) {
		while ((k < mymatrix.size()) && (output == -1)) {
			if (mystr == mymatrix[k][0]) output = k;
			else if (mystr.substr(0,1) == mymatrix[k][0]) one_char = k;
			else if (mystr.substr(0,2) == mymatrix[k][0]) two_char = k;
			k++;
		}
	}
	if (output == -1) {
		if (one_char != -1) output = one_char;
		if (two_char != -1) output = two_char;
	}
	
	return output;
}

// This function has the same porpuse, but prepared for arcs
int FindCode(const string& strfrom, const string& strto, const MatrixStr mymatrix) {
	int output = -1, one_one = -1, one_two = -1, one_all = -1, two_two = -1, two_all = -1, k = 0;
	int zero_one = -1, zero_two = -1, zero_all = -1;
	
	if (mymatrix.size() > 0) {
		while ((k < mymatrix.size()) && (output == -1)) {
			if (strfrom == mymatrix[k][0]) {
				if (strto == mymatrix[k][1]) output = k;
				else if (strto.substr(0,1) == mymatrix[k][1]) one_all = k;
				else if (strto.substr(0,2) == mymatrix[k][1]) two_all = k;
				else if (mymatrix[k][1] == "") zero_all = k;
			} else if (strfrom.substr(0,2) == mymatrix[k][0]) {
				if (strto == mymatrix[k][1]) two_all = k;
				else if (strto.substr(0,1) == mymatrix[k][1]) one_two = k;
				else if (strto.substr(0,2) == mymatrix[k][1]) two_two = k;
				else if (mymatrix[k][1] == "") zero_two = k;
			} else if (strfrom.substr(0,1) == mymatrix[k][0]) {
				if (strto == mymatrix[k][1]) one_all = k;
				else if (strto.substr(0,1) == mymatrix[k][1]) one_one = k;
				else if (strto.substr(0,2) == mymatrix[k][1]) one_two = k;
				else if (mymatrix[k][1] == "") zero_one = k;
			} else if (mymatrix[k][0] == "") {
				if (strto == mymatrix[k][1]) zero_all = k;
				else if (strto.substr(0,1) == mymatrix[k][1]) zero_one = k;
				else if (strto.substr(0,2) == mymatrix[k][1]) zero_two = k;
			}
			k++;
		}
	}
	if (output == -1) {
		if (zero_one != -1) output = zero_one;
		if (zero_two != -1) output = zero_two;
		if (zero_all != -1) output = zero_all;
		if (one_one != -1) output = one_one;
		if (one_two != -1) output = one_two;
		if (one_all != -1) output = one_all;
		if (two_two != -1) output = two_two;
		if (two_all != -1) output = two_all;
	}
	
	return output;
}

// Shortcuts for nodes and arcs
int FindCode(const Node& mynode, const MatrixStr mymatrix) {
	return FindCode(mynode.Get("ShortCode"), mymatrix);
}

int FindCode(const Arc& myarc, const MatrixStr mymatrix) {
	int output = -1;
	// Look for properties for the arc in the opposite direction
	Arc myarc2(myarc, true);
	if ( myarc.isBidirect() || myarc.isTransport() ) {
		int code1 = FindCode(myarc.Get("From"), myarc.Get("To"), mymatrix);
		int code2 = FindCode(myarc2.Get("From"), myarc2.Get("To"), mymatrix);
		output = (code1 >= code2) ? code1 : code2;
	} else {
		output = FindCode(myarc.Get("From"), myarc.Get("To"), mymatrix);
	}
	return output;
}
