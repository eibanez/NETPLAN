// --------------------------------------------------------------
//    NETSCORE Version 2
//    read.cpp -- Implementation of reading functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "node.h"
#include "arc.h"

#define CHAR_LINE 15000

// Read global parameters
void ReadParameters(const char* fileinput) {
	char* t_read;
	string prop, value, discount = "0", inflation = "0", demandrate = "0", peakdemandrate = "0";
	char line [CHAR_LINE];
	
	FILE *file = fopen(fileinput, "r");
	if ( file != NULL ) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if ( fgets(line, sizeof line, file) == NULL ) {
				break;
			}
			// Remove comments and end of line characters
			CleanLine(line);
			
			if ( line[0] != '$' && line[0] != '/' && line[0] != '#' && line[0] != '%' ) {
				t_read = strtok(line,",");
				prop = string(t_read);
				t_read = strtok(NULL,",");
				value = string(t_read);
				
				// Apply read item
				if (prop == "StepName") SName = value;
				else if (prop == "StepLength") SLength = Str2Step(value);
				else if (prop == "StepHours") StepHours.push_back(value);
				else if (prop == "UseDCFlow") useDCflow = (value == "true" || value == "True" || value == "TRUE");
				else if (prop == "UseBenders") useBenders = (value == "true" || value == "True" || value == "TRUE");
				else if (prop == "OutputLevel") outputLevel = atoi(value.c_str());
				else if (prop == "CodeDC") DCCode = value;
				else if (prop == "DefStep") DefStep = value;
				else if (prop == "DefDiscount") discount = value;
				else if (prop == "DefInflation") inflation = value;
				else if (prop == "DefDemandRate") demandrate = value;
				// Transportation parameters
				else if (prop == "TransStep") TransStep = value;
				else if (prop == "TransInfra") TransInfra.push_back(value);
				else if (prop == "TransComm") TransComm.push_back(value);
				else if (prop == "TransCoal") TransCoal = value;
				// Sustainability metrics and objectives
				else if (prop == "AddObj") SustObj.push_back(value);
				else if (prop == "AddMetric") SustMet.push_back(value);
				// Resiliency parameters
				else if (prop == "NumberEvents") Nevents = atoi(value.c_str());
				// NSGA-II parameters
				else if (prop == "popsize") Npopsize = atoi(value.c_str());
				else if (prop == "ngen") Nngen = atoi(value.c_str());
				else if (prop == "pcross_real") Npcross_real = value;
				else if (prop == "pmut_real") Npmut_real = value;
				else if (prop == "eta_c") Neta_c = value;
				else if (prop == "eta_m") Neta_m = value;
				else if (prop == "pcross_bin") Npcross_bin = value;
				else if (prop == "pmut_bin") Npmut_bin = value;
				else if (prop == "stages") Nstages = value;
				else if (prop == "pstart") Np_start = atof(value.c_str());
				else { printError("parameter", prop); }
			}
		}
		fclose(file);
	} else { printError("error", fileinput); }
	
	// Calculate how many hours are there for each step and store it in StepHours
	int laststep = SLength[SName.size()-1], temp_hour = 0;
	if ( StepHours.size() == 0) {
		for (int i=0; i < laststep; ++i) StepHours.push_back("1");
		temp_hour = laststep;
	} else if ( StepHours.size() == laststep ) {
		for (int i=0; i < laststep; ++i) {
			temp_hour += atoi(StepHours[i].c_str());
		}
	} else {
		if (StepHours.size() > 1) printError("parameter", string("StepHours"));
		for (int i=1; i < StepHours.size(); ++i) StepHours[i] = StepHours[0];
		for (int i=StepHours.size(); i < laststep; ++i) StepHours.push_back( StepHours[0] );
		temp_hour = laststep * atoi(StepHours[0].c_str());
	}
	for (int j = SName.size()-2; j >= 0; --j) {
		StepHours.insert( StepHours.begin(), ToString<int>(temp_hour) );
		temp_hour = temp_hour * SLength[j];
	}
	
	// Number of objectives
	Nobj = 1 + SustObj.size();
	if (Nevents > 0) { ++Nobj; }
	SustMet.insert( SustMet.begin(), SustObj.begin(), SustObj.end() );
	
	// Declare a vector with the node property codes
	NodeProp.push_back("Code"); NodeDefault.push_back("X");
	NodeProp.push_back("ShortCode"); NodeDefault.push_back("X");
	NodeProp.push_back("Step"); NodeDefault.push_back(DefStep);
	NodeProp.push_back("StepLength"); NodeDefault.push_back("X");
	NodeProp.push_back("Demand"); NodeDefault.push_back("0");
	NodeProp.push_back("DemandPower"); NodeDefault.push_back("X");
	NodeProp.push_back("DemandRate"); NodeDefault.push_back(demandrate);
	NodeProp.push_back("PeakPower"); NodeDefault.push_back("X");
	NodeProp.push_back("PeakPowerRate"); NodeDefault.push_back(peakdemandrate);
	NodeProp.push_back("CostUD"); NodeDefault.push_back("X");
	NodeProp.push_back("DiscountRate"); NodeDefault.push_back(discount);
	NodeProp.push_back("InflationRate"); NodeDefault.push_back(inflation);
	NodePropOffset = 4;
	
	// Declare a vector with the arc property codes
	string startzero = SName.substr(0,1) + "2";
	ArcProp.push_back("Code"); ArcDefault.push_back("X");
	ArcProp.push_back("From"); ArcDefault.push_back("X");
	ArcProp.push_back("To"); ArcDefault.push_back("X");
	ArcProp.push_back("FromStep"); ArcDefault.push_back(DefStep);
	ArcProp.push_back("ToStep"); ArcDefault.push_back(DefStep);
	ArcProp.push_back("StepLength"); ArcDefault.push_back("X");
	ArcProp.push_back("InvStep"); ArcDefault.push_back(SName.substr(0,1));
	ArcProp.push_back("TransInfr"); ArcDefault.push_back("");
	ArcProp.push_back("OpCost"); ArcDefault.push_back("0");
	ArcProp.push_back("InvCost"); ArcDefault.push_back("X");
	ArcProp.push_back("DiscountRate"); ArcDefault.push_back(discount);
	ArcProp.push_back("InflationRate"); ArcDefault.push_back(inflation);
	ArcProp.push_back("Distance"); ArcDefault.push_back("X");
	ArcProp.push_back("OpMin"); ArcDefault.push_back("0");
	ArcProp.push_back("OpMax"); ArcDefault.push_back("Inf");
	ArcProp.push_back("InvMin"); ArcDefault.push_back("0");
	ArcProp.push_back("InvMax"); ArcDefault.push_back("Inf");
	ArcProp.push_back("InvStart"); ArcDefault.push_back(startzero);
	ArcProp.push_back("LifeSpan"); ArcDefault.push_back("X");
	ArcProp.push_back("Eff"); ArcDefault.push_back("1");
	ArcProp.push_back("InvertEff"); ArcDefault.push_back("N");
	ArcProp.push_back("Suscep"); ArcDefault.push_back("X");
	ArcProp.push_back("CapacityFactor"); ArcDefault.push_back("0");
	// Sustainability
	for (int j = 0; j < SustMet.size(); ++j) { ArcProp.push_back("Op" + SustMet[j]); ArcDefault.push_back("0"); }
	// Resiliency events
	for (int j = 1; j <= Nevents; ++j) {
		ArcProp.push_back("CapacityLoss" + ToString<int>(j)); ArcDefault.push_back("1");
	}
	
	ArcPropOffset = 8;
}

// Read properties file and store it in a matrix of strings.
// The first 'num_fields' columns are copied and not touched.
// The rest of the columns correspond to the different 'Steps' as determined by the function 'Step2Pos'
MatrixStr ReadProperties(const char* fileinput, const string& defvalue, const int num_fields) {
	VectorStr Values(Step2Pos(SLength) + num_fields + 1), Header(0);
	MatrixStr output(0);
	char* t_read;
	string t2_read;
	char line [CHAR_LINE];
	int i = 0, j = 0;
	
	FILE *file = fopen(fileinput, "r");
	if ( file != NULL ) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if ( fgets(line, sizeof line, file) == NULL ) {
				break;
			}
			// Remove comments and end of line characters
			CleanLine(line);
			
			// Avoid line comments
			if ( line[0]!='%' || line[0]!='\0' ) {
				if (i==0) {
					// Read column headers and store them
					i++;
					
					// Skip first 'num_fields' columns
					t_read = strtok(line,",");
					for (int k=0; k < num_fields; k++) {
						 t_read = strtok(NULL, ",");
						 j++;
					}
					
					while (t_read != NULL) {
						Header.push_back(t_read);
						t_read = strtok(NULL, ",");
						j++;
					}
				} else {
					// A line of properties is stored here
					VectorStr temp_read(j);
					const char *str = line;
					const char *delims = ",";
					int k = 0;
					
					for (unsigned int m = num_fields; m < Values.size(); m++) Values[m] = defvalue;
					
					size_t start = 0;
					
					while (str[start] != '\0') {
						size_t end = strcspn(str + start, delims);
						t2_read = string(line).substr(start,end);
						
						if (k < num_fields) {
							Values[k] = t2_read;
						} else if (end != 0) {
							if (Header[k-num_fields] == "const") {
								for (unsigned int m = num_fields; m < Values.size(); m++)  Values[m] = t2_read;
							} else {
								Step Temp_Step = Str2Step(Header[k-num_fields]);
								if (Temp_Step[0] == 0) {
									// Step is smaller than a year (to repeat monthly data, etc.)
									Step Temp_Begin(SName.size(), 0);
									bool zeros = true;
									for (int l = 0; (l < SName.size()) & zeros; ++l) {
										if (Temp_Step[l] == 0) {
											Temp_Begin[l] = 1;
										} else {
											zeros = false;
										}
									}
									while (Temp_Begin < SLength) {
										int m = Step2Pos( StepSum(Temp_Step, Temp_Begin) ) + num_fields;
										Values[m] = t2_read;
										Temp_Begin = NextStep(Temp_Begin);
									}
								} else if (Temp_Step[0] > 0) {
									// Step with year
									int a = Step2Pos(Temp_Step) + num_fields;
									int b = Step2Pos(NextStep(Temp_Step)) + num_fields;
									for (int m = a; (m < b) && (m < Values.size()); m++ )  Values[m] = t2_read;
								}
							}
						}
						start += (str[start + end] != '\0') ? end + 1 : end;
						k++;
					}
					output.push_back(Values);
				}
			}
		}
		fclose(file);
	} else printError("warning", fileinput);
	return output;
}

// Read Node list and store it in a vector of 'Nodes' (only 'ShortCode' is stored)
vector<Node> ReadListNodes(const char* fileinput) {
	vector<Node> output(0);
	char* t_read;
	char line [CHAR_LINE];
	int i = 0;
	
	FILE *file = fopen(fileinput, "r");
	if ( file != NULL ) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if ( fgets(line, sizeof line, file) == NULL ) {
				break;
			}
			// Remove comments and end of line characters
			CleanLine(line);
			
			// Skip first line
			if ( i!=0 && line[0]!='%' && line[0]!='\0' ) {
				Node Temp_Node;
				t_read = strtok(line,",");
				Temp_Node.Set("ShortCode", string(t_read));
				output.push_back(Temp_Node);
			}
			i++;
		}
		fclose(file);
	} else printError("error", fileinput);
	return output;
}

// Read Step list and store it in a matrix of strings
MatrixStr ReadStep(const char* fileinput) {
	MatrixStr output(0);
	VectorStr Temp_Vector(2);
	//char* t_read;
	char line [CHAR_LINE];
	int i = 0;

	FILE *file = fopen(fileinput, "r");
	if ( file != NULL ) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if ( fgets(line, sizeof line, file) == NULL ) {
				break;
			}
			// Remove newline character
			CleanLine(line);
			
			// Skip first line, then read the rest
			if ( i!=0 && line[0]!='%' && line[0]!='\0' ) {
				const char *str = line;
				const char *delims = ",";
				int k = 0;
				size_t start = 0;
				
				while (k < 2) {
					size_t end = strcspn(str + start, delims);
					Temp_Vector[k] = string(line).substr(start,end);
					start += (str[start + end] != '\0') ? end + 1 : end;
					k++;
				}
				
				output.push_back(Temp_Vector);
			}
			i++;
		}
		fclose(file);
	} else printError("warning", fileinput);
	return output;
}

// Read Arc list and store in a vector of 'Arcs' (Only 'From' and 'To' codes are stored)
vector<Arc> ReadListArcs(const char* fileinput) {
	vector<Arc> output(0);
	char* t_read;
	char line [CHAR_LINE];
	int i = 0;
	
	FILE *file = fopen(fileinput, "r");
	if ( file != NULL ) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if ( fgets(line, sizeof line, file) == NULL ) {
				break;
			}
			// Remove comments and end of line characters
			CleanLine(line);
			
			// Skip first line, then read the rest
			if ( i!=0 && line[0]!='%' && line[0]!='\0' ) {
				Arc Temp_Arc;
				t_read = strtok(line,",");
				Temp_Arc.Set("From", string(t_read));
				t_read = strtok(NULL,",");
				Temp_Arc.Set("To", string(t_read));
				output.push_back(Temp_Arc);
				if ( Temp_Arc.isBidirect() ) {
					Arc Temp_Arc2(Temp_Arc, true);
					output.push_back( Temp_Arc2 );
				}
			}
			i++;
		}
		fclose(file);
	} else { printError("error", fileinput); }
	return output;
}

// Read and create transportation network
void ReadTrans(vector<Node>& Nodes, vector<Arc>& Arcs, const char* fileinput) {
	// Create default nodes and arcs
	vector<string> DefNodes(0), DefFrom(0), DefTo(0), DefInf(0);
	Node TempNode;
	Arc TempArc;
	int NInfra = TransInfra.size(), NFleet = 0, NComm = TransComm.size(), k = 0;
	string Infra = "", Fleet = "", FleetInf = "", Comm = "", Coal = "";
	
	// Determine the number of fleet
	for (unsigned int i = 0; i < NInfra; ++i)
		NFleet += TransInfra[i].size()-1;
	
	vector< vector<bool> > NodeTable(0), ArcTable(0);
	vector<bool> TableColumn(NFleet, false);
	
	// For each line in the definition of infrastructures
	for (unsigned int i = 0; i < NInfra; ++i) {	
		Infra.push_back(TransInfra[i][0]);
		
		// Create an arc (for infrastructure capacity constraints)
		DefFrom.push_back( TransInfra[i].substr(0,1) + TransInfra[i].substr(0,1) );
		DefTo.push_back( "XX" );
		DefInf.push_back( "" );
		ArcTable.push_back(TableColumn);
		
		// For each fleet within that infrastructure
		for (unsigned int j = 1; j < TransInfra[i].size(); ++j) {
			// Create an arc (for fleet capacity constraints)
			DefFrom.push_back( TransInfra[i].substr(j,1) + TransInfra[i].substr(j,1) );
			DefTo.push_back( "XX" );
			DefInf.push_back( "" );
			ArcTable.push_back(TableColumn);
			
			Fleet.push_back(TransInfra[i][j]);
			FleetInf.push_back(TransInfra[i][0]);
			
			ArcTable[ArcTable.size()-1][k] = true;
			ArcTable[ArcTable.size()-j-1][k] = true;
			k++;
		}
	}
	
	// For each commodity
	for (unsigned int i = 0; i < NComm; ++i) {
		Comm.push_back(TransComm[i][0]);
		DefNodes.push_back( TransComm[i].substr(0,1) + "T" );
		NodeTable.push_back(TableColumn);
		
		for (unsigned int j = 1; j < TransComm[i].size(); ++j) {
			// Find for each fleet that the commodity can use
			k = Fleet.find(TransComm[i][j]);
			if (k >= 0) {
				// Create arc
				DefFrom.push_back( Fleet.substr(k,1) + Fleet.substr(k,1) );
				DefTo.push_back( TransComm[i].substr(0,1) + "T" );
				DefInf.push_back( FleetInf.substr(k,1) + FleetInf.substr(k,1) );
				ArcTable.push_back(TableColumn);
				
				ArcTable[ArcTable.size()-1][k] = true;
				NodeTable[NodeTable.size()-1][k] = true;
			}
		}
	}
	
	char* t_read;
	char line [CHAR_LINE];
	int i = 0;
	
	TempNode.Set("Step", TransStep);
	TempArc.Set("FromStep", TransStep);
	TempArc.Set("ToStep", TransStep);
	
	FILE *file = fopen(fileinput, "r");
	if ( file != NULL ) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if ( fgets(line, sizeof line, file) == NULL ) {
				break;
			}
			// Remove comments and end of line characters
			CleanLine(line);
			
			// Skip first line, then read the rest
			if (i!=0) {
				string from, to, fleetlist, swap;
				vector<bool> ShowNode(NodeTable.size(), false), ShowArc(ArcTable.size(), false);
				int kk, swapindex;
				
				// Read from and to codes
				t_read = strtok(line,",");
				from = string(t_read);
				t_read = strtok(NULL,",");
				to = string(t_read);
				
				// Distance
				t_read = strtok(NULL,",");
				TempArc.Set("Distance", string(t_read));
				
				// Read allowed fleet and determine what nodes and arcs will be appropriate
				t_read = strtok(NULL,",");
				if (t_read == '\0') {
					fleetlist = "";
					
					for (unsigned int k1 = 0; k1 < ShowNode.size(); ++k1) ShowNode[k1] = true;
					for (unsigned int k1 = 0; k1 < ShowArc.size(); ++k1) ShowArc[k1] = true;
				} else {
					fleetlist = string(t_read);
					
					for (unsigned int k1 = 0; k1 < ShowNode.size(); ++k1) ShowNode[k1] = false;
					for (unsigned int k1 = 0; k1 < ShowArc.size(); ++k1) ShowArc[k1] = false;
					for (unsigned int k2 = 0; k2 < fleetlist.size(); ++k2) {
						kk = Fleet.find(fleetlist[k2]);
						if (kk >=0) {
							for (unsigned int k1 = 0; k1 < ShowNode.size(); ++k1) ShowNode[k1] = ShowNode[k1] || NodeTable[k1][kk];
							for (unsigned int k1 = 0; k1 < ShowArc.size(); ++k1) ShowArc[k1] = ShowArc[k1] || ArcTable[k1][kk];
						}
					}
				}
				
				// To account for both directions and avoid repetitions
				swapindex = 0;
				
				while ( (swapindex==0) || ((swapindex==1) && (from!=to)) ) {
					for (unsigned int k1 = 0; k1 < ShowNode.size(); ++k1) {
						if (ShowNode[k1]) {
							TempNode.Set("ShortCode", DefNodes[k1] + from + to);
							Nodes.push_back(TempNode);
							
							//Coal to transportation
							if ( (swapindex==0) && (from!=to) && (DefNodes[k1]!=TransDummy) && (DefNodes[k1][1]=='T') ) {
								kk = TransCoal.find(DefNodes[k1][0]);
								if (kk >= 0) {
									// Check if nodes exist, if not it creates it
									bool fromexists = false, toexists = false;
									for (unsigned int k2 = 0; k2 < Nodes.size(); ++k2) {
										if ( Nodes[k2].Get("ShortCode") == DefNodes[k1] + from ) {
											fromexists = true;
										} else if ( Nodes[k2].Get("ShortCode") == DefNodes[k1] + to ) {
											toexists = true;
										}
									}
									if (!fromexists) {
										TempNode.Set("ShortCode", DefNodes[k1] + from);
										Nodes.push_back(TempNode);
									}
									if (!toexists) {
										TempNode.Set("ShortCode", DefNodes[k1] + to);
										Nodes.push_back(TempNode);
									}
									
									// Create arcs for coal transportation
									TempArc.Set("Energy2Trans", true);
									TempArc.Set("From", DefNodes[k1] + from);
									TempArc.Set("To", DefNodes[k1] + to);
									Arcs.push_back(TempArc);
									TempArc.Set("From", DefNodes[k1] + to);
									TempArc.Set("To", DefNodes[k1] + from);
									Arcs.push_back(TempArc);
									TempArc.Set("Energy2Trans", false);
								}
							}
						}
					}
					
					for (unsigned int k1 = 0; k1 < ShowArc.size(); ++k1) {
						if (ShowArc[k1]) {
							TempArc.Set("From", DefFrom[k1] + from + to);
							TempArc.Set("To", DefTo[k1] + from + to);
							TempArc.Set("TransInfr", DefInf[k1]);
							Arcs.push_back(TempArc);
						}
					}
					swap = from;
					from = to;
					to = swap;
					swapindex++;
				}
			}
			i++;
		}
		fclose(file);
	} else { printError("error", fileinput); }
}

// Read step lengths for capacitated arcs
void ReadEvents(double output[], const char* fileinput) {
	char* t_read;
	char line [CHAR_LINE];
	int i=0;
	
	FILE *file = fopen(fileinput, "r");
	if ( file != NULL ) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if ( fgets(line, sizeof line, file) == NULL ) {
				break;
			}
			// Remove comments and end of line characters
			CleanLine(line);
			t_read = strtok(line,",");
			output[i] = atof(t_read);
			++i;
			for (int j=1; j<=Nevents; ++j) {
				t_read = strtok(NULL,",");
				output[i] = atof(t_read);
				++i;
			}
		}
		fclose(file);
	} else printError("error", fileinput);
}
