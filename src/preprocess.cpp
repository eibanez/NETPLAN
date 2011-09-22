// --------------------------------------------------------------
//    NETSCORE Version 2
//    preprocess.cpp -- Implementation of preprocessor
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#include <sys/stat.h>
#include "netscore.h"

int main() {
	printHeader(H_Prep);
	
	cout << "- Reading global parameters...\n";
	ReadParameters("data/parameters.csv");
	
	// Create folders to store files if it doesn't exist
	mkdir("./prepdata", 0777);
	mkdir("./nsgadata", 0777);
	mkdir("./bestdata", 0777);
	
	// Variables to store information
	vector<Node> ListNodes(0), Nodes(0), ListDCNodes(0);
	vector<Arc> ListArcs(0), Arcs(0), ListDCArcs(0);
	MatrixStr NStep(0), ATransEnergy(0), SustLimits(0);
	vector<MatrixStr> NVectorProp(0), AVectorProp(0);
	vector<int> NVectorIndex(NodeProp.size()-NodePropOffset, -1), AVectorIndex(ArcProp.size()-ArcPropOffset, -1);
	
	cout << "- Reading list of nodes...\n";
	ListNodes = ReadListNodes("data/nodes_List.csv");
	cout << "- Reading node data...\n";
	NStep = ReadStep("data/nodes_Step.csv");
	for (unsigned int t=NodePropOffset; t < NodeProp.size(); ++t) {
		string file_name = "data/nodes_" + NodeProp[t] + ".csv";
		NVectorProp.push_back(ReadProperties(file_name.c_str(), NodeDefault[t], 1));
	}
	
	cout << "- Reading list of arcs...\n";
	ListArcs = ReadListArcs("data/arcs_List.csv");
	cout << "- Reading arc data...\n";
	ATransEnergy = ReadProperties("data/arcs_TransEnergy.csv", "X", 2);
	for (unsigned int t=ArcPropOffset; t < ArcProp.size(); ++t) {
		string file_name;
		if (t < ArcProp.size() - Nevents) {
			// Regular properties
			file_name = "data/arcs_" + ArcProp[t] + ".csv";
		} else {
			// Resiliency properties
			file_name = "data/events/" + ArcProp[t] + ".csv";
		}
		AVectorProp.push_back(ReadProperties(file_name.c_str(), ArcDefault[t], 2));
	}
	
	cout << "- Creating transportation network...\n";
	ReadTrans(ListNodes, ListArcs, "data/trans_List.csv");
	
	cout << "- Reading sustainability constraints...\n";
	SustLimits = ReadProperties("data/sust_Limits.csv", "X", 1);
	
	// Expand nodes
	for (unsigned int k=0; k < ListNodes.size(); ++k) {
		// Print progress
		cout << "\r- Expanding nodes... " << k+1 << " / " << ListNodes.size() << flush;
		
		int StepIndex = FindCode(ListNodes[k], NStep);
		
		// Identify the row containing data for each property
		for (unsigned int t=0; t < NVectorIndex.size(); ++t)
			NVectorIndex[t] = FindCode(ListNodes[k], NVectorProp[t]);
		
		// Copy step information
		if (StepIndex >= 0) ListNodes[k].Set("Step", NStep[StepIndex][1]);
		
		if (ListNodes[k].Get("Step") == "") {
			printError("nodestep", ListNodes[k].Get("ShortCode"));
		} else {
			// Use a temporary node to store information and cycle through steps
			Step TempStep(SName.size(), 0);
			for (unsigned int l = 0; l < ListNodes[k].Get("Step").size(); l++) TempStep[l] = 1;
			
			while (TempStep <= SLength) {
				// Apply information
				Node TempNode = ListNodes[k];
				TempNode.Set("Step", Step2Str(TempStep));
				TempNode.Set("StepLength", Step2Hours(TempStep));
				int l = Step2Pos(TempStep) + 1;
				TempNode.Set("Code", TempNode.Get("ShortCode") + Step2Str(TempStep));
				
				for (unsigned int t=0; t < NVectorIndex.size(); ++t) {
					int tmp_index = NVectorIndex[t];
					if (tmp_index >= 0) TempNode.Set(NodeProp[NodePropOffset + t], NVectorProp[t][tmp_index][l]);
				}
				
				// Calculate demand if power demand is given
				if ((TempNode.Get("Demand") == "0") && (TempNode.Get("DemandPower") != "X")) {
					double step_length = TempNode.GetDouble("StepLength");
					TempNode.Multiply("DemandPower", step_length);
					TempNode.Set("Demand", TempNode.Get("DemandPower"));
				}
				
				// Adjust peak demand with increase rate
				double dem_rate = TempNode.GetDouble("DemandRate");
				double peak_rate = TempNode.GetDouble("PeakPowerRate");
				double dem_factor = 1, peak_factor = 1;
				
				if ((dem_rate != 0) || (peak_rate != 0)) {
					for (unsigned int l = 1; l < TempStep[0]; ++l) {
						dem_factor = dem_factor * (1 + dem_rate);
						peak_factor = peak_factor * (1 + peak_rate);
					}
					TempNode.Multiply("Demand", dem_factor);
					TempNode.Multiply("PeakPower", peak_factor);
				}
				
				// Store node for later use
				Nodes.push_back(TempNode);
				if (TempNode.isDCflow()) {
					ListDCNodes.push_back(TempNode);
					IdxDc.Add(k, TempStep, TempNode.Get("ShortCode"));
				}
				
				// Record indices to recover information
				IdxNode.Add(k, TempStep, TempNode.Get("ShortCode"));
				if (TempNode.Get("CostUD") != "X") {
					IdxUd.Add(k, TempStep, TempNode.Get("ShortCode"));
				}
				if ((TempNode.Get("PeakPower") != "X") && TempNode.isFirstinYear()) {
					Step temp2(SName.size(), 0);
					temp2[0] = TempStep[0];
					IdxRm.Add(k, temp2, TempNode.Get("ShortCode"));
				}
				
				// Move to the next step
				TempStep = NextStep(TempStep);
			}
		}
	}
	
	
	// Expand arcs
	cout << endl;
	for (unsigned int k = 0; k < ListArcs.size(); ++k) {
		// Print progress
		cout << "\r- Expanding arcs... " << k+1 << " / " << ListArcs.size() << flush;
		
		// Identify the row containing data for each property
		int StepFromIndex = FindCode(ListArcs[k].Get("From"),NStep);
		int StepToIndex = FindCode(ListArcs[k].Get("To"),NStep);
		int TransEnergyIndex = FindCode(ListArcs[k].Get("From"), ATransEnergy);
		for (unsigned int t=0; t < AVectorIndex.size(); ++t) {
			AVectorIndex[t] = FindCode(ListArcs[k], AVectorProp[t]);
		}
		
		// Recover step information
		if (StepFromIndex >= 0) ListArcs[k].Set("FromStep", NStep[StepFromIndex][1]);
		if (StepToIndex >= 0) ListArcs[k].Set("ToStep", NStep[StepToIndex][1]);
		
		// Check for a storage arc
		bool isStorage = ListArcs[k].isStorage();
		
		if ((ListArcs[k].Get("FromStep") == "") && (ListArcs[k].Get("ToStep") == "")) {
			printError("arcstep", ListArcs[k].Get("From") + "_" + ListArcs[k].Get("To"));
		} else {
			// Cycle through steps (more complicated here) to expand arcs
			string TempArcStepCode = max(ListArcs[k].Get("FromStep"), ListArcs[k].Get("ToStep"));
			
			Step TempStep(SName.size()), TempFromStep(SName.size(), 0), TempToStep(SName.size(), 0);
			Step NextFromStep(SName.size()), NextToStep(SName.size());
			
			for (unsigned int l = 0; l < ListArcs[k].Get("FromStep").size(); l++) TempFromStep[l] = 1;
			for (unsigned int l = ListArcs[k].Get("FromStep").size(); l < SName.size(); l++) TempFromStep[l] = 0;
			
			if (isStorage) {
				TempToStep = NextStep(TempFromStep);
			} else {
				for (unsigned int l = 0; l < ListArcs[k].Get("ToStep").size(); l++) TempToStep[l] = 1;
				for (unsigned int l = ListArcs[k].Get("ToStep").size(); l < SName.size(); l++) TempToStep[l] = 0;
			}
			
			NextFromStep = (TempFromStep[0] == 1) ? NextStep(TempFromStep) : NextStep(SLength);
			NextToStep = (TempToStep[0] == 1) ? NextStep(TempToStep) : NextStep(SLength);
			
			TempStep = ((TempFromStep < TempToStep) && !isStorage) ? TempToStep : TempFromStep;
			
			// Find the shortest step, to assign it as a default for 'InvStep'
			string TempStepStr = (TempFromStep < TempToStep) ? ListArcs[k].Get("ToStep") : ListArcs[k].Get("FromStep");
			
			while ((TempStep <= SLength) && (TempToStep <= SLength)) {
				// Apply information
				Arc TempArc = ListArcs[k];
				int l = Step2Pos(TempStep) + 2;
				TempArc.Set("FromStep", Step2Str(TempFromStep));
				TempArc.Set("ToStep", Step2Str(TempToStep));
				TempArc.Set("StepLength", Step2Hours(TempStep));
				
				if (TempArc.isTransport() && (TempArc.Get("TransInfr") == ""))
					TempArc.Set("Code", TempArc.Get("From") + Step2Str(TempFromStep));
				else
					TempArc.Set("Code", TempArc.Get("From") + Step2Str(TempFromStep) + "_" + TempArc.Get("To") + Step2Str(TempToStep));
				
				for (unsigned int t=0; t < AVectorIndex.size(); ++t) {
					int tmp_index = AVectorIndex[t];
					if (tmp_index >= 0) TempArc.Set(ArcProp[ArcPropOffset + t], AVectorProp[t][tmp_index][l]);
				}
				
				// Is there a load on the an energy node?
				bool isTrans2Energy = (TransEnergyIndex >= 0);
				int IndexTemp = TransEnergyIndex;
				while (isTrans2Energy) {
					// Read code and step for energy node
					string LoadCode = ATransEnergy[IndexTemp][1];
					int LoadIndex = FindCode(LoadCode, NStep);
					string LoadStepCode = DefStep;
					if (LoadIndex >= 0) LoadStepCode = NStep[LoadIndex][1];
					
					if (LoadStepCode.size() <= TempArcStepCode.size()) {
						Step LoadStep = TempStep;
						for (unsigned int m = LoadStepCode.size(); m < LoadStep.size(); m++) LoadStep[m] = 0;
						int l2 = Step2Pos(LoadStep) + 2;
						TempArc.Add("Trans2Energy", LoadCode + Step2Str(LoadStep));
						TempArc.Add("Trans2Energy", ATransEnergy[IndexTemp][l2]);
					} else {
						Step NextTempStep = NextStep(TempStep);
						Step LoadStep = TempStep;
						for (unsigned int m = TempArcStepCode.size(); m < LoadStepCode.size(); m++) LoadStep[m] = 1;
						while (LoadStep < NextTempStep) {
							int l2 = Step2Pos(LoadStep) + 2;
							TempArc.Add("Trans2Energy", LoadCode + Step2Str(LoadStep));
							TempArc.Add("Trans2Energy", ATransEnergy[IndexTemp][l2]);
							LoadStep = NextStep(LoadStep);
						}
					}
					
					IndexTemp++;
					if (IndexTemp >= ATransEnergy.size()) {
						isTrans2Energy = false;
					} else {
						isTrans2Energy = ListArcs[k].Get("From") == ATransEnergy[IndexTemp][0];
					}
				}
				
				// From Investment cost, retirement cost, Fixed O&M, discount rate, lifetime  ==> Overnight cost
				// Feature is planned but not implemented yet
				
				// Apply discount and inflation rate to investment and operational costs
				double factor = (1 + TempArc.GetDouble("InflationRate")) / (1 + TempArc.GetDouble("DiscountRate"));
				double dollar_factor = 1;
				
				double inv_cost = TempArc.GetDouble("InvCost");
				double op_cost = TempArc.GetDouble("OpCost");
				
				if ((factor != 1) && ((inv_cost != 0) || (op_cost != 0))) {
					for (int l = 1; l < TempStep[0]; ++l)
						dollar_factor = dollar_factor * factor;
				}
				
				// If distance is available adjust costs, emissions, demand for energy...
				if (TempArc.Get("Distance") != "X") {
					double distance = TempArc.GetDouble("Distance");
					dollar_factor = dollar_factor * distance;
					
					for (int j = 0; j < SustMet.size(); ++j)
						TempArc.Multiply("Op" + SustMet[j], distance);
					TempArc.Multiply("Trans2Energy", distance);
				}
				
				if (dollar_factor != 1)
					TempArc.Multiply("OpCost", dollar_factor);
				
				// Need to adjust for investment costs at the end of the simulation period
				string life_span = TempArc.Get("LifeSpan");
				if (life_span != "X") {
					int years_left = (SLength[0] + 1) - TempStep[0];
					int life_inv = Str2Step(life_span)[0];
					if (years_left < life_inv)  dollar_factor = dollar_factor * years_left / life_inv;
				}
				
				// Store modified investment costs
				if (dollar_factor != 1)
					TempArc.Multiply("InvCost", dollar_factor);
				
				// Store Arc for later use
				Arcs.push_back(TempArc);
				if (TempArc.isDCflow())
					ListDCArcs.push_back(TempArc);
				
				// Store Arc indices to recover solution information
				if (!TempArc.isTransport() || TempArc.Get("TransInfr") != "") {
					IdxArc.Add(k, TempStep, TempArc.Get("From") + "_" + TempArc.Get("To"));
				}
				if (TempArc.InvArc()  && TempArc.Get("TransInfr") == "") {
					Step YearStep(SName.size(), 0);
					YearStep[0] = TempStep[0];
					IdxInv.Add(k, YearStep, TempArc.Get("From") + "_" + TempArc.Get("To"));
					if (TempArc.Get("InvMax") != "Inf")
						IdxNsga.Add(k, YearStep, TempArc.Get("From") + "_" + TempArc.Get("To"));
				}
				if (TempArc.Get("OpMax") != "Inf"  && TempArc.Get("TransInfr") == "") {
					IdxUb.Add(k, TempStep, TempArc.Get("From") + "_" + TempArc.Get("To"));
					if (TempArc.isFirstinYear()) {
						Step temp2(SName.size(), 0);
						temp2[0] = TempStep[0];
						IdxCap.Add(k, temp2, TempArc.Get("From") + "_" + TempArc.Get("To"));
					}
				}
				
				// Move to next time step
				TempStep = NextStep(TempStep);
				if (NextFromStep <= TempStep) {
					TempFromStep = NextFromStep;
					NextFromStep = NextStep(NextFromStep);
				}
				
				if (isStorage) {
					TempToStep = NextToStep;
					NextToStep = NextStep(NextToStep);
					// This part of the code eliminates storage connection between different years (interferes with Benders decomposition)
					// Must have a negative demand on the storage node for the first step in the year and a positive for the last
					if (TempFromStep[0] != TempToStep[0]) {
						TempStep = NextStep(TempStep);
						TempFromStep = NextFromStep;
						TempToStep = NextToStep;
						NextFromStep = NextStep(NextFromStep);
						NextToStep = NextStep(NextToStep);
					}
				} else if (NextToStep <= TempStep) {
					TempToStep = NextToStep;
					NextToStep = NextStep(NextToStep);
				}
			}
		}
	}
	
	// Save index for sustainability metrics
	for (int j = 0; j < SustMet.size(); ++j)
		for (int i = 1; i <= SLength[0]; ++i)
			IdxEm.Add(j, i-1, i, SustMet[j]);
	
	
	cout << endl << "- Writing MPS files..." << endl;
	int nyears = SLength[0];
	ofstream afile, bfile;
	string Ychar = SName.substr(0,1), temp_string;
	
	// afile stores one single MPS file (no Benders)
	// myfile stores the Benders decomposition
	afile.open("prepdata/netscore.mps");
	bfile.open("prepdata/netscore-op.mps");
	
	// NAME and ROWS and Cost objective funtion
	afile << "NAME" << endl;
	afile << "ROWS" << endl;
	afile << " N obj" << endl;
	bfile << "NAME" << endl;
	bfile << "ROWS" << endl;
	bfile << " N obj" << endl;
	
	// Sustainability metrics (rows)
	for (int j = 0; j < SustMet.size(); ++j) {
		for (int i = 1; i <= nyears; ++i) {
			afile << " E " << SustMet[j] << Ychar << i << endl;
			bfile << " E " << SustMet[j] << Ychar << i << endl;
		}
	}
	
	// Peak load
	for (unsigned int i = 0; i < Nodes.size(); ++i) {
		temp_string = Nodes[i].NodePeakRows();
		afile << temp_string;
	}
	
	// Nodal demand constraints
	for (unsigned int i = 0; i < Nodes.size(); ++i) {
		temp_string = Nodes[i].NodeNames();
		afile << temp_string;
		bfile << temp_string;
	}
	
	// Upper bound constraints rows
	for (unsigned int i = 0; i < Arcs.size(); ++i) {
		temp_string = Arcs[i].ArcUbNames();
		afile << temp_string;
		bfile << temp_string;
	}
	
	// "inv2cap" constraints
	for (unsigned int i = 0; i < Arcs.size(); ++i) {
		temp_string = Arcs[i].ArcCapNames();
		afile << temp_string;
	}
	
	// DC Power flow constraints
	for (unsigned int i = 0; i < Arcs.size(); ++i) {
		temp_string = Arcs[i].ArcDcNames();
		afile << temp_string;
		bfile << temp_string;
	}
	
	// COLUMNS (Variables)
	afile << "COLUMNS" << endl;
	bfile << "COLUMNS" << endl;
	
	// Capacities  (these vary slightly for Benders)
	for (unsigned int i = 0; i < Arcs.size(); ++i) {
		afile << Arcs[i].CapArcColumns(0);
		bfile << Arcs[i].CapArcColumns(2);
	}
	
	// Investments
	for (unsigned int i = 0; i < Arcs.size(); ++i) {
		temp_string = Arcs[i].InvArcColumns();
		afile << temp_string;
	}
	
	// Sustainability metrics
	for (int j = 0; j < SustMet.size(); ++j) {
		for (int i = 1; i <= nyears; ++i) {
			afile << "    " << SustMet[j] << "_" << Ychar << i << " " << SustMet[j] << Ychar << i << " -1" << endl;
			bfile << "    " << SustMet[j] << "_" << Ychar << i << " " << SustMet[j] << Ychar << i << " -1" << endl;
		}
	}
	
	// Reserve margin
	for (unsigned int i = 0; i < Nodes.size(); ++i) {
		temp_string = Nodes[i].NodeRMColumns();
		afile << temp_string;
	}
	
	// Flows
	for (unsigned int i = 0; i < Arcs.size(); ++i) {
		temp_string = Arcs[i].ArcColumns();
		afile << temp_string;
		bfile << temp_string;
	}
	
	// Unserved demands
	for (unsigned int i = 0; i < Nodes.size(); ++i) {
		temp_string = Nodes[i].NodeUDColumns();
		afile << temp_string;
		bfile << temp_string;
	}
	
	// Power flow variables (angles)
	vector<string> DcOutput = DCFlowColumns(ListDCNodes, ListDCArcs);
	for (unsigned int i = 1; i < DcOutput.size(); ++i) {
		afile << DcOutput[i];
		bfile << DcOutput[i];
	}
	
	// RHS
	afile << "RHS" << endl;
	bfile << "RHS" << endl;
	
	// Nodal Demands
	for (unsigned int i = 0; i < Nodes.size(); ++i) {
		temp_string = Nodes[i].NodeRhs();
		afile << temp_string;
		bfile << temp_string;
	}
	
	// Initial capacity terms
	for (unsigned int i = 0; i < Arcs.size(); ++i) {
		temp_string = Arcs[i].ArcRhs();
		afile << temp_string;
	}
	
	// BOUNDS
	afile << "BOUNDS" << endl;
	bfile << "BOUNDS" << endl;
	
	// Peak load must be met
	for (unsigned int i = 0; i < Nodes.size(); ++i) {
		temp_string = Nodes[i].NodeRMBounds();
		afile << temp_string;
	}
	
	// Flow and investment bounds
	for (unsigned int i = 0; i < Arcs.size(); ++i) {
		temp_string = Arcs[i].ArcBounds();
		afile << temp_string;
		bfile << temp_string;
		temp_string = Arcs[i].ArcInvBounds();
		afile << temp_string;
	}
	
	// DC Power flow angles
	for (unsigned int i = 0; i < ListDCNodes.size(); ++i) {
		temp_string = ListDCNodes[i].DCNodesBounds();
		afile << temp_string;
		bfile << temp_string;
	}
	
	// Sustainability limits
	for (int j = 0; j < SustMet.size(); ++j) {
		int SustIndex = FindCode(SustMet[j], SustLimits);
		if (SustIndex >= 0) {
			Step TempStep(SName.size(), 0);
			for (int i = 1; i <= nyears; ++i) {
				TempStep[0] = i;
				string Value = SustLimits[SustIndex][Step2Pos(TempStep)+1];
				if (Value != "X") {
					afile << " UP bnd " << SustMet[j] << "_" << Step2Str(TempStep) << " " << Value << endl;
					bfile << " UP bnd " << SustMet[j] << "_" << Step2Str(TempStep) << " " << Value << endl;
				}
			}
		}
	}
	
	// End of file
	afile << "ENDATA";
	afile.close();
	bfile << "ENDATA";
	bfile.close();
	
	
	cout << "- Writing auxiliary files..." << endl;
	
	// *** Write step lengths for capacitated arcs ***
	afile.open("prepdata/bend_events.csv");
	
	// Determines whether an operational year needs to be solved for each event
	vector<double> YearEvents(nyears*(Nevents+1), 0);
	
	for (unsigned int i = 0; i < Arcs.size(); ++i) {
		vector<string> ArcEvents(Arcs[i].Events());
		// If information is returned
		if (ArcEvents.size() > 0) {
			afile << ArcEvents[0];
			for (int k = 1; k < ArcEvents.size(); ++k) {
				afile << "," << ArcEvents[k];
				if (ArcEvents[k] != "1") {
					YearEvents[(Arcs[i].Time()-1) * (Nevents+1) + k] = 1;
					YearEvents[(Arcs[i].Time()-1) * (Nevents+1)] = 1;
				}
			}
			afile << endl;
		}
	}
	for (int i=0; i < nyears; ++i) {
		afile << YearEvents[ i * (Nevents+1) ];
		for (int k=1; k <= Nevents; ++k) {
			afile << "," << YearEvents[ i * (Nevents+1) + k ];
		}
		afile << endl;
	}
	afile.close();
	
	// *** Write node, arc information index files ***
	IdxNode.WriteFile("prepdata/idx_node.csv");
	IdxUd.WriteFile("prepdata/idx_ud.csv");
	IdxRm.WriteFile("prepdata/idx_rm.csv");
	IdxArc.WriteFile("prepdata/idx_arc.csv");
	IdxInv.WriteFile("prepdata/idx_inv.csv");
	IdxNsga.WriteFile("prepdata/idx_nsga.csv");
	IdxCap.WriteFile("prepdata/idx_cap.csv");
	IdxUb.WriteFile("prepdata/idx_ub.csv");
	IdxEm.WriteFile("prepdata/idx_em.csv");
	IdxDc.WriteFile("prepdata/idx_dc.csv");
	

	// *** Write node demand information ***
	//WriteOutput("prepdata/data_node_demand.csv", IdxNode, Nodes, "Demand", "% Node demand");
	//WriteOutput("prepdata/data_arc_opmax.csv", IdxArc, Arcs, "OpMax", "% Initial capacity");
	//WriteOutput("prepdata/data_arc_invcost.csv", IdxArc, Arcs, "InvCost", "% Arc: Investment costs");
	//WriteOutput("prepdata/data_arc_invstart.csv", IdxArc, Arcs, "InvStart", "% Arc: Investment start");
	/*for (int j = 0; j < SustMet.size(); ++j) {
		string file_name = "prepdata/data_arc_" + SustMet[j] + ".csv";
		WriteOutput(file_name.c_str(), IdxArc, Arcs, "Op" + SustMet[j], "% Arc sustainability: " + SustMet[j]);
	}*/
	
	
	// *** Write multiobjective parameters file ***
	afile.open("prepdata/param.in");

	// Pop. size, # gen, # objectives, # constraints
	afile << Npopsize << endl;
	afile << Nngen << endl;
	afile << Nobj << endl;
	afile << "0" << endl;
	
	// # real variables (none use yet)
	afile << "0" << endl;
	
	// Crossover probability, mutation, 2 more indices
	afile << Npcross_real << endl;
	afile << Npmut_real << endl;
	afile << Neta_c << endl;
	afile << Neta_m << endl;
	
	// Add # of binary variables, min and max for all
	int num_var = 0;
	string text_var = "";
	for (unsigned int i = 0; i < Arcs.size(); ++i) {
		if (Arcs[i].InvArc() && (Arcs[i].Get("TransInfr") == "") && (Arcs[i].Get("InvMax") != "Inf")) {
			num_var++;
			text_var += Nstages + " " + Arcs[i].Get("InvMin") + " " + Arcs[i].Get("InvMax") + "\n";
		}
	}
	
	afile << num_var << endl;
	afile << text_var;
	
	// Crossover probability, mutation
	afile << Npcross_bin << endl;
	afile << Npmut_bin << endl;
	
	// Close file
	afile.close();
	
	printHeader(H_Completed);

	return 0;
}
