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

CPLEX::CPLEX() : env() {
	try {
		// Full model
		model = new IloModel(env);
		cplex = new IloCplex(env);
		obj = new IloObjective(env);
		var = new IloNumVarArray(env);
		rng = new IloRangeArray(env);
		
		// Operational model
		modelB = new IloModel(env);
		cplexB = new IloCplex(env);
		objB = new IloObjective(env);
		varB = new IloNumVarArray(env);
		rngB = new IloRangeArray(env);
		
		// Aditional values
		solution = new IloNumArray(env, 0);
		TempArray = new IloNumArray(env, 0);
		dualsolution = NULL;
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

CPLEX::~CPLEX() {
	try {
		// Remove optimization elements from memory
		if (dualsolution != NULL)
			dualsolution->end();
		delete dualsolution;
		TempArray->end(); delete TempArray;
		solution->end(); delete solution;
		
		rngB->end(); delete rngB;
		varB->end(); delete varB;
		objB->end(); delete objB;
		cplexB->end(); delete cplexB;
		modelB->end(); delete modelB;
		
		rng->end(); delete rng;
		var->end(); delete var;
		obj->end(); delete obj;
		cplex->end(); delete cplex;
		model->end(); delete model;
		
		env.end();
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
};

// Loads the problem from MPS files into memory
void CPLEX::LoadProblem() {
	cout << "- Reading problem..." << endl;
	
	try {
		if (outputLevel > 0) {
			cplex->setOut(env.getNullStream());
			cplexB->setOut(env.getNullStream());
		}
		string file_name = "prepdata/netscore.mps";
		cplex->importModel(*model, file_name.c_str(), *obj, *var, *rng);
		file_name = "prepdata/netscore-op.mps";
		cplexB->importModel(*modelB, file_name.c_str(), *objB, *varB, *rngB);
		
		// Extract models
		cplex->extract(*model);
		cplexB->extract(*modelB);
		
		// Initialize variable to store dual solutions
		dualsolution = new IloNumArray(env, (Nevents + 1) * IdxNode.size);
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
		// Start by solving full problem
		if (outputLevel < 2)
			cout << "- Minimizing cost..." << endl;
		
		if (!cplex->solve()) {
			// Solution not found, return very large values
			cout << "\tProblem infeasible!" << endl;
			for (int i=0; i < Nobj; ++i)
				objective[i] = 1.0e30;
		} else {
			// Solution found, store objective
			objective[0] = cplex->getObjValue();
			if (outputLevel < 2)
				cout << "\tCost: " << objective[0] << endl;
				
			// Store solution
			StoreSolution();
			StoreDualSolution();
			
			// Sustainability metrics
			vector<double> emissions = SumByRow(*solution, IdxEm);
			for (int i=0; i < SustObj.size(); ++i) {
				// Print results on screen
				if (outputLevel < 2) {
					if (SustObj[i] == "EmCO2" || SustObj[i] == "CO2") {
						cout << "\t" << SustObj[i] << ": ";
						cout << EmissionIndex(*solution, IdxEm.start + SLength[0]*i);
						cout << " (Sum: " << emissions[i] << ")" << endl;
					} else {
						cout << "\t" << SustObj[i] << ": " << emissions[i] << endl;
					}
				}
				
				// Return sustainability metric
				if (SustObj[i] == "EmCO2" || SustObj[i] == "CO2")
					objective[1+i] = EmissionIndex(*solution, IdxEm.start + SLength[0]*i);
				else
					objective[1+i] = emissions[i];
			}
			
			// Write emissions and investments in output string (only for NSGA-II postprocessor)
			if (returnString != "skip") {
				returnString = "";
				
				// Write total emissions
				for (int i=0; i < SustMet.size(); ++i)
					returnString += "," + ToString<double>(emissions[i]);
				
				// Write investments
				/*vector<double> Investments = SumByRow(*solution, IdxInv);
				for (int j=0; j < Investments.size(); ++j)
					returnString += "," + ToString<double>(Investments[j]);*/
			}
			
			// Resiliency calculations
			if (Nevents > 0) {
				bool ResilOptimal = true;
				double ResilObj[Nevents], resiliency = 0;
				int startPos = IdxCap.size * (Nevents + 1);
				
				// Evaluate all the events and obtain operating cost
				if (outputLevel < 2)
					cout << "- Solving resiliency..." << endl;
				
				// Initialize operational cost
				CapacityConstraints(events);
				if (!cplexB->solve())
					ResilOptimal = false;
				else {
					for (int event = 1; event <= Nevents; ++event)
						ResilObj[event-1] = -cplexB->getObjValue();
				}
				
				for (int event = 1; event <= Nevents && ResilOptimal; ++event) {
					// Store capacities as constraints and solve
					CapacityConstraints(events, event);
					
					if (!cplexB->solve()) {
						// If operational problem is infeasible
						ResilObj[event-1] = 1.0e10;
						ResilOptimal = false;
						if (outputLevel < 2)
							cout << "\tEvent: " << event << "\tInfeasible!" << endl;
					} else {
						// If subproblem is feasible
						ResilObj[event - 1] += cplexB->getObjValue();
						StoreDualSolution(event);
						resiliency += ResilObj[event - 1];
						if (outputLevel < 2)
							cout << "\tEvent: " << event << "\tCost delta: " << ResilObj[event - 1] << endl;
					}
				}
				
				if (ResilOptimal) {
					// Calculate resiliency results
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
					
					for (int event = 0; event < Nevents; ++event)
						tempString += "," + ToString<double>(ResilObj[event]);
					returnString = tempString + returnString;
				}
			}
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
void CPLEX::StoreSolution() {
	int nyears = SLength[0];
	solution->clear();
	
	try {
		cplex->getValues(*solution, *var);
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

// Store dual solution vector
void CPLEX::StoreDualSolution(int event) {
	try {
		int start = IdxEm.size;
		int start2 = event * IdxNode.size;
		if (event == 0) {
			// Full model
			cplex->getDuals(*TempArray, *rng);
			start += IdxRm.size;
		} else {
			// Operational model
			cplexB->getDuals(*TempArray, *rngB);
		}
		for (int i = 0; i < IdxNode.size; ++i)
			(*dualsolution)[start2 + i] = (*TempArray)[start + i];
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
	
	for (int i = 0; i < IdxNsga.size; ++i)
		(*var)[inv + i].setLB(x[i]);
	
	// Solve problem
	SolveIndividual(objective, events);
}

// Apply minimum investments to the master problem
void CPLEX::ApplyMinInv(double *x) {
	// Start of investment variables
	int inv = IdxCap.size;
	
	for (int i = 0; i < IdxNsga.size; ++i) {
		(*var)[inv + i].setLB(x[i]);
	}
}

// Provide solution as a string vector
vector<string> CPLEX::SolutionString() {
	vector<string> solstring(0);
	for (int i = 0; i < solution->getSize(); ++i)
		solstring.push_back(ToString<IloNum>((*solution)[i]));
	return solstring;
}

// Provide dual solution as a string vector
vector<string> CPLEX::SolutionDualString(int event) {
	vector<string> solstring(0);
	for (int i = 0; i < IdxNode.size; ++i)
		solstring.push_back(ToString<double>((*dualsolution)[event * IdxNode.size + i]));
	return solstring;
}

// Apply capacities from master to subproblems
void CPLEX::CapacityConstraints(const double events[], int event) {
	int nyears = SLength[0];
	
	try {
		for (int i=0; i < IdxCap.size; ++i) {
			IloNum rhs = events[i * (Nevents + 1) + event] * (*solution)[i];
			(*varB)[i].setUB(rhs);
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

double EmissionIndex(const IloNumArray v, const int start) {
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

vector<double> SumByRow(const IloNumArray v, Index Idx) {
	// This function sums each row for an index across years
	int last_index = -1, j=0;
	double sum = 0;
	vector<double> result(0);
	
	for (int i = 0; i < Idx.size; ++i) {
		if ((last_index != Idx.position[i]) && (last_index != -1)) {
			result.push_back(sum);
			sum = 0; j=0;
			last_index = Idx.position[i];
		} else {
			if (last_index == -1)
				last_index = Idx.position[i];
			sum += v[Idx.start + i];
			++j;
		}
	}
	result.push_back(sum);
	
	return result;
}
