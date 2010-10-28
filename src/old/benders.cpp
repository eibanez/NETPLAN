// --------------------------------------------------------------
//    NETSCORE Version 1
//    benders.cpp -- Implementation of preprocessor for Benders
//    2009-2010 (c) Eduardo Ibáñez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "netscore.h"

int main() {
	printHeader("preprocessor");

	cout << "- Reading global parameters...\n";
	ReadParameters("data/parameters.csv");

	// Variables to store information
	vector<Node> ListNodes(0), Nodes(0), ListDCNodes(0);
	vector<Arc> ListArcs(0), Arcs(0), ListDCArcs(0);
	MatrixStr NStep(0), ATransEnergy(0);
	vector<MatrixStr> NVectorProp(0), AVectorProp(0);
	vector<int> NVectorIndex( NodeProp.size()-NodePropOffset, -1 ), AVectorIndex( ArcProp.size()-ArcPropOffset, -1 );
	
	// Store indices to recover data after optimization
	Index IdxNode, IdxUd, IdxRm, IdxArc, IdxInv, IdxCap, IdxEm;
	
	cout << "- Reading list of nodes...\n";
	ListNodes = ReadListNodes("data/nodes_List.csv");
	cout << "- Reading node data...\n";
	NStep = ReadStep("data/nodes_Step.csv");
	for (unsigned int t=NodePropOffset; t < NodeProp.size(); ++t) {
		string file_name = "data/nodes_" + NodeProp[t] + ".csv";
		NVectorProp.push_back( ReadProperties(file_name.c_str(), NodeDefault[t], 1) );
	}
	
	cout << "- Reading list of arcs...\n";
	ListArcs = ReadListArcs("data/arcs_List.csv");
	cout << "- Reading arc data...\n";
	ATransEnergy = ReadProperties("data/arcs_TransEnergy.csv", "X", 2);
	for (unsigned int t=ArcPropOffset; t < ArcProp.size(); ++t) {
		string file_name = "data/arcs_" + ArcProp[t] + ".csv";
		AVectorProp.push_back( ReadProperties(file_name.c_str(), ArcDefault[t], 2) );
	}

	cout << "- Creating transportation network...\n";
	ReadTrans(ListNodes, ListArcs, "data/trans_List.csv");
	

	// Expand nodes
	cout << "- Expanding nodes...\n";
	for (unsigned int k=0; k < ListNodes.size(); ++k) {
		int StepIndex = FindCode(ListNodes[k], NStep);
		
		// Identify the row containing data for each property
		for (unsigned int t=0; t < NVectorIndex.size(); ++t) {
			NVectorIndex[t] = FindCode(ListNodes[k], NVectorProp[t]);
		}

		// Copy step information
		if (StepIndex >= 0) ListNodes[k].Set("Step", NStep[StepIndex][1]);

		if ( ListNodes[k].Get("Step") == "") {
			printError("nodestep", ListNodes[k].Get("ShortCode"));
		} else {
			// Use a temporary node to store information and cycle through steps
			Step TempStep( SName.size() );
			for (unsigned int l = 0; l < ListNodes[k].Get("Step").size(); l++) TempStep[l] = 1;

			while ( TempStep <= SLength ) {
				// Apply information
				Node TempNode = ListNodes[k];
				TempNode.Set("Step", Step2Str(TempStep));
				TempNode.Set("StepLength", Step2Hours(TempStep));
				int l = Step2Pos(TempStep) + 1;
				TempNode.Set("Code", TempNode.Get("ShortCode") + Step2Str(TempStep));
				
				for (unsigned int t=0; t < NVectorIndex.size(); ++t) {
					int tmp_index = NVectorIndex[t];
					if ( tmp_index >= 0) TempNode.Set( NodeProp[NodePropOffset + t], NVectorProp[t][tmp_index][l] );
				}
				
				// Calculate demand if power demand is given
				if ( (TempNode.Get("Demand") == "0") && (TempNode.Get("DemandPower") != "X") ) {
					double step_length = TempNode.GetDouble("StepLength");
					TempNode.Multiply("DemandPower", step_length);
					TempNode.Set( "Demand", TempNode.Get("DemandPower") );
				}
				
				// Adjust peak demand with increase rate
				double dem_rate = TempNode.GetDouble("DemandRate");
				double peak_rate = TempNode.GetDouble("PeakPowerRate");
				double dem_factor = 1, peak_factor = 1;
				
				if ( (dem_rate != 0) || (peak_rate != 0) ) {
					for (unsigned int l = 1; l < TempStep[0]; ++l) {
						dem_factor = dem_factor * (1 + dem_rate);
						peak_factor = peak_factor * (1 + peak_rate);
					}
					TempNode.Multiply("Demand", dem_factor);
					TempNode.Multiply("PeakPower", peak_factor);
				}
				
				// Store node for later use
				Nodes.push_back(TempNode);
				if ( TempNode.isDCflow() ) ListDCNodes.push_back(TempNode);
				
				// Record indices to recover information
				IdxNode.Add(k, TempStep, TempNode.Get("ShortCode"));
				if ( TempNode.Get("CostUD") != "X" ) {
					IdxUd.Add(k, TempStep, TempNode.Get("ShortCode"));
				}
				if ( TempNode.Get("PeakPower") != "X" ) {
					IdxRm.Add(k, TempStep, TempNode.Get("ShortCode"));
				}
				// Move to the next step
				TempStep = NextStep(TempStep);
			}
		}
	}
	
	
	// Expand arcs
	cout << "- Expanding arcs...\n";
	for (unsigned int k = 0; k < ListArcs.size(); ++k) {
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
		bool isStorage = (ListArcs[k].Get("From").substr(1,1) == StorageCode) && (ListArcs[k].Get("From") == ListArcs[k].Get("To"));

		if ((ListArcs[k].Get("FromStep") == "") && (ListArcs[k].Get("ToStep") == "")) {
			printError("arcstep", ListArcs[k].Get("From") + "_" + ListArcs[k].Get("To"));
		} else {
			// Cycle through steps (more complicated here) to expand arcs
			string TempArcStepCode = max(ListArcs[k].Get("FromStep"), ListArcs[k].Get("ToStep"));

			Step TempStep(SName.size()), TempFromStep(SName.size()), TempToStep(SName.size());
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

			TempStep = ((TempFromStep < TempToStep) && !isStorage ) ? TempToStep : TempFromStep;

			// Find the shortest step, to assign it as a default for 'InvStep'
			string TempStepStr = (TempFromStep < TempToStep) ? ListArcs[k].Get("ToStep") : ListArcs[k].Get("FromStep");

			while ((TempStep <= SLength) && ( TempToStep <= SLength)) {
				// Apply information
				Arc TempArc = ListArcs[k];
				int l = Step2Pos(TempStep) + 2;
				TempArc.Set("FromStep", Step2Str(TempFromStep));
				TempArc.Set("ToStep", Step2Str(TempToStep));
				TempArc.Set("StepLength", Step2Hours(TempStep));
				
				if ( TempArc.isTransport() && TempArc.Get("TransInfr") == "") {
					TempArc.Set("Code", TempArc.Get("From") + Step2Str(TempFromStep));
				} else {
					TempArc.Set("Code", TempArc.Get("From") + Step2Str(TempFromStep) + "_" + TempArc.Get("To") + Step2Str(TempToStep));
				}
				
				for (unsigned int t=0; t < AVectorIndex.size(); ++t) {
					int tmp_index = AVectorIndex[t];
					if ( tmp_index >= 0) TempArc.Set( ArcProp[ArcPropOffset + t], AVectorProp[t][tmp_index][l] );
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
					
					if ( LoadStepCode.size() <= TempArcStepCode.size() ) {
						Step LoadStep = TempStep;
						for (unsigned int m = LoadStepCode.size(); m < LoadStep.size(); m++) LoadStep[m] = 0;
						int l2 = Step2Pos(LoadStep) + 2;
						TempArc.Add("Trans2Energy", LoadCode + Step2Str(LoadStep));
						TempArc.Add("Trans2Energy", ATransEnergy[IndexTemp][l2]);
					} else {
						Step NextTempStep = NextStep(TempStep);
						Step LoadStep = TempStep;
						for (unsigned int m = TempArcStepCode.size(); m < LoadStepCode.size(); m++) LoadStep[m] = 1;
						while ( LoadStep < NextTempStep ) {
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
				
				// From Investment cost, retirement cost, Fixed O&M, discount rate, lifetime  ==> Overnight cost /////////////////////////////////////////
				
				// Apply discount and inflation rate to investment and operational costs
				double factor = (1 + TempArc.GetDouble("InflationRate")) / (1 + TempArc.GetDouble("DiscountRate"));
				double dollar_factor = 1;
				
				double inv_cost = TempArc.GetDouble("InvCost");
				double op_cost = TempArc.GetDouble("InvCost");
				
				if ( factor != 1 && ( inv_cost != 0 || op_cost != 0 ) ) {	
					for (int l = 1; l < TempStep[0]; ++l)
					dollar_factor = dollar_factor * factor;
				}
				
				// If distance is available adjust costs, emissions, demand for energy...
				if ( TempArc.Get("Distance") != "X" ) {
					double distance = TempArc.GetDouble("Distance");
					dollar_factor = dollar_factor * distance;
					
					TempArc.Multiply("InvEmCO2", distance);
					TempArc.Multiply("OpEmCO2", distance);
					TempArc.Multiply("Trans2Energy", distance);
				}
				
				if (dollar_factor != 1) TempArc.Multiply("OpCost", dollar_factor);
				
				// Need to adjust for investment costs at the end of the simulation period
				string life_span = TempArc.Get("LifeSpan");
				if (life_span != "X") {
					int years_left = (SLength[0] + 1) - TempStep[0];
					int life_inv = Str2Step(life_span)[0];
					if (years_left < life_inv)  dollar_factor = dollar_factor * years_left / life_inv;
				}
				
				// Store modified investment costs
				if (dollar_factor != 1) TempArc.Multiply("InvCost", dollar_factor);
				
				// Store Arc for later use
				Arcs.push_back(TempArc);
				if ( TempArc.isDCflow() ) ListDCArcs.push_back(TempArc);
				
				// Store Arc indices to recover solution information
				if ( !TempArc.isTransport() || TempArc.Get("TransInfr") != "" ) {
					IdxArc.Add(k, TempStep, TempArc.Get("From") + "_" + TempArc.Get("To"));
				}
				if ( TempArc.InvArc() ) {
					IdxInv.Add(k, TempStep, TempArc.Get("From") + "_" + TempArc.Get("To"));
				}
				if ( TempArc.Get("OpMax") != "Inf" ) {
					IdxCap.Add(k, TempStep, TempArc.Get("From") + "_" + TempArc.Get("To"));
				}
				
				// Move to next time step
				TempStep = NextStep(TempStep);
				if (NextFromStep <= TempStep) {
					TempFromStep = NextFromStep;
					NextFromStep = NextStep(NextFromStep);
				}

				if (isStorage) {
					TempToStep = NextStep(TempFromStep);
					NextToStep = NextStep(NextToStep);
				} else if (NextToStep <= TempStep) {
					TempToStep = NextToStep;
					NextToStep = NextStep(NextToStep);
				}
			}
		}
	}
	
	// Save index for emissions
	for (int i = 1; i <= SLength[0]; ++i) IdxEm.Add(1, i-1, "emCO2");
	
	// Number of files to create (one per year)
	int nyears = SLength[0];

	// Start MPS file
	cout << "- Writing files...\n";
	ofstream myfile[nyears+1];
	
	myfile[0].open("bend/netscore_inv.mps");
	
	for (int i = 1; i <= nyears; ++i) {
		string file_name = "bend/netscore_" + ToString<int>(i) + ".mps";
		myfile[i].open( file_name.c_str() );
	}
	
	for (int i = 0; i <= nyears; ++i) {
		myfile[i] << "NAME\n";
		myfile[i] << "ROWS\n";
		
		// Cost (objective funtion)
		myfile[i] << " N obj\n";
	}
	
	// CO2 Emissions
	for (int i = 1; i <= nyears; ++i) {
		myfile[0] << " E emCO2" << SName.substr(0,1) << i << "\n";
		myfile[i] << " E emCO2" << SName.substr(0,1) << i << "\n";
	}
	
	// Peak load
	for (unsigned int i = 0; i < Nodes.size(); ++i)
		myfile[0] << Nodes[i].NodePeakRows();

	for (unsigned int i = 0; i < Arcs.size(); ++i)
		myfile[ Arcs[i].Time() ] << Arcs[i].ArcUbNames();
		
	for (unsigned int i = 0; i < Nodes.size(); ++i)
		myfile[ Nodes[i].Time() ] <<  Nodes[i].NodeNames();
	
	for (unsigned int i = 0; i < Arcs.size(); ++i)
		myfile[0] << Arcs[i].ArcCapNames();
	
	for (unsigned int i = 0; i < Arcs.size(); ++i)
		myfile[ Arcs[i].Time() ] << Arcs[i].ArcDcNames();
	
	
	// COLUMNS
	for (int i = 0; i <= nyears; ++i)
		myfile[i] << "COLUMNS\n";
	
	// Cost of subproblems
	for (int i = 1; i <= nyears; ++i)
		myfile[0] << "    cost_" << i << " obj 1\n";
	
	// CO2 emissions
	for (int i = 1; i <= nyears; ++i) {
		myfile[0] << "    emCO2_" << SName.substr(0,1) << i << " emCO2" << SName.substr(0,1) << i << " -1\n";
		myfile[i] << "    emCO2_" << SName.substr(0,1) << i << " emCO2" << SName.substr(0,1) << i << " -1\n";
	}
	
	// Reserve margin
	for (unsigned int i = 0; i < Nodes.size(); ++i)
		myfile[0] << Nodes[i].NodeRMColumns();
	
	for (unsigned int i = 0; i < Arcs.size(); ++i)
		myfile[0] << Arcs[i].InvArcColumns();

	for (unsigned int i = 0; i < Arcs.size(); ++i)
		myfile[0] << Arcs[i].CapArcColumns( true );
		
	for (unsigned int i = 0; i < Arcs.size(); ++i)
		myfile[ Arcs[i].Time() ] << Arcs[i].ArcColumns();
	
	for (unsigned int i = 0; i < Nodes.size(); ++i)
		myfile[ Nodes[i].Time() ] << Nodes[i].NodeColumns();
	
	vector<string> DcOutput = DCFlowColumns(ListDCNodes, ListDCArcs);
	for (unsigned int i = 1; i < DcOutput.size(); ++i)
		myfile[i] << DcOutput[i];
	
	// RHS
	for (int i = 0; i <= nyears; ++i)
		myfile[i] << "RHS\n";
	
	for (unsigned int i = 0; i < Nodes.size(); ++i)
		myfile[ Nodes[i].Time() ] << Nodes[i].NodeRhs();
	
	for (unsigned int i = 0; i < Arcs.size(); ++i)
		myfile[0] << Arcs[i].ArcRhs();
	
	// BOUNDS
	for (int i = 0; i <= nyears; ++i)
		myfile[i] << "BOUNDS\n";
	
	// Peak load must be met
	for (unsigned int i = 0; i < Nodes.size(); ++i)
		myfile[0] << Nodes[i].NodeRMBounds();
	
	for (unsigned int i = 0; i < Arcs.size(); ++i) {
		myfile[ Arcs[i].Time() ] << Arcs[i].ArcBounds();
		myfile[0] << Arcs[i].ArcInvBounds();
	}
	
	for (unsigned int i = 0; i < ListDCNodes.size(); ++i) {
		myfile[ ListDCNodes[i].Time() ] << ListDCNodes[i].DCNodesBounds();
	}
	
	//ENDATA and close files
	for (int i = 0; i <= nyears; ++i) {
		myfile[i] << "ENDATA";
		myfile[i].close();
	}
	
	
	// *** Write step lengths for capacitated arcs ***
	ofstream afile;
	afile.open("bend/steplengths.csv");
	for (unsigned int i = 0; i < Arcs.size(); ++i)
		afile << Arcs[i].StepLengthBenders();
	afile.close();
	
	
	// *** Write node, arc information recovery index file ***
	IdxNode.WriteFile("bend/idx_node.csv");
	IdxUd.WriteFile("bend/idx_ud.csv");
	IdxRm.WriteFile("bend/idx_rm.csv");
	IdxArc.WriteFile("bend/idx_arc.csv");
	IdxInv.WriteFile("bend/idx_inv.csv");
	IdxCap.WriteFile("bend/idx_cap.csv");
	IdxEm.WriteFile("bend/idx_em.csv");
	

	// *** Write node demand information ***
	/*WriteOutput("prepdata/data_node_demand.csv", IdxNode, Nodes, "Demand", "Node demand");
	WriteOutput("prepdata/data_arc_invcost.csv", IdxArc, Arcs, "InvCost", "Arc: Investment costs");
	WriteOutput("prepdata/data_arc_invstart.csv", IdxArc, Arcs, "InvStart", "Arc: Investment start");
	WriteOutput("prepdata/data_arc_emiss.csv", IdxArc, Arcs, "OpEmCO2", "Arc: CO2 Emissions");*/
	
	
	// *** Write multiobjective parameters file ***
	afile.open("param.in");

	// Pop. size, # gen, # objectives, # const.
	afile << "20\n200\n3\n0\n";
	
	// Add # of variables, min and max for all
	int num_var = 0;
	string text_var = "";
	for (unsigned int i = 0; i < Arcs.size(); ++i) {
		if ( Arcs[i].InvArc() && Arcs[i].Get("TransInfr") == "" ) {
			num_var++;
			text_var += Arcs[i].Get("InvMin") + " " + Arcs[i].Get("InvMax") + "\n";
		}
	}
	
	afile << num_var << endl;
	afile << text_var;

	// Probability crossover, mutation, 2 more indices, #bin variables
	afile << "0.75\n0.4\n7\n20\n0";

	// Close file
	afile.close();	
	
	cout << "\nProcess completed!\n\n";

	return 0;
}
