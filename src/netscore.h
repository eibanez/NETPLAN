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
bool useDCflow = false;
string StorageCode = "S", DCCode = "", TransCoal = "";
int Npopsize = 20, Nngen = 200, Nobj = 1, Nevents = 0;
string Npcross_real = "0.75", Npmut_real = "0.2", Neta_c = "7", Neta_m = "20", Npcross_bin = "0.4", Npmut_bin = "0.7", Nstages = "2";
double Np_start = 0.5;
vector<string> ArcProp(0), StepHours(0), SustObj(0), SustMet(0);
int ArcPropOffset = 0, outputLevel = 2;
// Store indices to recover data after optimization
Index IdxNode, IdxUd, IdxRm, IdxArc, IdxInv, IdxCap, IdxUb, IdxEm, IdxDc, IdxNsga;

// Write DC Power flow columns in the MPS file
vector<string> DCFlowColumns(const vector<Node>& v, const vector<Arc>& w) {
	vector<string> temp_output(SLength[0] + 1, "");
	for (unsigned int i = 0; i < v.size(); ++i) {
		for (unsigned int j = 0; j < w.size(); ++j) {
			if (w[j].Get("From") < w[j].Get("To")) {
				if (v[i].Get(N_Code) == (w[j].Get("From") + w[j].Get("FromStep"))) {
					temp_output[ v[i].Time() ] += "    th" + v[i].Get(N_Code) + " dcpf" + w[j].Get("Code") + " " + w[j].Get("Suscep") + "\n";
				} else if (v[i].Get(N_Code) == (w[j].Get("To") + w[j].Get("ToStep"))) {
					temp_output[ v[i].Time() ] += "    th" + v[i].Get(N_Code) + " dcpf" + w[j].Get("Code") + " -" + w[j].Get("Suscep") + "\n";
				}
			}
		}
	}
	return temp_output;
}
