// --------------------------------------------------------------
//    NETSCORE Version 2
//    read.h -- Definition of reading functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#ifndef _READ_H_
#define _READ_H_

#include "global.h"
#include "node.h"
#include "arc.h"

void ReadParameters(const char* fileinput, GlobalParam *p);
vector<Node> ReadListNodes(const char* fileinput, GlobalParam *p);
vector<Arc> ReadListArcs(const char* fileinput, GlobalParam *p);
MatrixStr ReadStep(const char* fileinput);
MatrixStr ReadProperties(const char* fileinput, const string& defvalue, const int num_fields);
void ReadTrans(vector<Node>& Nodes, vector<Arc>& Arcs, const char* fileinput, GlobalParam *p);
void ReadEvents(double output[], const char* fileinput);

#endif  // _READ_H_
