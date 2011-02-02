// --------------------------------------------------------------
//    NETSCORE Version 1
//    write.h -- Definition of file writing functions
//    2009-2010 (c) Eduardo Ibáñez
// --------------------------------------------------------------

#ifndef _WRITE_H_
#define _WRITE_H_

void WriteOutput(const char* fileinput, Index& idx, vector<string>& values, const string& header);
void WriteOutput(const char* fileinput, Index& idx, vector<string>& values, const int start, const string& header);
void WriteOutput(const char* fileinput, Index& idx, vector<Node>& Nodes, const string& selector, const string& header);
void WriteOutput(const char* fileinput, Index& idx, vector<Arc>& Arcs, const string& selector, const string& header);

#endif  // _WRITE_H_
