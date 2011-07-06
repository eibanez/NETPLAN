#pragma once
/* *************************************************
Parallelization for NETPLAN  NSGA-IIb
Copyright (C) Jinxu Ding 2010-2011
****************************************************/
//#ifndef GLOBALNAME_H
//#define GLOBALNAME_H

//using namespace std;
//#define DEBUG_main 1
//#define DEBUG_initPopPara 1
//#define DEBUG_myNeplanTaskScheduler 1
//#define PRINTFOUT       1
//#define USE_BOOST_MPI	1
//#define  DEBUG_myNeplanTaskSchedulerTest 1
//#define DEBUG_workerRunTask 1
//#define  DEBUG_MASTERRUNTASK 1
// #define DEBUG_twice_free 1
#include "CNSGA2.h"
// http://stackoverflow.com/questions/1646844/valgrind-reports-invalid-free-delete-delete-c
// Valgrind reports “Invalid free() / delete / delete[]” (c++)
#define DOUBLE_DELETE_GAURD static bool x = false; assert(x == false); x = true;


	#include <mpi.h>
	#include <stdexcept>
	#include <vector>
	//#include <boost/mpi.hpp>

using namespace std;

//#define PRINTFOUT 1

extern int myRank;
extern int mySize;
extern bool testAResult;
extern bool testBResult;
extern int availableRank;
extern int leftTaskNum; 
extern int aveTaskNum;

extern int t2FinishStatusCounter, taskTotalNum, taskCounter, t1Flag, t2Flag, myGenerationNum, 
tempResult1, tempResult2, tempResult;


extern int downStreamTaskTag; 
extern int upStreamTaskTag  ;
//const int generationNum    = 1;
//const int TOTALTASKNUM	   = 50;	
extern const int ISTRUE ;
extern const int ISFALSE;
extern const int SLEEPTIME; // 16;  // 128; //780; // 13 minutes * 60 sec
extern const bool BOOLFALSE ;
extern const bool BOOLTRUE ;

// do timging
//extern mpi::timer       seqTimer;
//extern mpi::timer       paraTimerT1;
//extern mpi::timer       paraTimerT2;

//typedef std::vector<int> myPopSizeVectorType;
//myPopSizeVectorType myPopSizeVector;
//extern std::vector<int>::iterator  myPopSizeVectorItr;

//extern CNSGA2* nsga2a ;
//extern CNSGA2* nsga2b ;

//extern vector< std::vector<ResultTaskPackage*> > resultTaskPackageT2Pr;	
//extern vector< std::vector<message_to_masterT*> > resultTaskPackageT2Pr; 

//extern TaskPackage myTaskPackageM;

void openFile(FILE*,FILE*, FILE*, FILE*);
void openFileTest(FILE*,FILE*, FILE*, FILE*);

typedef struct {
	int popsize  ;
	int ngen     ;
	int nobj     ;
	int ncon     ;
	int nreal    ;
	int nbin    ;
	//friend class boost::serialization::access;
	//template<class Archive>
	//void serialize(Archive& ar, const unsigned int version) 
  	//{
    //		ar & popsize & ngen & nobj & ncon & nreal;
  	//};

} initParaType ;
initParaType  initPara;
//int nbin    = 0 ;
//int nbins;
// the data structure to transfer result from workers to master
// master get results from workers
// the elements : generation,. genCan , indNum (assigned to a worker) , constr_violation , obj  ... 
//		   1		2	  3					4	    5 ... 

extern const int paraTypeTag;

// data structure for master to send pop para to workers 
typedef struct message_para_to_workersT{
	
	//message_para_to_workersT(){};
	//~message_para_to_workersT(){
	//#ifdef DEBUG_twice_free
	//DOUBLE_DELETE_GAURD
	//#endif
	//};

	int 	rank;
	double 	constr_violation;
    	vector<double> xreal;//double 	xreal[nreal];
    	vector<double> obj; // nobj
	vector<double> constr; //double 	constr[ncon];
	double 	crowd_dist;
	int indNum; // the indNum in the next xdata package assigned to a worker
  	int generation;
  	int taskNum;
	int taskTag; // worker use it to return result to master
  	int indBegin;
  	int indEnd;
	int packNum; // packNum is the ind num per node 
	char genCandTag;
	vector<double> xbin;//double 	xreal[nreal];
	//friend class boost::serialization::access;
	//template<class Archive>
	//void serialize(Archive& ar, const unsigned int version) 
  	//{
    	//	ar & rank & constr_violation & xreal & obj & constr & crowd_dist & endRun & generation & taskNum & taskTag & indBegin & indEnd & packNum & genCandTag;
  	//};
} ;
typedef vector<message_para_to_workersT> message_para_to_workers_VecT;
message_para_to_workers_VecT::iterator mToWorkerVecItr;
message_para_to_workers_VecT::iterator mToWorkerVecItr1;
//message_para_to_workers_VecT message_para_to_workers_Vec;
message_para_to_workersT message_para_to_one_worker;
typedef vector<double> vecType;

typedef struct {
	char genCandTag;
	double 	constr_violation;
    	double 	crowd_dist;
	int packNum; // packNum is the ind num per node
	int indNum; // the indNum in the next xdata package assigned to a worker
  	int generation;
  	int taskNum;
	int taskTag; // worker use it to return result to master
  	int indBegin;
  	int indEnd;
	int rank; 
} message_para_to_workersTNew;
message_para_to_workersTNew message_para_to_workersTNewSt;
message_para_to_workersTNew message_para_to_workersSt;
//message_para_to_workersTNew message_para_to_workersTNewSt; // data structure to get data from matser
//typedef vector<message_para_to_workersTNew> message_para_to_workers_VecTNew;
//message_para_to_workers_VecTNew::iterator mToWorkerVecNewItr;
//message_para_to_workers_VecTNew::iterator mToWorkerVecNewItr1;

//message_para_to_workersT* message_para_to_workersTemp; // globalize the pointer to initialize master to worker vec 
//message_para_to_workersT* message_para_to_workers_sendTemp ;

// int 	gene[nbin][nbits[]]; // nbin = 0; gene[][] is not initalized and empty, refer to CNSGA2.cpp line 124  
    	// double 	xbin[nbin]; // nbin =0 
    	//double 	obj[nobj];
    	

// data structure for workers to return result to master 
/*
typedef struct 
{
	double 	constr_violation;
	vector<double> 	obj; // size is nobj;
    	int generation;
  	int taskNum;
	int taskTag; // genration + genCandTag
  	int indNum; // it is the ind num worked by a worker
  	int indEnd; // no use
	int xdataTag; // the result data tag = taskTag + popsize   
	char genCandTag; // A or B	
	//friend class boost::serialization::access;
	//template<class Archive>
	//void serialize(Archive& ar, const unsigned int version) 
  	//{
    	//	ar & obj & generation & taskNum & taskTag & indBegin & indEnd & endRun & genCandTag;
  	//};
		
} message_to_masterT ;
*/
	//typedef vector<message_to_masterT*> message_to_master_VecT;
	//message_to_master_VecT message_to_master_Vec;
	//vector<message_to_masterT*>::iterator mToMasterVecItr; 
	// the 2-D vector to collect results from workers
	/*
	typedef vector<message_to_masterT> resultTaskPackageT1Type;   
	resultTaskPackageT1Type resultTaskPackageT1;
	typedef vector< vector <message_to_masterT> > resultTaskPackageT2PrType;
	resultTaskPackageT2PrType resultTaskPackageT2Pr;
	*/
	//vector< vector <message_to_masterT> > resultTaskPackageT2Pr; // (availableRank, vector<message_to_masterT>());
		
	//vector< std::vector<ResultTaskPackage*> > resultTaskPackageT2Pr(availableRank, vector<ResultTaskPackage*>());
	/*
	vector<message_to_masterT>::iterator t1TaskResultItr_i;
	vector< vector<message_to_masterT> >::iterator t2TaskResultItr_ii;
	typedef vector<message_to_masterT> message_to_master_VecT; 
	
	message_to_masterT message_to_master;

	message_to_masterT* message_to_master_temp; 
	message_to_master_VecT* message_to_master_tempV;
	*/
#ifdef USE_BOOST_MPI
	// define some request handels for mpi_send scheduler 
	vector<boost::mpi::request> recvReqsT1AVec(initPara.popsize);
	vector<boost::mpi::request> recvReqsT1BVec(initPara.popsize);
	vector<boost::mpi::request> recvReqsT2AVec(initPara.popsize);
	vector<boost::mpi::request> recvReqsT2BVec(initPara.popsize);
	
	vector<boost::mpi::request> sendReqsT1AVec(initPara.popsize); 
	vector<boost::mpi::request> sendReqsT1BVec(initPara.popsize); 
	vector<boost::mpi::request> sendReqsT2AVec(initPara.popsize);
	vector<boost::mpi::request> sendReqsT2BVec(initPara.popsize);
	
	boost::mpi::request mToWorkerT1Req;
	boost::mpi::request mToWorkerT2Req;
	boost::mpi::request mToMasterT1Req;
	boost::mpi::request mToMasterT2Req;

	#else

	vector<MPI_Request> recvReqsT1AVec;
	vector<MPI_Request> recvReqsT1BVec;
	vector<MPI_Request> recvReqsT2AVec;
	vector<MPI_Request> recvReqsT2BVec;
	
	vector<MPI_Request> sendReqsT1AVec;
	
	vector<MPI_Request> sendReqsT1BVec;
	
	vector<MPI_Request> sendReqsT2AVec;
	
	vector<MPI_Request> sendReqsT2BVec;
	

	vector<MPI_Request> sendReqsT1AXVec; 
	vector<MPI_Request> sendReqsT1BXVec; 
	vector<MPI_Request> sendReqsT2AXVec;
	vector<MPI_Request> sendReqsT2BXVec;
	
	MPI_Request mToWorkerT1Req;
	MPI_Request mToWorkerT2Req;
	MPI_Request mToMasterT1Req;
	MPI_Request mToMasterT2Req;

	MPI_Request mToWorkerT1XReq;
	MPI_Request mToWorkerT2XReq;
	MPI_Request mToMasterT1XReq;
	MPI_Request mToMasterT2XReq;

	typedef vector<MPI_Request> redVecType;
#endif

// some output files
	FILE * paraT1File ;
    	FILE * paraT2File;
    	FILE * pFile;
    	FILE * paraFile;
 
int taskNumPerNode = 0; 
int runTaskCounter = 0; 

//typedef vector<double> oneDVType;
//typedef vector < vector<double> > twoDVType;

//EOF



