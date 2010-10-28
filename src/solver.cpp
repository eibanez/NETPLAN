// --------------------------------------------------------------
//    NETSCORE Version 1
//    solver.cpp -- Implementation of solver functions
//    2009-2010 (c) Eduardo Ibáñez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "index.h"
#include "read.h"
#include "solver.h"
#include <ilcplex/ilocplex.h>

// Declare variables to store the optimization model
IloEnv env;
IloArray<IloModel> model(env, 0);
IloArray<IloCplex> cplex(env, 0);
IloArray<IloObjective> obj(env, 0);
IloArray<IloNumVarArray> var(env, 0);
IloArray<IloRangeArray> rng(env, 0);
IloRangeArray TempCuts(env, 0);

// Vector of step lengths for capacitated arcs
vector< vector<double> > steplength(0);

// Loads the problem from MPS files into memory
void LoadProblem() {
	try {
		int nyears = SLength[0];
		
		for (int i=0; i <= nyears; ++i) {
			model.add( IloModel(env) );
			cplex.add ( IloCplex(env) );
			obj.add( IloObjective(env) );
			var.add( IloNumVarArray(env) );
			rng.add( IloRangeArray(env) );
		}
		
		// Read MPS files
		for (int i=0; i <= nyears; ++i) {
			string file_name = "";
			if ( !useBenders && (i == 0) ) {
				file_name = "netscore.mps";
			} else {
				file_name = "prepdata/bend_" + ToString<int>(i) + ".mps";
			}
			if (i!=0) {
				cplex[i].setParam(IloCplex::PreInd,0);
				cplex[i].setParam(IloCplex::ScaInd,-1); 
				cplex[i].setParam(IloCplex::RootAlg, IloCplex::Primal);
			}
			if (outputLevel > 0 ) {
				cplex[i].setOut(env.getNullStream());
			} else {
				cout << "Reading " << file_name << endl;
			}
			cplex[i].importModel(model[i], file_name.c_str(), obj[i], var[i], rng[i]);
		}
		
		// Read vector of step lengths for capacitated arcs
		steplength = ReadBendersStepLength("prepdata/bend_steplengths.csv");

	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

// Solves current model
void OptimizeCost( bool& optimal, double& objective, IloRangeArray& TempCuts ) {
	int nyears = SLength[0], MstartCap = var[0].getSize() - IdxCap.GetSize();
	
	try {
		if ( !useBenders ) {
			// Only one file
			if (outputLevel < 2 ) cout << "- Solving problem" << endl;
			cplex[0].extract( model[0] );
			if ( cplex[0].solve() ) {
				optimal = true;
				objective = cplex[0].getObjValue();
			} else {
				optimal = false;
			}
		} else {
			// Use Benders decomposition
			int OptCuts = 1, FeasCuts = 1, TotalCuts = TempCuts.getSize(), iter = 0;
			
			// Keep track of necessary cuts
			bool status[nyears];
			IloExprArray expr_cut(env, nyears);
			IloNumArray rhs_cut(env, nyears), result(env), dual(env);
			IloArray<IloNumArray> duals(env, nyears);
			
			while ((OptCuts+FeasCuts > 0) && ( iter <= 1000)) {		
				++iter; OptCuts = 0; FeasCuts = 0;
				
				// Solve master problem
				if (outputLevel < 2 ) cout << "- Solving master problem (Iteration #" << iter << ")" << endl;
				cplex[0].extract( model[0] );
				// If master is infeasible, exit loop
				if ( !cplex[0].solve() ) {
					break;
				}
				
				// Recover variables (first nyears are estimated obj. val)
				cplex[0].getValues(result, var[0]);
				
				// Solve subproblems
				if (outputLevel < 2 ) cout << "- Solving subproblems" << endl << "  ";
				
				// Apply capacities
				ApplyCapacities(0);
				
				for (int j=1; j <= nyears; ++j) {
					// Solve subproblem
					cplex[j].extract(model[j]);
					cplex[j].solve();
					
					// If subproblem is infeasible
					if (cplex[j].getCplexStatus() != CPX_STAT_OPTIMAL) {
						// Create feasibility cut
						++FeasCuts; ++TotalCuts; status[j-1] = true;
						cplex[j].getDuals(dual, rng[j]);
						duals[j-1] = dual;
						IloExpr expr(env);
						expr_cut[j-1] = expr;
						rhs_cut[j-1] = cplex[j].getObjValue();
						
						if (outputLevel < 2 ) cout << "f" << j << " ";
					} else if (result[j-1] < cplex[j].getObjValue() * 0.999) {
						// If cost is underestimated, create optimality cut
						++OptCuts; ++TotalCuts; status[j-1] = true;
						cplex[j].getDuals(dual, rng[j]);
						duals[j-1] = dual;
						IloExpr expr(env);
						expr_cut[j-1] = expr;
						expr_cut[j-1] = var[0][j-1];
						rhs_cut[j-1] = cplex[j].getObjValue();
						
						if (outputLevel < 2 ) cout << j << " ";
					} else {
						status[j-1] = false;
					}
				}
				
				// Create cuts
				vector<int> copied2(nyears, 0);
				for (int i=0; i < IdxCap.GetSize(); ++i) {
					int year = IdxCap.GetYear(i);
					if (status[year-1]) {
						expr_cut[year-1] += -duals[year-1][copied2[year-1]] * steplength[i][0] * var[0][MstartCap + i];
						rhs_cut[year-1] += -duals[year-1][copied2[year-1]] * steplength[i][0] * result[MstartCap + i];
						++copied2[year-1];
					}
				}
				
				// Apply cuts to master
				for (int j=1; j <= nyears; ++j) {
					if (status[j-1]) {
						TempCuts.add( expr_cut[j-1] >= rhs_cut[j-1]);
						string constraintName = "Cut_y" + ToString<int>(j) + "_iter" + ToString<int>(iter);
						TempCuts[TempCuts.getSize()-1].setName( constraintName.c_str() );
						model[0].add( TempCuts[TempCuts.getSize()-1] );
					}
				}
				
				if (outputLevel < 2 ) {
					if (OptCuts+FeasCuts == 0) cout << "No cuts";
					cout << endl;
				}
			}
			if (cplex[0].getCplexStatus() == CPX_STAT_OPTIMAL) {
				optimal = true;
				objective = cplex[0].getObjValue();
			} else {
				optimal = false;
				objective = -1;
			}
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

void OptimizeCost( bool& optimal, double& objective) {
	IloRangeArray a(env, 0);
	OptimizeCost( optimal, objective, a);
}

// Recover complete solution vector
vector<double> SolutionDouble() {
	int nyears = SLength[0];
	vector<double> ValDouble(0);
	
	try {
		if ( !useBenders ) {
			// Only one file
			IloNumArray varsol(env);
			cplex[0].getValues(varsol, var[0]);
			for (int i = 0; i < varsol.getSize(); ++i)
				ValDouble.push_back( varsol[i] );
		} else {
			// Multiple files (Benders decomposition)
			IloArray<IloNumArray> varsol(env, 0);
			for (int i=0; i <= nyears; ++i) {
				IloNumArray temp(env);
				cplex[i].getValues(temp, var[i]);
				varsol.add(temp);
			}
			
			// The following array keeps track of what has already been copied
			vector<int> position(nyears+1, 0);
			position[0] = nyears;
			
			// Recover sustainability metrics
			for (int j = 0; j < IdxEm.GetSize(); ++j) {
				int tempYear = IdxArc.GetYear(j);
				ValDouble.push_back( varsol[tempYear][position[tempYear]] );
				++position[tempYear];
			}
			
			// Recover reserve margin, investment and capacity
			for (int j = 0; j < IdxRm.GetSize() + IdxInv.GetSize() + IdxCap.GetSize(); ++j) {
				ValDouble.push_back( varsol[0][position[0]] );
				++position[0];
			}
			
			// Recover flows
			for (int j = 0; j < IdxArc.GetSize(); ++j) {
				int tempYear = IdxArc.GetYear(j);
				ValDouble.push_back( varsol[tempYear][position[tempYear]] );
				++position[tempYear];
			}
			
			// Recover unserved demand
			for (int j = 0; j < IdxUd.GetSize(); ++j) {
				int tempYear = IdxUd.GetYear(j);
				ValDouble.push_back( varsol[tempYear][position[tempYear]] );
				++position[tempYear];
			}
			
			// Recover DC angles
			for (int j = 0; j < IdxDc.GetSize(); ++j) {
				int tempYear = IdxDc.GetYear(j);
				ValDouble.push_back( varsol[tempYear][position[tempYear]] );
				++position[tempYear];
			}
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
	// Return variables
	return ValDouble;
}

vector<string> SolutionString() {
	vector<string> ValStr(0);
	vector<double> ValDouble( SolutionDouble() );
	for (int i=0; i < ValDouble.size(); ++i)
		ValStr.push_back( ToString<IloNum>(ValDouble[i]) );
	return ValStr;
}


// Apply capacities from master to subproblems
void ApplyCapacities(int event) {
	try {
		int nyears = SLength[0];
		int MstartCap = (useBenders) ? var[0].getSize() - IdxCap.GetSize() : IdxEm.GetSize() + IdxRm.GetSize() + IdxInv.GetSize();
		IloNumArray result(env);
		
		// Recover variables
		cplex[0].getValues(result, var[0]);
		
		// Apply capacities
		vector<int> copied(nyears, 0);
		for (int i=0; i < IdxCap.GetSize(); ++i) {
			int year = IdxCap.GetYear(i);
			rng[year][copied[year-1]].setUB( steplength[i][event] * result[MstartCap + i] );
			++copied[year-1];
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}


// Eliminate cuts created during Benders
void EraseCuts( IloRangeArray& TempCuts ) {
	model[0].remove( TempCuts );
}


// Evaluate resiliency
void TestResiliency(bool& optimal, double objectives[]) {
	int nyears = SLength[0];
	optimal = true;
	
	try {
		// Evaluate all the events and obtain operating cost
		if (outputLevel < 2 ) cout << "- Solving resiliency..." << endl;
		int startEvent = (useBenders) ? 1 : 0;
		double baseCost[nyears];
		
		if (useBenders) {
			for (int j=1; (j <= nyears); ++j) {
				baseCost[j-1] = cplex[j].getObjValue();
				objectives[0] += baseCost[j-1];
			}
		}
		
		for (int event=startEvent; event <= Nevents; ++event) {
			// Initialize operational cost
			objectives[event] = 0;
			bool current_feasible = true;
			
			// Apply capacities
			ApplyCapacities(event);
			
			for (int j=1; (j <= nyears) & (current_feasible); ++j) {
				// Solve subproblem
				cplex[j].extract(model[j]);
				cplex[j].solve();
				
				if (cplex[j].getCplexStatus() != CPX_STAT_OPTIMAL) {
					// If subproblem is infeasible
					objectives[event] = -1;
					optimal = false;
					current_feasible = false;
					// cout << "\tEvent: " << event << "\ty: " << j << "\tInfeasible!" << endl;
				} else {
					// If subproblem is feasible
					objectives[event] += cplex[j].getObjValue();
					if (useBenders && (event==0)) baseCost[j-1] = cplex[j].getObjValue();
					// cout << "\tEvent: " << event << "\ty: " << j << "\tObj: " << cplex[j].getObjValue() << endl;
				}
			}
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

// Import Minimum investment into the model from file (not tested)
void ImportMin( const char* filename, const int MstartInv ) {
	int inv = MstartInv;
	
	FILE *file;
	char line [ 200 ];
	
	file = fopen(filename, "r");
	if ( file != NULL ) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if ( fgets(line, sizeof line, file) == NULL )
				break;
			double d1;
			d1 = strtod(line, NULL);
			model[0].add( var[0][inv] >= d1 );
			++inv;
		}
	}
}
