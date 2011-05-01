// --------------------------------------------------------------
//    NETSCORE Version 2
//    solver.cpp -- Implementation of solver functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "index.h"
#include "read.h"
#include "write.h"
#include "solver.h"

#define MAX_ITER 1000

// Loads the problem from MPS files into memory
void CPLEX::LoadProblem() {
	cout << "- Reading problem..." << endl;
	
	try {
		int nyears = SLength[0];
		
		for (int i=0; i <= nyears; ++i) {
			model.add( IloModel(env) );
			cplex.add ( IloCplex(env) );
			obj.add( IloObjective(env) );
			var.add( IloNumVarArray(env) );
			rng.add( IloRangeArray(env) );
			dualsolution.add( IloNumArray(env) );
			TempNumArray.add( IloNumArray(env) );
		}
		
		// Read MPS files
		for (int i=0; i <= nyears; ++i) {
			string file_name = "";
			if (!useBenders && (i == 0)) {
				file_name = "prepdata/netscore.mps";
			} else {
				file_name = "prepdata/bend_" + ToString<int>(i) + ".mps";
			}
			if (i!=0) {
				//cplex[i].setParam(IloCplex::PreInd, 0);
				//cplex[i].setParam(IloCplex::ScaInd, -1); 
				cplex[i].setParam(IloCplex::RootAlg, IloCplex::Dual);
			}
			if (outputLevel > 0) {
				cplex[i].setOut(env.getNullStream());
			} else {
				cout << "Reading " << file_name << endl;
			}
			cplex[i].importModel(model[i], file_name.c_str(), obj[i], var[i], rng[i]);
			
			// Extract models
			cplex[i].extract(model[i]);
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

// Solves current model
void CPLEX::SolveIndividual(double *objective, const double events[], string & returnString) {
	int nyears = SLength[0];
	
	try {
		// Keep track of solution
		bool optimal = true;
		int iter = 0;
		
		if ( !useBenders ) {
			// Only one file
			if (outputLevel < 2 ) cout << "- Solving problem" << endl;
			
			if (cplex[0].solve()) {
				optimal = true;
				objective[0] = cplex[0].getObjValue();
				
				// Store solution if optimal solution found
				StoreSolution();
				StoreDualSolution();
			} else {
				optimal = false;
			}
		} else {
			// Use Benders decomposition
			int OptCuts = 1, FeasCuts = 1;
			
			while ((OptCuts+FeasCuts > 0) && (iter <= MAX_ITER)) {
				++iter; OptCuts = 0; FeasCuts = 0;
				
				// Keep track of necessary cuts
				bool status[nyears];
				IloExprArray expr_cut(env, nyears);
				
				// Save master problem with added cuts
				// cplex[0].exportModel("master.lp");
				
				// Solve master problem. If master is infeasible, exit loop
				if (outputLevel < 2 ) cout << "- Solving master problem (Iteration #" << iter << ")" << endl;
				if (!cplex[0].solve()) {
					break;
				}
				
				// Recover variables (first nyears are estimated obj. val)
				StoreSolution(true);
				
				// Store capacities as constraints
				CapacityConstraints(events, 0, nyears);
				
				// Start subproblems
				if (outputLevel < 2) cout << "- Solving subproblems" << endl << "  ";
				
				for (int j=1; j <= nyears; ++j) {
					// Solve subproblem
					cplex[j].solve();
					
					if (cplex[j].getCplexStatus() != CPX_STAT_OPTIMAL) {
						// If subproblem is infeasible, create feasibility cut
						++FeasCuts; status[j-1] = true;
						
						// Change solver properties to find dual unbouded ray
						cplex[j].setParam(IloCplex::PreInd,0);
						cplex[j].setParam(IloCplex::ScaInd,-1); 
						cplex[j].setParam(IloCplex::RootAlg, IloCplex::Primal);
						cplex[j].solve();
						
						IloExpr temp(env); expr_cut[j-1] = temp;
						cplex[j].getDuals(TempArray, rng[j]);
						for (int k=0; k < TempArray.getSize(); ++k)
							expr_cut[j-1] += TempArray[k] * rng[j][k].getUB();
						cplex[j].getReducedCosts(TempNumArray[j-1], var[j]);
						
						if (outputLevel < 2) cout << j << " ";
					} else if (solution[j-1] <= cplex[j].getObjValue() * 0.999) {
						// If cost is underestimated, create optimality cut
						++OptCuts; status[j-1] = true;
						expr_cut[j-1] = - var[0][j-1];
						cplex[j].getDuals(TempArray, rng[j]);
						for (int k=0; k < TempArray.getSize(); ++k)
							expr_cut[j-1] += TempArray[k] * rng[j][k].getUB();
						cplex[j].getReducedCosts(TempNumArray[j-1], var[j]);
						
						if (outputLevel < 2 ) cout << "o" << j << " ";
					} else {
						status[j-1] = false;
					}
				}
				
				if (OptCuts+FeasCuts > 0) {
					// Finalize cuts
					vector<int> copied(nyears, 0);
					for (int i=0; i < IdxCap.GetSize(); ++i) {
						int year = IdxCap.GetYear(i);
						if ( status[year-1] )
							expr_cut[year-1] += TempNumArray[year-1][copied[year-1]] * var[0][nyears + i];
						++copied[year-1];
					}
					
					// Apply cuts to master
					for (int j=1; j <= nyears; ++j) {
						if (status[j-1]) {
							MasterCuts.add(expr_cut[j-1] <= 0);
							string constraintName = "Cut_y" + ToString<int>(j) + "_iter" + ToString<int>(iter);
							MasterCuts[MasterCuts.getSize()-1].setName(constraintName.c_str());
							model[0].add(MasterCuts[MasterCuts.getSize()-1]);
						}
						// Reset solver properties
						if (cplex[j].getCplexStatus() != CPX_STAT_OPTIMAL) {
							cplex[j].setParam(IloCplex::PreInd,1);
							cplex[j].setParam(IloCplex::ScaInd,0); 
							cplex[j].setParam(IloCplex::RootAlg, IloCplex::Dual);
						}
					}
				} else {
					// Store solution if optimal solution found
					StoreSolution();
					StoreDualSolution();
				}
				
				if (outputLevel < 2) {
					if (OptCuts+FeasCuts == 0) cout << "No cuts - Optimal solution found!";
					cout << endl;
				}
			}
			
			if ((cplex[0].getCplexStatus() == CPX_STAT_OPTIMAL) && (iter <= MAX_ITER)) {
				optimal = true;
				objective[0] = cplex[0].getObjValue();
			} else {
				optimal = false;
			}
		}
		
		if (!optimal) {
			// Solution not found, return very large values
			if (iter > MAX_ITER)
				cout << "\tReached max. number of iterations!" << endl;
			else
				cout << "\tProblem infeasible!" << endl;
			for (int i=0; i < Nobj; ++i)
				objective[i] = 1.0e30;
		} else {
			if (outputLevel < 2)
				cout << "\tCost: " << objective[0] << endl;
			
			// Sustainability metrics
			vector<double> emissions = SumByRow(solution, IdxEm);
			for (int i=0; i < SustObj.size(); ++i) {
				// Print results on screen
				if (outputLevel < 2) {
					if (SustObj[i] == "EmCO2" || SustObj[i] == "CO2") {
						cout << "\t" << SustObj[i] << ": ";
						cout << EmissionIndex(solution, IdxEm.start + SLength[0]*i);
						cout << " (Sum: " << emissions[i] << ")" << endl;
					} else {
						cout << "\t" << SustObj[i] << ": " << emissions[i] << endl;
					}
				}
				
				// Return sustainability metric
				if (SustObj[i] == "EmCO2" || SustObj[i] == "CO2")
					objective[1+i] = EmissionIndex(solution, IdxEm.start + SLength[0]*i);
				else
					objective[1+i] = emissions[i];
			}
			
			// Write emissions and investments in output string (only for NSGA-II postprocessor
			if (returnString != "skip") {
				returnString = "";
				
				// Write total emissions
				for (int i=0; i < SustMet.size(); ++i)
					returnString += "," + ToString<double>( emissions[i] );
				
				// Write investments
				/*vector<double> Investments = SumByRow(solution, IdxInv);
				for (int j=0; j < Investments.size(); ++j)
					returnString += "," + ToString<double>( Investments[j] );*/
			}
			
			// Resiliency calculations
			if (Nevents > 0) {
				bool ResilOptimal = true;
				double ResilObj[Nevents], resiliency = 0;
				int startPos = IdxCap.GetSize() * (Nevents + 1);
				
				// Evaluate all the events and obtain operating cost
				if (outputLevel < 2) cout << "- Solving resiliency..." << endl;
				
				// Initialize operational cost
				for (int event=1; event <= Nevents; ++event)
					ResilObj[event-1] = 0;
				
				for (int j=1; j <= nyears; ++j) {
					if (events[startPos + (j-1) * (Nevents+1)] == 1) {
						// If Benders is used, the operational cost is already available
						if (!useBenders) {
							// Solve subproblem
							CapacityConstraints(events, 0, 0);
							cplex[j].solve();
						}
						
						for (int event=1; event <= Nevents; ++event)
							if (events[startPos + (j-1) * (Nevents+1) + event] == 1)
								ResilObj[event-1] -= cplex[j].getObjValue();
					}
				}
				
				for (int event=1; event <= Nevents; ++event) {
					bool current_feasible = true;
					
					// Store capacities as constraints
					CapacityConstraints(events, event, 0);
					double years_changed[nyears];
					
					for (int j=1; (j <= nyears) & (current_feasible); ++j) {
						if (events[startPos + (j-1) * (Nevents+1) + event] == 1) {
							// Solve subproblem
							cplex[j].solve();
							years_changed[j-1] = 1;
							
							if (cplex[j].getCplexStatus() != CPX_STAT_OPTIMAL) {
								// If subproblem is infeasible
								ResilObj[event-1] = 1.0e10;
								ResilOptimal = false;
								current_feasible = false;
								if (outputLevel < 2) cout << "\t\tEv: " << event << "\tYr: " << j << "\tInfeasible!" << endl;
							} else {
								// If subproblem is feasible
								ResilObj[event-1] += cplex[j].getObjValue();
							}
						} else {
							years_changed[j-1] = 0;
						}
					}
					
					if (current_feasible) {
						StoreDualSolution(event, years_changed);
					}
				}
				
				if (ResilOptimal) {
					// Calculate resiliency results
					for (int j = 0; j < Nevents; ++j) {
						resiliency += ResilObj[j];
						if (outputLevel < 2) cout << "\t\tEv: " << j+1 << "\tCost: " << ResilObj[j] << endl;
					}
					objective[SustObj.size() + 1] = resiliency / Nevents;
					if (outputLevel < 2)
						cout << "\tResiliency: " << resiliency / Nevents << endl;
				} else {
					objective[SustObj.size() + 1] = 1.0e9;
					if (outputLevel < 2)
						cout << "\tResiliency infeasible!" << endl;
				}
				
				// Report resiliency results
				if (returnString != "skip") {
					string tempString = "";
					
					for (int event=0; event < Nevents; ++event)
						tempString += "," + ToString<double>(ResilObj[event]);
					
					returnString = tempString + returnString;
				}
			}
		}
		
		// Erase cuts created with Benders
		if (useBenders) {
			model[0].remove(MasterCuts);
			MasterCuts.endElements();
		}
		
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

void CPLEX::SolveIndividual(double *objective, const double events[]) {
	string skipString = "skip";
	SolveIndividual(objective, events, skipString);
}

// Store complete solution vector
void CPLEX::StoreSolution(bool onlymaster) {
	int nyears = SLength[0];
	solution.clear();
	
	try {
		if (!useBenders || onlymaster) {
			// Only one file
			cplex[0].getValues(solution, var[0]);
		} else {
			// Multiple files (Benders decomposition)
			for (int i=0; i <= nyears; ++i) {
				cplex[i].getValues(TempNumArray[i], var[i]);
			}
			
			// The following array keeps track of what has already been copied
			vector<int> position(nyears+1, 0);
			position[0] = nyears;
			
			// Recover capacities
			for (int j = 0; j < IdxCap.GetSize(); ++j) {
				int tempYear = IdxCap.GetYear(j);
				solution.add(TempNumArray[0][position[0]]);
				++position[0]; ++position[tempYear];
			}
			
			// Recover investments
			for (int j = 0; j < IdxInv.GetSize(); ++j) {
				solution.add(TempNumArray[0][position[0]]);
				++position[0];
			}
			
			// Recover sustainability metrics
			for (int j = 0; j < IdxEm.GetSize(); ++j) {
				int tempYear = IdxArc.GetYear(j);
				solution.add(TempNumArray[tempYear][position[tempYear]]);
				++position[tempYear];
			}
			
			// Recover reserve margin
			for (int j = 0; j < IdxRm.GetSize(); ++j) {
				solution.add(TempNumArray[0][position[0]]);
				++position[0];
			}
			
			// Recover flows
			for (int j = 0; j < IdxArc.GetSize(); ++j) {
				int tempYear = IdxArc.GetYear(j);
				solution.add(TempNumArray[tempYear][position[tempYear]]);
				++position[tempYear];
			}
			
			// Recover unserved demand
			for (int j = 0; j < IdxUd.GetSize(); ++j) {
				int tempYear = IdxUd.GetYear(j);
				solution.add(TempNumArray[tempYear][position[tempYear]]);
				++position[tempYear];
			}
			
			// Recover DC angles
			for (int j = 0; j < IdxDc.GetSize(); ++j) {
				int tempYear = IdxDc.GetYear(j);
				solution.add(TempNumArray[tempYear][position[tempYear]]);
				++position[tempYear];
			}
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

// Store dual solution vector
void CPLEX::StoreDualSolution() {
	int nyears = SLength[0];
	for (int i=0; i<Nevents; ++i)
		dualsolution[i].clear();
	
	try {
		if (!useBenders) {
			// Only one file
			cplex[0].getDuals(TempArray, rng[0]);
			int start = IdxEm.GetSize() + IdxRm.GetSize();
			for (int i=0; i < IdxNode.GetSize(); ++i)
				dualsolution[0].add(TempArray[start +i]);
		} else {
			// Multiple files (Benders decomposition)
			for (int i=1; i <= nyears; ++i)
				cplex[i].getDuals(TempNumArray[i-1], rng[i]);
			
			// The following array keeps track of what has already been copied
			vector<int> position(nyears, SustMet.size());
			
			// Recover nodal duals
			for (int j = 0; j < IdxNode.GetSize(); ++j) {
				int tempYear = IdxNode.GetYear(j);
				dualsolution[0].add(TempNumArray[tempYear-1][position[tempYear-1]]);
				++position[tempYear-1];
			}
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

void CPLEX::StoreDualSolution(int event, double *years) {
	int nyears = SLength[0];
	
	try {
		for (int i=1; i <= nyears; ++i) {
			if (years[i-1] == 1)
				cplex[i].getDuals(TempNumArray[i-1], rng[i]);
			else
				TempNumArray[i-1].clear();
		}
		
		// The following array keeps track of what has already been copied
		vector<int> position(nyears, SustMet.size());
		int globalposition = 0;
		
		// Recover nodal duals
		for (int j = 0; j < IdxNode.GetSize(); ++j) {
			int tempYear = IdxNode.GetYear(j);
			if (years[tempYear-1] == 1) {
				dualsolution[event].add(TempNumArray[tempYear-1][position[tempYear-1]]);
			} else {
				dualsolution[event].add(dualsolution[0][globalposition]);
			}
			++position[tempYear-1]; ++globalposition;
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

// Function called by the NSGA-II method. It takes the minimum investement (x) and calculates the metrics (objective)
void CPLEX::SolveProblem(double *x, double *objective, const double events[]) {
	// Start of investment variables
	int inv = IdxCap.size;
	if (useBenders) inv += SLength[0];
	
	for (int i = 0; i < IdxNsga.GetSize(); ++i)
		var[0][inv + i].setLB(x[i]);
	
	// Solve problem
	SolveIndividual(objective, events);
}

// Apply minimum investments to the master problem
void CPLEX::ApplyMinInv(double *x) {
	// Start of investment variables
	int inv = IdxCap.size;
	if (useBenders) inv += SLength[0];
	
	for (int i = 0; i < IdxNsga.GetSize(); ++i) {
		var[0][inv + i].setLB(x[i]);
	}
}

// Provide solution as a string vector
vector<string> CPLEX::SolutionString() {
	vector<string> solstring(0);
	for (int i=0; i < solution.getSize(); ++i)
		solstring.push_back(ToString<IloNum>(solution[i]));
	return solstring;
}

// Provide dual solution as a string vector
vector<string> CPLEX::SolutionDualString(int event) {
	vector<string> solstring(0);
	for (int i=0; i < dualsolution[event].getSize(); ++i)
		solstring.push_back(ToString<IloNum>(dualsolution[event][i]));
	return solstring;
}

// Apply capacities from master to subproblems
void CPLEX::CapacityConstraints(const double events[], const int event, const int offset) {
	int nyears = SLength[0];
	
	try {
		vector<int> copied(nyears, 0);
		for (int i=0; i < IdxCap.GetSize(); ++i) {
			int year = IdxCap.GetYear(i);
			IloNum rhs = events[i * (Nevents+1) + event] * solution[offset + i];
			var[year][copied[year-1]].setUB(rhs);
			++copied[year-1];
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

double EmissionIndex(const IloNumArray& v, const int start) {
	// This function calculates an emission index
	double em_zero = v[start], max = v[start], min = v[start], reduction = 0.01 * v[start], increase = 0.01, sum = 0;
	int first_year = 5, j = 0;
	vector<double> index(0);
	
	for (int i = 1; i < SLength[0]; ++i) {
		// max: worst case scenario emissions
		max = max * (1 + increase);
		// min: best case scenario emissions
		min -= reduction;
		if ((i > first_year) && (max > min)) {
			// Find index for the emissions at year i and carry a sum
			sum += (v[start+i]-min)/(max-min);
			++j;
		}
	}
	
	// Find average value for the index (zero if cannot be calculated)
	double result = (j == 0) ? 0 : sum/j;
	
	return result;
}

vector<double> SumByRow(const IloNumArray& v, Index Idx) {
	// This function sums each row for an index across years
	int last_index = -1, j=0;
	double sum = 0;
	vector<double> result(0);
	
	for (int i = 0; i < Idx.GetSize(); ++i) {
		if ((last_index != Idx.GetPosition(i)) && (last_index != -1)) {
			result.push_back(sum);
			sum = 0; j=0;
			last_index = Idx.GetPosition(i);
		} else {
			if (last_index == -1)
				last_index = Idx.GetPosition(i);
			sum += v[Idx.start + i];
			++j;
		}
	}
	result.push_back(sum);
	
	return result;
}


/*
// Import Minimum investment into the model from file (not tested)
void ImportMin(const char* filename, const int MstartInv) {
	int inv = MstartInv;
	
	FILE *file;
	char line [200];
	
	file = fopen(filename, "r");
	if (file != NULL) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if (fgets(line, sizeof line, file) == NULL)
				break;
			double d1;
			d1 = strtod(line, NULL);
			model[0].add(var[0][inv] >= d1);
			++inv;
		}
	}
}

*/
