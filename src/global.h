// --------------------------------------------------------------
//    NETSCORE Version 2
//    global.h -- Definition of global variables and functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <string>
#include <sstream>
#include "step.h"
#include "index.h"

// Type definitions
typedef vector<string> VectorStr;
typedef vector<VectorStr> MatrixStr;

// Global variables
extern string SName;
extern Step SLength;
extern bool useDCflow, useBenders;
extern string DefStep, StorageCode, DCCode, TransStep, TransDummy, TransCoal;
extern int Npopsize, Nngen, Nobj, Nevents;
extern string Npcross_real, Npmut_real, Neta_c, Neta_m, Npcross_bin, Npmut_bin, Nstages;
extern double Np_start;
extern vector<string> ArcProp, ArcDefault, NodeProp, NodeDefault, TransInfra, TransComm, StepHours, SustObj, SustMet;
extern int NodePropOffset, ArcPropOffset, outputLevel;
// Store indices to recover data after optimization
extern Index IdxNode, IdxUd, IdxRm, IdxArc, IdxInv, IdxCap, IdxUb, IdxEm, IdxDc, IdxNsga;

// Print error messages
void printError(const string& selector, const char* fileinput);
void printError(const string& selector, const string& field);

// Print header at the beginning of execution
void printHeader(const string& selector);

// Remove comments and end of line characters
void CleanLine(char* line);

// Convert a value to a string
template <class T>
string ToString(T t) {
	stringstream ss;
	ss << t;
	return ss.str();
}

#endif  // _GLOBAL_H_
