// --------------------------------------------------------------
//    NETSCORE Version 2
//    read.cpp -- Implementation of reading functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#include "global.h"
#include "read.h"

#define CHAR_LINE 15000

// Read global parameters
void ReadParameters(const char* fileinput, GlobalParam *p) {
	char* t_read;
	string prop, value, discount = "0", inflation = "0", demandrate = "0", peakdemandrate = "0", step = "";
	char line[CHAR_LINE];
	
	FILE *file = fopen(fileinput, "r");
	if (file != NULL) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if (fgets(line, sizeof line, file) == NULL)
				break;
			
			// Remove comments and end of line characters
			CleanLine(line);
			
			if ((line[0] != '$') && (line[0] != '/') && (line[0] != '#') && (line[0] != '%')) {
				t_read = strtok(line,",");
				prop = string(t_read);
				t_read = strtok(NULL,",");
				value = string(t_read);
				
				// Apply read item
				if (prop == "StepName") SName = value;
				else if (prop == "StepLength") {
					SLength = Str2Step(value);
					step = value;
				} else if (prop == "StepHours") StepHours.push_back(value);
				else if (prop == "UseDCFlow") useDCflow = (value == "true" || value == "True" || value == "TRUE");
				else if (prop == "OutputLevel") outputLevel = atoi(value.c_str());
				else if (prop == "CodeDC") DCCode = value;
				else if (prop == "DefStep") {
					p->DefStep = value;
					if (p->TransStep == "")
						p->TransStep = value;
				} else if (prop == "DefDiscount") discount = value;
				else if (prop == "DefInflation") inflation = value;
				else if (prop == "DefDemandRate") demandrate = value;
				// Transportation parameters
				else if (prop == "TransStep") p->TransStep = value;
				else if (prop == "TransInfra") p->TransInfra.push_back(value);
				else if (prop == "TransComm") p->TransComm.push_back(value);
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
	
	// Initialize time steps
	p->s = new GlobalStep(step, StepHours);
	
	// Calculate how many hours are there for each step and store it in StepHours
	int laststep = SLength[SName.size() - 1], temp_hour = 0;
	if (StepHours.size() == 0) {
		for (int i = 0; i < laststep; ++i)
			StepHours.push_back("1");
		temp_hour = laststep;
	} else if (StepHours.size() == laststep) {
		for (int i = 0; i < laststep; ++i)
			temp_hour += atoi(StepHours[i].c_str());
	} else if (StepHours.size() > 1) {
		printError("parameter", string("StepHours"));
	} else {
		for (int i = 1; i < StepHours.size(); ++i)
			StepHours[i] = StepHours[0];
		for (int i = StepHours.size(); i < laststep; ++i)
			StepHours.push_back(StepHours[0]);
		temp_hour = laststep * atoi(StepHours[0].c_str());
	}
	
	for (int j = SName.size()-2; j >= 0; --j) {
		StepHours.insert(StepHours.begin(), ToString<int>(temp_hour));
		temp_hour = temp_hour * SLength[j];
	}
	
	// Number of objectives
	Nobj = 1 + SustObj.size();
	if (Nevents > 0)
		++Nobj;
	SustMet.insert(SustMet.begin(), SustObj.begin(), SustObj.end());
	
	// Declare a vector with the node property codes
	p->NodeProp[N_Code] = "Code";
	p->NodeProp[N_Step] = "Step";
	p->NodeProp[N_StepLength] = "StepLength";
	p->NodeProp[N_Demand] = "Demand";
	p->NodeProp[N_DemandPower] = "DemandPower";
	p->NodeProp[N_DemandRate] = "DemandRate";
	p->NodeProp[N_PeakPower] = "PeakPower";
	p->NodeProp[N_PeakPowerRate] = "PeakPowerRate";
	p->NodeProp[N_CostUD] = "CostUD";
	p->NodeProp[N_Discount_Rate] = "DiscountRate";
	p->NodeProp[N_InflationRate] = "InflationRate";
	
	// Default node properties
	p->NodeDefault[N_Code] = "X";
	p->NodeDefault[N_Step] = p->DefStep;
	p->NodeDefault[N_StepLength] = "X";
	p->NodeDefault[N_Demand] = "0";
	p->NodeDefault[N_DemandPower] = "X";
	p->NodeDefault[N_DemandRate] = demandrate;
	p->NodeDefault[N_PeakPower] = "X";
	p->NodeDefault[N_PeakPowerRate] = peakdemandrate;
	p->NodeDefault[N_CostUD] = "X";
	p->NodeDefault[N_Discount_Rate] = discount;
	p->NodeDefault[N_InflationRate] = inflation;
	
	// Declare a vector with the arc property codes
	string startzero = SName.substr(0,1) + "2";
	ArcProp.push_back("Code"); p->ArcDefault.push_back("X");
	ArcProp.push_back("From"); p->ArcDefault.push_back("X");
	ArcProp.push_back("To"); p->ArcDefault.push_back("X");
	ArcProp.push_back("FromStep"); p->ArcDefault.push_back(p->DefStep);
	ArcProp.push_back("ToStep"); p->ArcDefault.push_back(p->DefStep);
	ArcProp.push_back("StepLength"); p->ArcDefault.push_back("X");
	ArcProp.push_back("InvStep"); p->ArcDefault.push_back(SName.substr(0,1));
	ArcProp.push_back("TransInfr"); p->ArcDefault.push_back("");
	ArcProp.push_back("OpCost"); p->ArcDefault.push_back("0");
	ArcProp.push_back("InvCost"); p->ArcDefault.push_back("X");
	ArcProp.push_back("DiscountRate"); p->ArcDefault.push_back(discount);
	ArcProp.push_back("InflationRate"); p->ArcDefault.push_back(inflation);
	ArcProp.push_back("Distance"); p->ArcDefault.push_back("X");
	ArcProp.push_back("OpMin"); p->ArcDefault.push_back("0");
	ArcProp.push_back("OpMax"); p->ArcDefault.push_back("Inf");
	ArcProp.push_back("InvMin"); p->ArcDefault.push_back("0");
	ArcProp.push_back("InvMax"); p->ArcDefault.push_back("Inf");
	ArcProp.push_back("InvStart"); p->ArcDefault.push_back(startzero);
	ArcProp.push_back("LifeSpan"); p->ArcDefault.push_back("X");
	ArcProp.push_back("Eff"); p->ArcDefault.push_back("1");
	ArcProp.push_back("InvertEff"); p->ArcDefault.push_back("N");
	ArcProp.push_back("Suscep"); p->ArcDefault.push_back("X");
	ArcProp.push_back("CapacityFactor"); p->ArcDefault.push_back("0");
	// Sustainability
	for (int j = 0; j < SustMet.size(); ++j) {
		ArcProp.push_back("Op" + SustMet[j]);
		p->ArcDefault.push_back("0");
	}
	
	// Resiliency events
	for (int j = 1; j <= Nevents; ++j) {
		ArcProp.push_back("CapacityLoss" + ToString<int>(j));
		p->ArcDefault.push_back("1");
	}
	
	ArcPropOffset = 8;
}

// Read properties file and store it in a matrix of strings.
// The first 'num_fields' columns are copied and not touched.
// The rest of the columns correspond to the different 'Steps' as determined by the function 'Step2Pos'
MatrixStr ReadProperties(const char* fileinput, const string& defvalue, const int num_fields, GlobalStep *s) {
	VectorStr Values(s->MaxPos + num_fields + 1, defvalue);
	vector<int> Header(0);
	MatrixStr output(0);
	char* t_read;
	string t2_read;
	char line[CHAR_LINE];
	int i = 0, j = 0;
	
	FILE *file = fopen(fileinput, "r");
	if (file != NULL) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if (fgets(line, sizeof line, file) == NULL)
				break;
			
			// Remove comments and end of line characters
			CleanLine(line);
			
			// Avoid empty line or with comments
			if ((line[0] != '%') && (line[0] != '\0')) {
				if (i == 0) {
					// Read column headers and store them
					++i;
					t_read = strtok(line, ",");
					
					while (t_read != NULL) {
						// Skip first 'num_fields' columns
						if (j >= num_fields)
							Header.push_back(s->Str2Pos(t_read));
						
						t_read = strtok(NULL, ",");
						++j;
					}
					
					// First row contains just default value
					for (int m = 0; m < num_fields; ++m)
						Values[m] = "";
					output.push_back(Values);
				} else {
					// A line of properties is stored here
					const char *str = line, *delims = ",";
					int k = 0;
					
					// Default values
					Values = VectorStr(s->MaxPos + 1 + num_fields, defvalue);
					
					size_t start = 0;
					
					while (str[start] != '\0') {
						size_t end = strcspn(str + start, delims);
						t2_read = string(line).substr(start, end);
						
						if (k < num_fields) {
							// Row names
							Values[k] = t2_read;
						} else if (end != 0) {
							// Values
							int initialPos = Header[k - num_fields];
							if (initialPos == 0) {
								// 'const' value
								for (int m = num_fields; m < Values.size(); ++m)
									Values[m] = t2_read;
							} else if (initialPos < 0) {
								// Step is smaller than a year (to repeat monthly data, etc.) ///////////////////////////////////////////////
								int pos = 1;
								while (initialPos <= 0) {
									initialPos += 2 * (s->Next[pos] - pos);
									++pos;
								}
								--pos;
								initialPos -= pos;
								
								if (Values[0] == "NSAL" && num_fields == 1)
									for (int a = 0; a < Header.size(); ++a)
										cout << " " << Header[a] << endl;
								
								while (pos <= s->MaxPos) {
									if (Values[0] == "NTCO" && num_fields == 1)
										cout << Values[0] << " " << s->Text[pos+initialPos] << " " << t2_read << endl;
									
									Values[pos + initialPos + num_fields] = t2_read;
									pos = s->Next[pos];
								}
							} else if (initialPos > 0) {
								// Step with year
								int a = initialPos + num_fields;
								int b = s->Next[initialPos] + num_fields;
								if (b > Values.size())
									b = Values.size();
								for (int m = a; m < b; ++m)
									Values[m] = t2_read;
							}
						}
						start += (str[start + end] != '\0') ? end + 1 : end;
						++k;
					}
					output.push_back(Values);
				}
			}
		}
		fclose(file);
	} else
		printError("warning", fileinput);
	
	return output;
}

// Read Node list and store it in a vector of 'Nodes' (only 'ShortCode' is stored)
vector<Node> ReadListNodes(const char* fileinput, GlobalParam *p) {
	vector<Node> output(0, Node(p));
	char* t_read;
	char line[CHAR_LINE];
	int i = 0;
	
	FILE *file = fopen(fileinput, "r");
	if (file != NULL) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if (fgets(line, sizeof line, file) == NULL)
				break;
			
			// Remove comments and end of line characters
			CleanLine(line);
			
			// Skip first line
			if ((i!=0 && line[0]!='%') && (line[0]!='\0')) {
				Node Temp_Node(p);
				t_read = strtok(line,",");
				Temp_Node.Set(N_ShortCode, string(t_read));
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
	char line[CHAR_LINE];
	int i = 0;

	FILE *file = fopen(fileinput, "r");
	if (file != NULL) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if (fgets(line, sizeof line, file) == NULL)
				break;
			
			// Remove newline character
			CleanLine(line);
			
			// Skip first line, then read the rest
			if ((i!=0) && (line[0]!='%') && (line[0]!='\0')) {
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
vector<Arc> ReadListArcs(const char* fileinput, GlobalParam *p) {
	vector<Arc> output(0, Arc(p));
	char* t_read;
	char line[CHAR_LINE];
	int i = 0;
	
	FILE *file = fopen(fileinput, "r");
	if (file != NULL) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if (fgets(line, sizeof line, file) == NULL)
				break;
			
			// Remove comments and end of line characters
			CleanLine(line);
			
			// Skip first line, then read the rest
			if ((i!=0) && (line[0]!='%') && (line[0]!='\0')) {
				Arc Temp_Arc(p);
				t_read = strtok(line,",");
				Temp_Arc.Set("From", string(t_read));
				t_read = strtok(NULL,",");
				Temp_Arc.Set("To", string(t_read));
				output.push_back(Temp_Arc);
				if (Temp_Arc.isBidirect()) {
					Arc Temp_Arc2(Temp_Arc, true);
					output.push_back(Temp_Arc2);
				}
			}
			i++;
		}
		fclose(file);
	} else { printError("error", fileinput); }
	return output;
}

// Read and create transportation network
void ReadTrans(vector<Node>& Nodes, vector<Arc>& Arcs, const char* fileinput, GlobalParam *p) {
	// Create default nodes and arcs
	vector<string> DefNodes(0), DefFrom(0), DefTo(0), DefInf(0);
	Node TempNode(p);
	Arc TempArc(p);
	int NInfra = p->TransInfra.size(), NFleet = 0, NComm = p->TransComm.size(), k = 0;
	string Infra = "", Fleet = "", FleetInf = "", Comm = "", Coal = "";
	
	// Determine the number of fleet
	for (unsigned int i = 0; i < NInfra; ++i)
		NFleet += p->TransInfra[i].size() - 1;
	
	vector< vector<bool> > NodeTable(0), ArcTable(0);
	vector<bool> TableColumn(NFleet, false);
	
	// For each line in the definition of infrastructures
	for (unsigned int i = 0; i < NInfra; ++i) {
		Infra.push_back(p->TransInfra[i][0]);
		
		// Create an arc (for infrastructure capacity constraints)
		DefFrom.push_back(p->TransInfra[i].substr(0, 1) + p->TransInfra[i].substr(0, 1));
		DefTo.push_back("XX");
		DefInf.push_back("");
		ArcTable.push_back(TableColumn);
		
		// For each fleet within that infrastructure
		for (unsigned int j = 1; j < p->TransInfra[i].size(); ++j) {
			// Create an arc (for fleet capacity constraints)
			DefFrom.push_back(p->TransInfra[i].substr(j,1) + p->TransInfra[i].substr(j,1));
			DefTo.push_back("XX");
			DefInf.push_back("");
			ArcTable.push_back(TableColumn);
			
			Fleet.push_back(p->TransInfra[i][j]);
			FleetInf.push_back(p->TransInfra[i][0]);
			
			ArcTable[ArcTable.size()-1][k] = true;
			ArcTable[ArcTable.size()-j-1][k] = true;
			k++;
		}
	}
	
	// For each commodity
	for (unsigned int i = 0; i < NComm; ++i) {
		Comm.push_back(p->TransComm[i][0]);
		DefNodes.push_back(p->TransComm[i].substr(0, 1) + "T");
		NodeTable.push_back(TableColumn);
		
		for (unsigned int j = 1; j < p->TransComm[i].size(); ++j) {
			// Find for each fleet that the commodity can use
			k = Fleet.find(p->TransComm[i][j]);
			if (k >= 0) {
				// Create arc
				DefFrom.push_back(Fleet.substr(k, 1) + Fleet.substr(k,1));
				DefTo.push_back(p->TransComm[i].substr(0, 1) + "T");
				DefInf.push_back(FleetInf.substr(k, 1) + FleetInf.substr(k, 1));
				ArcTable.push_back(TableColumn);
				
				ArcTable[ArcTable.size()-1][k] = true;
				NodeTable[NodeTable.size()-1][k] = true;
			}
		}
	}
	
	char* t_read;
	char line[CHAR_LINE];
	int i = 0;
	
	TempNode.Set(N_Step, p->TransStep);
	TempArc.Set("FromStep", p->TransStep);
	TempArc.Set("ToStep", p->TransStep);
	
	FILE *file = fopen(fileinput, "r");
	if (file != NULL) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if (fgets(line, sizeof line, file) == NULL)
				break;
			
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
				
				while ((swapindex==0) || ((swapindex==1) && (from!=to))) {
					for (unsigned int k1 = 0; k1 < ShowNode.size(); ++k1) {
						if (ShowNode[k1]) {
							TempNode.Set(N_ShortCode, DefNodes[k1] + from + to);
							Nodes.push_back(TempNode);
							
							// Coal to transportation
							if ((swapindex == 0) && (from != to) && (DefNodes[k1] != p->TransDummy) && (DefNodes[k1][1] == 'T')) {
								kk = TransCoal.find(DefNodes[k1][0]);
								if (kk >= 0) {
									// Check if nodes exist, if not it creates it
									bool fromexists = false, toexists = false;
									for (unsigned int k2 = 0; k2 < Nodes.size(); ++k2) {
										if (Nodes[k2].Get(N_ShortCode) == DefNodes[k1] + from)
											fromexists = true;
										else if (Nodes[k2].Get(N_ShortCode) == DefNodes[k1] + to)
											toexists = true;
									}
									if (!fromexists) {
										TempNode.Set(N_ShortCode, DefNodes[k1] + from);
										Nodes.push_back(TempNode);
									}
									if (!toexists) {
										TempNode.Set(N_ShortCode, DefNodes[k1] + to);
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
	char line[CHAR_LINE];
	int i = 0;
	
	FILE *file = fopen(fileinput, "r");
	if (file != NULL) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if (fgets(line, sizeof line, file) == NULL)
				break;
			
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
