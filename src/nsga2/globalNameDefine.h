#pragma once
/* *************************************************
Parallelization for NETPLAN  NSGA-IIb
Copyright (C) Jinxu Ding 2010-2011
****************************************************/
#include <fstream>
#include <iostream>
#include <cstdio>
int myRank =0;
int mySize =0;
bool testAResult=false;
bool testBResult=false;
int availableRank=0;
int leftTaskNum=0; 
int aveTaskNum=0;

int t2FinishStatusCounter=0, taskTotalNum=0, taskCounter=0, t1Flag=0, t2Flag=0, myGenerationNum=0, 
tempResult1=0, tempResult2=0, tempResult=0;

//FILE * paraT1File;
//FILE * paraT2File;
//FILE * pFile;


int downStreamTaskTag= 123; 
int upStreamTaskTag  = 456;
//const int generationNum    = 1;
//const int TOTALTASKNUM	   = 50;	
const int ISTRUE =1;
const int ISFALSE =0;
const int SLEEPTIME= 0; // 0; //1; //32; // 16;  // 128; //780; // 13 minutes * 60 sec workload/ind 
const bool BOOLFALSE = 0;
const bool BOOLTRUE = 1;

// do timging
//mpi::timer       seqTimer;
//mpi::timer       paraTimerT1;
//mpi::timer       paraTimerT2;

//typedef std::vector<int> myPopSizeVectorType;
//myPopSizeVectorType myPopSizeVector;
//std::vector<int>::iterator  myPopSizeVectorItr;

CNSGA2*	nsga2a = new CNSGA2(true, 1.0);
CNSGA2*	nsga2b = new CNSGA2(false, 0.33);

//vector< std::vector<ResultTaskPackage*> > resultTaskPackageT2Pr(availableRank, vector<ResultTaskPackage*>());	

//TaskPackage myTaskPackageM;
//TaskSleep seqTasksleep;

//int myNeplanTaskScheduler(CNSGA2* nsga2, int popSize, population * myChildpop, mpi::communicator world, char genTag, int generationNum);

//bool myNeplanTaskSchedulerTest(char genCandTag, population * myChildpop, mpi::communicator);


vector<MPI_Request>::iterator recvReqsT2Itr;
vector<MPI_Request>::iterator recvReqsT1Itr;

// 

const int paraTypeTag=321;


/*
#else
extern vector<MPI_Request> recvReqsT1A(taskTotalNum);
extern vector<MPI_Request> recvReqsT1B(taskTotalNum);
extern vector<MPI_Request> recvReqsT2A(taskTotalNum);
extern vector<MPI_Request> recvReqsT2B(taskTotalNum);
extern vector<MPI_Request> sendReqsT1A(taskTotalNum); 
extern vector<MPI_Request> sendReqsT1B(taskTotalNum); 
extern vector<MPI_Request> sendReqsT2A(taskTotalNum);
extern vector<MPI_Request> sendReqsT2B(taskTotalNum);
extern vector<MPI_Request>::iterator recvReqsT2Itr;
extern vector<MPI_Request>::iterator recvReqsT1Itr;
#endif
*/
//EOF




