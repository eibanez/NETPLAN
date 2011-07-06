#pragma once

// Standard includes
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

// Other includes
#include "CFileIO.h"
#include "CRand.h"
#include "CQuicksort.h"
#include "CLinkedList.h"
#include "defines.h"
#include "../solver.h"

using namespace std;

class CFileIO;
class CLinkedList;

// ------------------------------------------------ //
//													//
// 		Main class for the NSGA-II algorithm		//
//													//
// ------------------------------------------------ //
class CNSGA2 {
	public:
		CNSGA2(bool output=true, double seed=RAND_SEED);
		~CNSGA2(void);
		CNSGA2(int noFileIO); //only for evalaute()
		
		// Initialization methods
		void Init(const char* param);
		void InitMemory();
		void InitPop(population *pop, double prob); // Initialize population randomly
		void InitInd(individual *ind, double prob); // Initialize individual randomly
		void ResumePop(population *pop, const char* fileinput); // Resume a population
		
		// Memory allocation/deallocation methods
		void allocate_memory_pop(population *pop, int size);
		void allocate_memory_ind(individual *ind);
		void deallocate_memory_pop(population *pop, int size);
		void deallocate_memory_ind(individual *ind);
		
		// Population decode methods
		void decodePop(population *pop);
		void decodeInd(individual *ind);
		
		// Population evaluate methods
		void evaluatePop (population *pop, CPLEX& netplan, const double events[]);
		// used for solving one pop ind
		void evaluatePopInd (population *pop, const double events[], int workerPopSize, int& objRSize, int myRank);
		//void evaluatePopInd (population *pop, const double events[], int workerPopSize, int& objRSize, int myRank, CPLEX* netplan);
		// master solve one ind
		void mEvaluatePopInd (population* pop, const double events[], vector<double>& mResultTaskPackageT12, int beginInd, int nobj, int genNum, char canTag, int myRank);
		//void mEvaluatePopInd (population* pop, const double events[], vector<double>& mResultTaskPackageT12, int beginInd, int nobj, int genNum, char canTag, int myRank, CPLEX* netplan);
		void sendPop(population *pop);
		void receivePop(population *pop);
		// void evaluateInd(individual *ind, const double events[], CPLEX& netplan);
		
		// Assign rank and crowding distance
		void assignRankCrowdingDistance(population *new_pop);
		void assignCrowdingDistance(population *pop, int *dist, int **obj_array, int front_size);
		void assignCrowdingDistanceList(population *pop, list *lst, int front_size);
		void assignCrowdingDistanceIndices(population *pop, int c1, int c2);
		
		// Check Dominance
		int checkDominance(individual *a, individual *b);
		
		// Tournament Selection
		void selection(population *old_pop, population *new_pop);
		individual* tournament(individual *ind1, individual *ind2);
		
		// Crossover
		void crossover(individual *parent1, individual *parent2, individual *child1, individual *child2);
		void realcross(individual *parent1, individual *parent2, individual *child1, individual *child2);
		void bincross(individual *parent1, individual *parent2, individual *child1, individual *child2);
		
		// Mutation
		void mutatePop(population *pop);
		void mutateInd(individual *ind);
		void binMutateInd(individual *ind);
		void realMutateInd(individual *ind);
		
		// Merge & Copy
		void merge(population *pop1, population *pop2, population *pop3);
		void copyInd(individual *ind1, individual *ind2);
		
		// Fill Non-dominated sort
		void fillNondominatedSort(population *mixed_pop, population *new_pop);
		void crowdingFill(population *mixed_pop, population *new_pop, int count, int front_size, list *elite);
		
		// NSGA-II Test Problem
		// void test_problem (double *xreal, double *xbin, int **gene, double *objective, double *constr);
		
		// NSGA-II variables
		int nreal;
		int nbin;
		int nobj;
		int ncon;
		int popsize;
		double pcross_real;
		double pcross_bin;
		double pmut_real;
		double pmut_bin;
		double eta_c;
		double eta_m;
		int ngen;
		int nbinmut;
		int nrealmut;
		int nbincross;
		int nrealcross;
		int *nbits;
		double *min_realvar;
		double *max_realvar;
		double *min_binvar;
		double *max_binvar;
		int bitlength;
		
		bool 		memallo;
		
		// Populations
		population *parent_pop;
		population *child_pop;
		population *mixed_pop;
		
		// Helper classes
		CRand* randgen;
		CFileIO* fileio;
		CQuicksort* quicksort;
		CLinkedList* linkedlist;
		
		int getPopsize(){return popsize;};
		int getNreal(){return nreal;};
		int getNbin(){return nbin;};
		//int getNbins(){return nbins;};
		int getNcon(){return ncon;};
		int getNobj(){return nobj;};
		int getNgen(){return ngen;};
};

class newCNSGA2:public CNSGA2{
public:
	//newCNSGA2(){memallo = false;  cout << " constructor for newCNSGA2 \n\n" << endl;};
	newCNSGA2(int noFileIO);
	//newCNSGA2(int newClass);	
	//newCNSGA2(int myClass);
	//~newCNSGA2();
	
		//cout << " destructor for newCNSGA2 \n\n" << endl;
		//#ifdef DEBUG_twice_free
		//DOUBLE_DELETE_GAURD
		//#endif
	
	
};
