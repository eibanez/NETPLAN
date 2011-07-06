/* ****************************************************
Parallelization for NETPLAN  NSGA-IIb
Copyright (C) Jinxu Ding 2010-2011
The Netplan scheduler that can schedule task to workers
*******************************************************/

//#include <stdio.h>
#include <iostream>
#include <vector>
#include <sys/utsname.h>
#include "globalName.h"

// T1:  taskTotalNum <= availableRank
//T2:   taskTotalNum <= availableRank maybe evenly or non-evenly distributed

//#include </lustre/jxding/rhascheduler/include/Scheduler/BasicScheduler.hpp>

/*
  This example shows how tasks can be defined
  as non-member functions.
*/
//extern int masterRunTask(population* myChildpop, vector<double>& resultTaskPackage, int iRank, int objsize, int myGenerationNum, char genCandTag, CPLEX netplan);
extern int masterRunTask(population* myChildpop, vector<double>& resultTaskPackage, int iRank, int objsize, int myGenerationNum, char genCandTag);
//myNeplanTaskScheduler(nsga2a, nsga2a->popsize, nsga2a->child_pop, world, 'A', i, &message_para_to_workers_VecT);

//int myNeplanTaskScheduler(CNSGA2* nsga2, int popSize, int nodeSize, int myRank, population * myChildpop, char genCandTag, int generationNum, message_para_to_workers_VecT& myPopParaVec, MPI_Datatype message_to_master_type, int& myT1Flag, int& myT2Flag, vector< vector<double> >& resultTaskPackageT1, vector< vector<double> >& resultTaskPackageT2Pr, vector<double>& xdataV, int objSize, vector <vector <double> >& resultTaskPackageT12, MPI_Datatype xdata_to_workers_type, int myGenerationNum, MPI_Datatype Mpara_to_workers_type, int nconNum, CPLEX netplan)
int myNeplanTaskScheduler(CNSGA2* nsga2, int popSize, int nodeSize, int myRank, population * myChildpop, char genCandTag, int generationNum, message_para_to_workers_VecT& myPopParaVec, MPI_Datatype message_to_master_type, int& myT1Flag, int& myT2Flag, vector< vector<double> >& resultTaskPackageT1, vector< vector<double> >& resultTaskPackageT2Pr, vector<double>& xdataV, int objSize, vector <vector <double> >& resultTaskPackageT12, MPI_Datatype xdata_to_workers_type, int myGenerationNum, MPI_Datatype Mpara_to_workers_type, int nconNum)
{
    //mpi::environment env(argc, argv);
    //mpi::communicator world;
    // total ranks of the world communicator
   
    #ifdef PRINTOUT
    openFile(pFile,paraFile,paraT1File,  paraT2File );
    #endif	
    availableRank = nodeSize-1;
    //availableRank = nodeSize;
    taskTotalNum = popSize;
    
    //vector< vector<ResultTaskPackage*> > resultTaskPackageT2Pr(availableRank, vector<ResultTaskPackage*>());	
   #ifdef DEBUG_myNeplanTaskScheduler
   cout << "In myNeplanTaskScheduler(), I am rank " << myRank << " , out of total size " << mySize << ", the taskTotalNum is "  <<  taskTotalNum << " , the availableRank is " << availableRank << ", the generationNum is " << generationNum << " taskTotalNum is " << taskTotalNum << ". \n\n"  << endl;
   #endif
   int beginIndex = 0; taskCounter=0;
   // a data structure to transfer pop para to workers
   //message_para_to_workersTNew message_para_to_workersSt;
   // Queue tasks with streaming operator which is a synonym for push().
   if (myRank == 0) // manager node send task to worker node
   {
	#ifdef USE_BOOST_MPI
	mpi::timer       seqTimer;
	#else
	double starttime, endtime; 
   	starttime = MPI_Wtime(); 
	#endif	
	int sourceRank=0, taskTag=0, stepLength =2, taskCounterT1=0, taskCounterT2=0, taskCounterT3=0; 
	int iRank,destRank, taski, aveTaskNum, leftTaskNum =0, taskOperand, startRank=1;
	int eachRank, recvReqIndex, maxTaskNumPerNode, taskIndexT1=0;
	int tempResult1=0; int tempResult2=0;
	// define a task package to get results
	if (mySize <= 2) // only 1 CPU sequential
	{
		maxTaskNumPerNode  = taskTotalNum;
	}
	else if ( mySize > 2) // more than 2 CPUs parallel
	{
		if (taskTotalNum % (availableRank) == 0)
			maxTaskNumPerNode = taskTotalNum/availableRank ;
		else 
			maxTaskNumPerNode = (taskTotalNum - taskTotalNum % availableRank)/availableRank +1 ;	
	}
		
	// mpi request for send and recv asynchronous communication
	//vector<bool> recvReqMark(taskTotalNum);
	//vector<bool>::iterator recvReqMarkItr;
  	
	cout << "In myNeplanTaskScheduler(), I am rank " << myRank << ", availableRank is " << availableRank << ", maxTaskNumPerNode is "  << maxTaskNumPerNode << ". \n\n " << endl;
	// manager decided the num of gen
	tempResult = 0; int reqCounter=1; t2FinishStatusCounter =0;
	// T1 T2 task has different aveTaskPerNode
	int resultSourceRank, t1Flag=0, t2Flag=0, taskT1ACounter=0, taskT1BCounter=0; 
	int myUpStreamTaskTag=0, myDownStreamTaskTag=0 , taskT2ACounter=0, taskT2BCounter=0;
	bool t2FinishStatus = BOOLFALSE;
	int masterTaskNum =0 ;
	vector<int> taskPerRank; 
	vector<int> taskArray;
	
	tempResult = 0;
	vector<double>::iterator xitr;
	static int seqCounter = 0 ;
	// sequential algorithm only 1 CPU
	if (availableRank <= 0)
	{
		#ifdef PRINTFOUT 
		
		fprintf(pFile, "In node %d , taskTotalNum = %d, availableRank = %d \n\n", myRank, taskTotalNum, availableRank ); 
		fprintf(pFile,"========================================================= \n\n") ;
			
		fprintf(pFile, " node | task# | OperA| OperB| Oper| result \n") ;
		#endif

		#ifdef DEBUG_myNeplanTaskScheduler

		cout << "In myNeplanTaskScheduler(), availableRank is " << availableRank << " , I am rank " << myRank << ", the seq run for Gen myGenerationNum " << myGenerationNum << "cand tag " << genCandTag << " I am before ImportIndices " <<  " \n\n " << endl;
		#endif
		// Read global parameters
		//ReadParameters("data/parameters.csv");
	
		// Import indices to export data
		//ImportIndices();
		#ifdef DEBUG_myNeplanTaskScheduler
		cout << "In myNeplanTaskScheduler(), availableRank is " << availableRank << " , I am rank " << myRank << ", the seq run for Gen " << genCandTag << " I am before CPLEX netplan \n\n " << endl;
		#endif
		// Declare variables to store the optimization model
		/*  djx do this in evaluatePop()
		if (seqCounter == 0)
		{
		CPLEX netplan;
		#ifdef DEBUG_myNeplanTaskScheduler
		cout << "In myNeplanTaskScheduler(), availableRank is " << availableRank << " , I am rank " << myRank << ", the seq run for Gen " << genCandTag << " I am before netplan.LoadProblem \n\n " << endl;
		#endif
		// Read master and subproblems
		netplan.LoadProblem();
		++seqCounter;
		}
		*/
		#ifdef DEBUG_myNeplanTaskScheduler
		cout << "In myNeplanTaskScheduler(), availableRank is " << availableRank << " , I am rank " << myRank << ", the seq run for Gen " << genCandTag << " I am before events[] \n\n " << endl;
		#endif
		
		// Vector of capacity losses for events
		double events[(SLength[0] + IdxCap.GetSize()) * (Nevents+1)];
		#ifdef DEBUG_myNeplanTaskScheduler
		cout << "In myNeplanTaskScheduler(), availableRank is " << availableRank << " , I am rank " << myRank << ", the seq run for Gen " << genCandTag << " I am before ReadEvents \n\n " << endl;
		#endif
		
		ReadEvents(events, "prepdata/bend_events.csv");
	
		//for (int jji = 0 ; jji < popSize;  jji++)
		//{	
		//sleep(SLEEPTIME);
		#ifdef DEBUG_myNeplanTaskScheduler
		cout << "In myNeplanTaskScheduler(), availableRank is " << availableRank << " , I am rank " << myRank << ", the seq run for Gen " << genCandTag << " I am before nsga2->evaluatePop \n\n " << endl;
		#endif

		// nsga2->evaluatePop(nsga2->child_pop, jji, nconNum , 0); // int paraFlag =0 for seq, evaluate different ind indexed by jji per time
		//nsga2->evaluatePop(nsga2->child_pop, events); 	// master solve all pop ind seq. 
		nsga2->evaluatePop(myChildpop, events); 	// master solve all pop ind seq.
				
		//}	
		#ifdef DEBUG_myNeplanTaskScheduler			
 
		endtime = MPI_Wtime();
		cout << "In myNeplanTaskScheduler(), availableRank is " << availableRank << " , I am rank " << myRank << ", the seq run for Gen " << genCandTag << ", after nsga2->evaluatePop(nsga2->child_pop, events) I use time "<< endtime - starttime << " seconds,  \n\n " << endl;
		
		#endif 
			
	}
	else if (availableRank > 0) // parallel algorithm with 2 cases T1 T2
	{
		// drop all elements of the result receiver data structure to avoid the previous generation results impact current generation 
		//resultTaskPackageT12.clear();
		#ifdef PRINTFOUT
		fprintf(paraT1File, "In node %d , taskTotalNum = %d, availableRank = %d, genCandTag = %s \n\n", myRank, taskTotalNum, availableRank , genCandTag); 
		fprintf(paraT1File,"========================================================= \n\n") ;
		fprintf(paraT1File, "  node | task# | genCandTag \n") ;
		#endif
		
		// master sends out task by real value and collects results by pointer to avoid truncate error. 
		
		if ( taskTotalNum <= availableRank+1) // T1 
		{
			// master sends a message about task assignments to each worker so that they know what tags they should use to recev and send task to mastr. 
			// for T1, worker can use their rank ID to recev and send task.
			// for T2, worker needs to use the head message from master to decide their tags for recv and send with master.
			if (genCandTag == 'A')
			{
				sendReqsT1AVec.clear(); sendReqsT1AXVec.clear();
				recvReqsT1AVec.clear();
			}
			else if (genCandTag == 'B')
			{
				sendReqsT1BVec.clear(); sendReqsT1BXVec.clear();
				recvReqsT1BVec.clear();		
			}
			
			t1Flag = 1; t2Flag = 0;myT1Flag=1; myT2Flag=0;
			taskT1ACounter=0; taskT1BCounter=0; taskCounterT1=0;
			masterTaskNum = 1;	
			
			// taskTotalNum is popsize 
			// myPopParaVec is a vector with individual as component
			for (iRank = 1, mToWorkerVecItr = myPopParaVec.begin()+1; (mToWorkerVecItr < myPopParaVec.end()) || (iRank < taskTotalNum) ; iRank++, mToWorkerVecItr++)
			{
				#ifdef DEBUG_myNeplanTaskScheduler
				cout << "In myNetplanTaskScheduler(), I am rank " << myRank << " , this is T1, iRank is " << iRank << " , taskTotalNum is " << taskTotalNum << " . \n\n " << endl;	
				#endif
				
 				//xdataV.clear(); // drop all elements in the vector
				destRank =  iRank;
				sourceRank = myRank;
				int couter =0 ; 
				if (genCandTag == 'B')
				{
					(*mToWorkerVecItr).taskTag = iRank+(int)genCandTag+ taskTotalNum;
					message_para_to_workersSt.taskTag = iRank+(int)genCandTag+ taskTotalNum;
				}
				else if (genCandTag == 'A')
				{
					(*mToWorkerVecItr).taskTag = iRank+(int)genCandTag;
					message_para_to_workersSt.taskTag = iRank+(int)genCandTag;
				}		

				message_para_to_workersSt.crowd_dist = 0.0 ;
				message_para_to_workersSt.constr_violation = 0.0 ;	
				//message_para_to_workers_sendTemp = new message_para_to_workersT;	
	
				//myPopParaVec.push_back(*message_para_to_workers_sendTemp);
				
				(*mToWorkerVecItr).packNum = 1;
				message_para_to_workersSt.packNum = 1; 

				(*mToWorkerVecItr).taskNum = iRank;
				message_para_to_workersSt.taskNum = iRank;
						
				(*mToWorkerVecItr).generation = generationNum;
				message_para_to_workersSt.generation = generationNum;

				(*mToWorkerVecItr).genCandTag = genCandTag; // A or B	
				message_para_to_workersSt.genCandTag = genCandTag;
	
				(*mToWorkerVecItr).indBegin = (iRank);
				message_para_to_workersSt.indBegin = (iRank);

				(*mToWorkerVecItr).indEnd = (iRank);
				message_para_to_workersSt.indEnd = (iRank);

				(*mToWorkerVecItr).indNum = (iRank);
				message_para_to_workersSt.indNum = (iRank);

				//int xTag = taskTag + popsize;
				// xdata xreal, obj, con, for worker to work on
				#ifdef DEBUG_myNeplanTaskScheduler
				cout << "In myNetplanScheduler(), At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T1 task " << iRank << ", out of total tasks "  << taskTotalNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << " , the task tag is " << (*mToWorkerVecItr).taskTag << ", try the value  (*mToWorkerVecItr).xreal[0] is " <<  (*mToWorkerVecItr).xreal[0] << ", try the value  (*mToWorkerVecItr).xreal[9] is " <<  (*mToWorkerVecItr).xreal[9] << " , sizeof((*mToWorkerVecItr).xreal) is " << sizeof((*mToWorkerVecItr).xreal) << " , ((*mToWorkerVecItr).xreal).size() is " << ((*mToWorkerVecItr).xreal).size() << " , before initalize xdataV " << ", (*mToWorkerVecItr).indNum is "<< (*mToWorkerVecItr).indNum << " , xdataV.size() is " << xdataV.size() << " , sizeof(xdataV) is " << sizeof(xdataV)  << " . \n\n" << endl; 
				#endif
				
				xdataV[0] = (*mToWorkerVecItr).indNum;
				int jj =1 ; 
				for (xitr = ((*mToWorkerVecItr).xreal).begin() ; xitr != ((*mToWorkerVecItr).xreal).end(); jj++, xitr++)
				{
					xdataV[jj]=(*xitr); // size of xreal is nreal
					++couter;
  				}
				for (xitr = ((*mToWorkerVecItr).xbin).begin() ; xitr != ((*mToWorkerVecItr).xbin).end(); jj++, xitr++)
				{
					xdataV[jj]=(*xitr); // size of nbin
					++couter;
  				}
				for (xitr = ((*mToWorkerVecItr).obj).begin() ; xitr != ((*mToWorkerVecItr).obj).end(); jj++, xitr++)
				{
					xdataV[jj] = (*xitr); // size of obj is nobj
					++couter;
  				}
				for (xitr = ((*mToWorkerVecItr).constr).begin() ; xitr != ((*mToWorkerVecItr).constr).end(); jj++, xitr++)
				{
					xdataV[jj]=(*xitr); // size of ncon
					++couter;
  				}
				

				#ifdef DEBUG_myNeplanTaskScheduler
				cout << "In myNetplanScheduler() check popVec, At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T1 task " << iRank << ", out of total tasks "  << taskTotalNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << " , the task tag is " << (*mToWorkerVecItr).taskTag << ", try the value  (*mToWorkerVecItr).xreal[0] is " <<  (*mToWorkerVecItr).xreal[0] << ", try the value  (*mToWorkerVecItr).xreal[9] is " <<  (*mToWorkerVecItr).xreal[9] << " , sizeof((*mToWorkerVecItr).xreal) is " << sizeof((*mToWorkerVecItr).xreal) << " , ((*mToWorkerVecItr).xreal).size() is " << ((*mToWorkerVecItr).xreal).size() << " , xdataV push_back counter is " << couter << ", (*mToWorkerVecItr).indNum is "<< (*mToWorkerVecItr).indNum << " , xdataV.size() is " << xdataV.size() << " , sizeof(xdataV) is " << sizeof(xdataV) << " xdataV[0] is " << xdataV[0] << " . \n\n" << endl; 
				#endif
				//Rha::BasicScheduler()<<(&task1, world, destRank, myTaskTag); 
					//resultSourceRank = destRank;
				myUpStreamTaskTag = (*mToWorkerVecItr).taskTag;

				#ifdef DEBUG_myNeplanTaskScheduler
				cout << "In myNetplanScheduler(), At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T1 task " << iRank << ", out of total tasks "  << taskTotalNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << " , I am after update myUpStreamTaskTag \n\n" << endl;
				#endif

				#ifdef DEBUG_myNeplanTaskScheduler
				cout << "In myNetplanScheduler(), At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T1 task " << iRank << ", out of total tasks "  << taskTotalNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << " , I am after resultTaskPackageT1.push_back(*message_to_master_temp) \n\n" << endl;
				#endif

				// master collects results from workers
				// each component of message_to_master_Vec is a pointer to a struct
				#ifdef DEBUG_myNeplanTaskScheduler			
				cout << "In myNetplanScheduler(), At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T1 task " << iRank << ", out of total tasks "  << taskTotalNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << " , I am before  MPI_Irecv(&(resultTaskPackageT12[iRank][0]),objSize, mpi_double, destRank, myUpStreamTaskTag, MPI_COMM_WORLD, &mToMasterT1Req), the Irecv's source rank is" << destRank << " , and the myUpStreamTaskTag is " << myUpStreamTaskTag << " , objSize is " << objSize << " \n\n" << endl;
				#endif

				// master post recv result from workers
				mToMasterT1Req = MPI::COMM_WORLD.Irecv(&(resultTaskPackageT12[iRank][0]), objSize, MPI_DOUBLE, destRank, myUpStreamTaskTag);
				 
				/*
				for (int ii = 0 ; ii < objSize;  ii++)
				{
					resultTaskPackageT12[iRank-1][ii] = recvArray[ii]; 

					#ifdef DEBUG_myNeplanTaskScheduler			
					cout << "In myNetplanScheduler(), At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T1 task " << iRank << ", out of total tasks "  << taskTotalNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << " , I am being after MPI_Irecv(&(recvArray[0]), objSize, MPI_DOUBLE, destRank, myUpStreamTaskTag, MPI_COMM_WORLD, &mToMasterT1Req), the Irecv's source rank is " << destRank << " , and the myUpStreamTaskTag is " << myUpStreamTaskTag << " , recvArray[" << ii <<  "] is " << recvArray[ii]   << " , resultTaskPackageT12[" << iRank-1  << "][" << ii<< "] is " <<  resultTaskPackageT12[iRank-1][ii] << " \n\n" << endl;
					#endif

				}
				delete 	[] recvArray;	
				*/

				if (genCandTag == 'A')
				//if (!strcmp(genCandTag,'A'))
				{	
					recvReqsT1AVec.push_back(mToMasterT1Req);
					++taskT1ACounter;
				}	
				// else if ((int)genCandTag == 'B')
				else if (genCandTag == 'B')
				{	
					recvReqsT1BVec.push_back(mToMasterT1Req);
					++taskT1BCounter;
				}
				#ifdef DEBUG_myNeplanTaskScheduler
				cout << "In myNetplanScheduler(), At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T1 task " << iRank << ", out of total tasks "  << taskTotalNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << ", and sizeof(*mToWorkerVecItr) is " << sizeof(*mToWorkerVecItr) << " , sizeof((*mToWorkerVecItr).xreal) is " <<  sizeof((*mToWorkerVecItr).xreal) << ", and sizeof(double) is " << sizeof(double) << " , I have run after   MPI_Irecv(&(resultTaskPackageT12[iRank-1]), objSize, MPI_DOUBLE, destRank, myUpStreamTaskTag, MPI_COMM_WORLD, &mToMasterT1Req) and also pushback mToMasterT1Req , taskT1ACounter is " << taskT1ACounter << " , taskT1BCounter is "<< taskT1BCounter  << " , the recvReqsT1AVec.size()  is " << recvReqsT1AVec.size() << " , recvReqsT1BVec.size() is " << recvReqsT1BVec.size() << " \n\n" << endl;
				#endif
				
  				// master sends out a T1 task to worker
				//Rha::BasicScheduler()<<(boost::bind(taskFromMaster, destRank, sourceRank, downStreamTaskTag, world, *mToWorkerVecItr));
				
				#ifdef USE_BOOST_MPI
				//mToWorkerT1Req = world.isend(destRank, downStreamTaskTag, mToWorkerVecItr);
					
				#else 
				#ifdef DEBUG_myNeplanTaskScheduler
				cout << "In myNetplanScheduler(), At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T1 task " << iRank << ", out of total tasks "  << taskTotalNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << " , I am after taskPerRank.push_back(1), iRank is " << iRank << ", I am before MPI_Isend(&(*mToWorkerVecItr) \n\n" << endl;
				#endif
				
				// master send head info to workers
				MPI_Isend(&message_para_to_workersSt, 1, Mpara_to_workers_type,destRank, downStreamTaskTag, MPI_COMM_WORLD, &mToWorkerT1Req);
				
				MPI_Isend(&xdataV[0], 1, xdata_to_workers_type, destRank, (*mToWorkerVecItr).taskTag, MPI_COMM_WORLD, &mToWorkerT1XReq);
				
				#ifdef DEBUG_myNeplanTaskScheduler
				cout << "In myNetplanScheduler(), At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T1 task " << iRank << ", out of total tasks "  << taskTotalNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << " , I am after   MPI_Isend(&xdataV, xdataV.size() , MPI_DOUBLE, destRank, (*mToWorkerVecItr).taskTag, MPI_COMM_WORLD, &mToWorkerT1XReq), the destRank is " << destRank << " , the downStreamTaskTag is " << downStreamTaskTag << " , the (*mToWorkerVecItr).taskTag is " << (*mToWorkerVecItr).taskTag << " , xdataV.size() is " << xdataV.size() << " , sizeof(xdataV) is " << sizeof(xdataV) << " \n\n" << endl;
				#endif
				
				#endif
				if (genCandTag == 'A')
				{					
					sendReqsT1AVec.push_back(mToWorkerT1Req);
					sendReqsT1AXVec.push_back(mToWorkerT1XReq);
				}
				else if (genCandTag == 'B')
				{					
					sendReqsT1BVec.push_back(mToWorkerT1Req);
					sendReqsT1BXVec.push_back(mToWorkerT1XReq);
				} 
				#ifdef DEBUG_myNeplanTaskScheduler
				//cout << "In myNetplanScheduler(), At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T1 task " << iRank << ", out of total tasks "  << taskTotalNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << " , I am after   MPI_Isend(&(*mToWorkerVecItr), 1, destRank, downStreamTaskTag, MPI_COMM_WORLD, &mToWorkerT1Req) and also push_back mToWorkerT1Req , the sendReqsT1AVec.size() is " << sendReqsT1AVec.size() << " , and sendReqsT1BVec.size() is "  << sendReqsT1BVec.size()  << " \n\n" << endl;
				#endif
	
				//taskPerRank.push_back(1);
				// post recv to get the result for the above send
				#ifdef DEBUG_myNeplanTaskScheduler
				cout << "In myNetplanScheduler(), At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T1 task " << iRank << ", out of total tasks "  << taskTotalNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << " , I am after taskPerRank.push_back(1), iRank is " << iRank <<  " . \n\n" << endl;
				#endif
				++taskCounterT1;
				
				#ifdef PRINTFOUT
				//fprintf(paraT1File, " %3d   %6d   %6d     %6d    %6d    \n ", iRank, iRank, myTaskPackage.getOperandA(), myTaskPackage.getOperandB() ,  myTaskPackage.getTaskOperationType() ) ;
				#endif
				
			} 
			/*
			while(iRank <= availableRank)
			{
				taskPerRank.push_back(0);
				++iRank;
			}*/
			// send the first grouped partitioned tasks to master 
			//xRealPr = &((*mToWorkerVecItr).xreal); // it is a vector 
			//cplexType myNetplan;
			//cplexType* netplan; netplan= &myNetplan; netplan->LoadProblem();
			masterRunTask(myChildpop, resultTaskPackageT12[0],0, objSize, myGenerationNum, genCandTag);
			//masterRunTask(myChildpop, resultTaskPackageT12[0],0, objSize, myGenerationNum, genCandTag, netplan);
			//delete netplan; 
		}
		else if (taskTotalNum > availableRank+1)  //T2 ----------------------------
		{
			#ifdef DEBUG_myNeplanTaskScheduler
			cout << "In myNetplanTaskScheduler(), I am rank " << myRank << " in (taskTotalNum > availableRank+1)  T2 before sending task to workers \n\n" << endl;
			#endif
			int counter =0 ;
			#ifdef PRINTFOUT
			fprintf(paraT2File, "In node %d , taskTotalNum = %d, availableRank = %d \n\n", myRank, taskTotalNum, availableRank ); 
			fprintf(paraT2File,"========================================================= \n\n") ;
			fprintf(paraT2File, "  node | task# | OperandA | OperandB | Operation | result \n") ;
			#endif
			if (genCandTag == 'A')
			{
				sendReqsT2AVec.clear(); sendReqsT2AXVec.clear();
				recvReqsT2AVec.clear();
			}
			else if (genCandTag == 'B')
			{	
				sendReqsT2BVec.clear(); sendReqsT2BXVec.clear();
				recvReqsT2BVec.clear();
			}

			t1Flag = 0; t2Flag = 1; myT1Flag=0; myT2Flag=1;
			taskT2ACounter=0; taskT2BCounter=0; 
			
			// taskTotalNum is total pop size
			// compute num of tasks per node
			//leftTaskNum = (taskTotalNum % availableRank);
			
			leftTaskNum 	 = (taskTotalNum % (availableRank+1));	
			// some of non-0-rank nodes will be assigned a task from left tasks
			// non-0-rank nodes ID : 1 ---- (leftTaskNum -1) 
 			//taskNumToOtherNodes = leftTaskNum -1 ; 	
			//aveTaskNum = (taskTotalNum - leftTaskNum)/availableRank;
			aveTaskNum = (taskTotalNum - leftTaskNum)/(availableRank+1);
			if (leftTaskNum == 0)
			{
				masterTaskNum = aveTaskNum;
			}
			else	
			{
				masterTaskNum = aveTaskNum+1;	
			}
			#ifdef DEBUG_myNeplanTaskScheduler
			cout << "In myNetplanTaskScheduler(), I am rank " << myRank << " in (taskTotalNum > availableRank)  T2 before sending task to workers leftTaskNum = " << leftTaskNum << " aveTaskNum = " << aveTaskNum << " \n\n" << endl;
			#endif

			//  send evenly partitioned tasks to master workers
			for (iRank = 0; iRank < availableRank ; iRank++) 
			{
				//iRank = 0 ;
				taskCounterT2=0;
				destRank = iRank+1;
				taskPerRank.push_back(aveTaskNum);
				beginIndex = iRank * aveTaskNum;
				taskArray.push_back(0);// each rank's task num
				
				for (taski = iRank * aveTaskNum, mToWorkerVecItr = myPopParaVec.begin() + iRank * aveTaskNum; taski < beginIndex + aveTaskNum ; taski++, mToWorkerVecItr++)
				{	
					sourceRank = myRank;
					//taskTag = downStreamTaskTag;
					//(*mToWorkerVecItr).packNum = 1;			
					//(*mToWorkerVecItr).endRun = 0;
					message_para_to_workersSt.crowd_dist = 0.0 ;
					message_para_to_workersSt.constr_violation = 0.0 ;	
					if (leftTaskNum == 0)
					{
						(*mToWorkerVecItr).packNum = aveTaskNum;
						message_para_to_workersSt.packNum = aveTaskNum;
					} 
					else // non evenly scheduled 
					{	if (destRank <= leftTaskNum-1) 
						{
							(*mToWorkerVecItr).packNum = aveTaskNum+1;
							message_para_to_workersSt.packNum = aveTaskNum+1;
						}
						else
						{
							(*mToWorkerVecItr).packNum = aveTaskNum;
							message_para_to_workersSt.packNum = aveTaskNum;
						}	
					}
					
					(*mToWorkerVecItr).taskNum = taski+1;
					message_para_to_workersSt.taskNum = taski+1;

					if (genCandTag == 'B')
					{
						(*mToWorkerVecItr).taskTag = taski+1+(int)genCandTag+ taskTotalNum;
						message_para_to_workersSt.taskTag = taski+1+(int)genCandTag+ taskTotalNum;
					}
					else if (genCandTag == 'A')
					{
						(*mToWorkerVecItr).taskTag = taski+1+(int)genCandTag;
						message_para_to_workersSt.taskTag = taski+1+(int)genCandTag;
					}					
					(*mToWorkerVecItr).generation = myGenerationNum;	
					message_para_to_workersSt.generation = myGenerationNum;
		
					(*mToWorkerVecItr).genCandTag = genCandTag; // A or B	
					message_para_to_workersSt.genCandTag = genCandTag; // A or B
					// partition workload according to rank num
					// each worker only works on 
					(*mToWorkerVecItr).indBegin = (aveTaskNum*iRank);
					message_para_to_workersSt.indBegin = (aveTaskNum*iRank);

					(*mToWorkerVecItr).indEnd = (aveTaskNum*(iRank+1)-1);
					message_para_to_workersSt.indEnd = (aveTaskNum*(iRank+1)-1);
						
					(*mToWorkerVecItr).indNum = taski;
					message_para_to_workersSt.indNum = taski;
					
					// master sends out a T2 package to worker nodes
					
					#ifdef DEBUG_myNeplanTaskScheduler
					cout << "In myNetplanScheduler() check data before send out, At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T2 task " << iRank << ", out of total tasks "  << taskTotalNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << " , the task tag is " << (*mToWorkerVecItr).taskTag << ", try the value  (*mToWorkerVecItr).xreal[0] is " <<  (*mToWorkerVecItr).xreal[0] << " \n\n" << endl;  
					//" , try the value  (*mToWorkerVecItr).xreal[1] is " << (*mToWorkerVecItr).xreal[1] << " , (*mToWorkerVecItr).xreal[2] is "<< (*mToWorkerVecItr).xreal[2] <<  " , (*mToWorkerVecItr).xreal[3] is " << (*mToWorkerVecItr).xreal[3] << " , (*mToWorkerVecItr).xreal[4] is " << (*mToWorkerVecItr).xreal[4] << " , (*mToWorkerVecItr).xreal[5] is " << (*mToWorkerVecItr).xreal[5] <<  " , (*mToWorkerVecItr).xreal[6] " << (*mToWorkerVecItr).xreal[6] << " , (*mToWorkerVecItr).xreal[7] is " << (*mToWorkerVecItr).xreal[7] << " , (*mToWorkerVecItr).xreal[8] is " << (*mToWorkerVecItr).xreal[8]  << ", try the value  (*mToWorkerVecItr).xreal[9] is " <<  (*mToWorkerVecItr).xreal[9] << " , sizeof((*mToWorkerVecItr).xreal) is " << sizeof((*mToWorkerVecItr).xreal) << " , ((*mToWorkerVecItr).xreal).size() is " << ((*mToWorkerVecItr).xreal).size()  << ", (*mToWorkerVecItr).indNum is "<< (*mToWorkerVecItr).indNum << " , xdataV.size() is " << xdataV.size() << " , sizeof(xdataV) is " << sizeof(xdataV) << " , message_para_to_workersSt.taskTag is " << message_para_to_workersSt.taskTag << " , message_para_to_workersSt.generation is " << message_para_to_workersSt.generation << " , message_para_to_workersSt.packNum is " << message_para_to_workersSt.packNum << " , message_para_to_workersSt.indEnd is " << message_para_to_workersSt.indEnd <<  " , the destRank is " << destRank << " . \n\n" << endl; 
					#endif
					// send head info
					
					MPI_Isend(&message_para_to_workersSt, 1, Mpara_to_workers_type, destRank, downStreamTaskTag+taskCounterT2, MPI_COMM_WORLD,  &mToWorkerT2Req);

					xdataV[0] = (*mToWorkerVecItr).indNum;
					int jj = 1;
					for (xitr = ((*mToWorkerVecItr).xreal).begin() ; xitr != ((*mToWorkerVecItr).xreal).end(); jj++, xitr++)
					{
						xdataV[jj] = (*xitr); // size of xreal is nreal
						++counter;
					}
					for (xitr = ((*mToWorkerVecItr).xbin).begin() ; xitr != ((*mToWorkerVecItr).xbin).end(); jj++, xitr++)
					{
						xdataV[jj]=(*xitr); // size of nbin
						++counter;
  					}
					for (xitr = ((*mToWorkerVecItr).obj).begin() ; xitr != ((*mToWorkerVecItr).obj).end(); jj++, xitr++)
					{
						xdataV[jj] = (*xitr); // size of obj is nobj
						++counter;
					}
					for (xitr = ((*mToWorkerVecItr).constr).begin() ; xitr != ((*mToWorkerVecItr).constr).end(); jj++, xitr++)
					{
						xdataV[jj] = (*xitr); // size of ncon
						++counter;
					}

					// send xdata
					//MPI_Isend(&xdataV[0], xdataV.size(), MPI_DOUBLE, destRank, (*mToWorkerVecItr).taskTag,MPI_COMM_WORLD,  &mToWorkerT2XReq);
					MPI_Isend(&xdataV[0], 1, xdata_to_workers_type, destRank, (*mToWorkerVecItr).taskTag,MPI_COMM_WORLD,  &mToWorkerT2XReq);
					#ifdef DEBUG_myNeplanTaskScheduler
					cout << "In myNeplanTaskScheduler(), At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << ", this is T2 evenly part after sending para and xdata, the aveTaskNum is " << aveTaskNum << ", leftTaskNum is " << leftTaskNum << ", the gen cand tag is " << (*mToWorkerVecItr).genCandTag << ", setIndBegin is" << (*mToWorkerVecItr).indBegin << ", setIndEnd is " << (*mToWorkerVecItr).indEnd << ". This is task "<< taski+1 <<  ", out of total tasks "  << taskTotalNum << " , it is sent to " << destRank << " , the check sentout xdada taskTag is " << (*mToWorkerVecItr).taskTag << " , the check xdataV[0] is " << xdataV[0] << " para downStreamTaskTag+taskCounterT2 is " << downStreamTaskTag+taskCounterT2 << " . \n\n" << endl; 
					#endif
					// https://rqchp.ca/modules/cms/checkFileAccess.php?file=local.rqchpweb_udes/mpi/exemples_cpp/ex06_ISend-IRecv_EN.cpp
				
						
					if (genCandTag == 'A')
					{
						sendReqsT2AVec.push_back(mToWorkerT2Req);
						sendReqsT2AXVec.push_back(mToWorkerT2XReq);
						++taskT2ACounter;
					}
					else if (genCandTag == 'B')
					{
						sendReqsT2BVec.push_back(mToWorkerT2Req);
						sendReqsT2BXVec.push_back(mToWorkerT2XReq);
						++taskT2BCounter;
					}
					// master post a recv for a T2 task assigned with a distinct tag to get result from worker
					resultSourceRank = destRank;

					myUpStreamTaskTag = (*mToWorkerVecItr).taskTag;
					// allocate space for receiver
									
					// master collect results from workers
					
					mToMasterT2Req = MPI::COMM_WORLD.Irecv(&(resultTaskPackageT12[taski][0]), objSize, MPI_DOUBLE, destRank, myUpStreamTaskTag);	

					// problem is here, Irecv is nonblocking, but the recvArray is a temporary  
					// resultTaskPackageT12 may not get result just after Irecv.
						
					if (genCandTag == 'A')
					{	
						recvReqsT2AVec.push_back(mToMasterT2Req);
					}
					else if (genCandTag == 'B')
					{
						recvReqsT2BVec.push_back(mToMasterT2Req);
					}
					#ifdef DEBUG_myNeplanTaskScheduler
					cout << "In myNeplanTaskScheduler(), At generation " << (*mToWorkerVecItr).generation << ", My rank is " << myRank << " , in T2 evenly part after posting isend, I post irecv, the (resultTaskPackageT12[" << taski << "].size() is " << (resultTaskPackageT12[taski].size()) << ", I need to get the task result with tag as "<< myUpStreamTaskTag << " from node " << resultSourceRank << " , and sizeof(resultTaskPackageT12[" << taski << "][" << 0 << "] is " << sizeof(resultTaskPackageT12[taski][0]) << " , genCandTag is " << genCandTag << " , recvReqsT2AVec.size() is " << recvReqsT2AVec.size() << " , recvReqsT2BVec.size() is " << recvReqsT2BVec.size() << " . \n\n" << endl;  
					#endif	
					// test deadslock 
					// post recv to get the result for the above send
					//resultSourceRank = destRank;
					// taskCounterT2 record num of tasks assigned to all nodes
					++taskCounterT2;// task counter for type 2 with possible left tasks
					++taskCounterT3;
					// record num of tasks assigned to each rank
					taskArray.push_back(taskArray[iRank] + 1);
					
					#ifdef PRINTFOUT
					fprintf(paraT2File, " %3d   %6d   \n ", destRank, taski) ;
					#endif
				}
				
 				taskCounterT3 = 0;
				
			}
			
			cout << "I am rank " << myRank << ", I have sent "  << taski << "tasks out of total tasks " << taskTotalNum << " leftTaskNum is " << leftTaskNum << "\n\n" << endl;

		startRank =1;	// send left task from worker 1 until left ones are all send out. 
		destRank = 0;
		 
		if (leftTaskNum-1 > 0)
		{		
			masterTaskNum = aveTaskNum+1;
			// the new iterator to index the left pop para in the case that # of task%rvailablerank != 0 
			//for (iRank = 0; iRank < leftTaskNum ; iRank++) //  send evenly partitioned tasks to workers
			//{
			//for (taski = (taskTotalNum - leftTaskNum), mToWorkerVecItr1 = mToWorkerVecItr; taski < taskTotalNum-masterTaskNum; mToWorkerVecItr1++, taski++) // send left tasks to nodes
			for (mToWorkerVecItr1 = mToWorkerVecItr; taski < taskTotalNum-masterTaskNum; mToWorkerVecItr1++, taski++) // send left tasks to nodes	
			{
				//destRank = iRank+1;
				++destRank;
				sourceRank = myRank;
				//TaskPackage myTaskPackage(world); 
				//(*mToWorkerVecItr).packNum = 1;
				// ending package signal to workers
				message_para_to_workersSt.crowd_dist = 0.0 ;
				message_para_to_workersSt.constr_violation = 0.0 ;

				if (leftTaskNum == 0)
				{
					(*mToWorkerVecItr).packNum = aveTaskNum;
					message_para_to_workersSt.packNum = aveTaskNum;
				} 
				else // non evenly scheduled 
				{	if (destRank <= leftTaskNum-1) 
					{
						(*mToWorkerVecItr).packNum = aveTaskNum+1;
						message_para_to_workersSt.packNum = aveTaskNum+1;
					}
					else
					{
						(*mToWorkerVecItr).packNum = aveTaskNum;
						message_para_to_workersSt.packNum = aveTaskNum;
					}	
				}

				//(*mToWorkerVecItr).packNum = aveTaskNum+1;
				//message_para_to_workersSt.packNum = aveTaskNum+1;

				(*mToWorkerVecItr1).taskNum = taski+1;
				message_para_to_workersSt.taskNum = taski+1;

				if (genCandTag == 'B')
				{
					(*mToWorkerVecItr1).taskTag = taski+1+(int)genCandTag+ taskTotalNum;
					message_para_to_workersSt.taskTag = taski+1+(int)genCandTag+ taskTotalNum;	
				}
				else if (genCandTag == 'A')
				{
					(*mToWorkerVecItr1).taskTag = taski+1+(int)genCandTag;
					message_para_to_workersSt.taskTag = taski+1+(int)genCandTag;
				}					
				(*mToWorkerVecItr1).generation = myGenerationNum;
				message_para_to_workersSt.generation = myGenerationNum;
				
				(*mToWorkerVecItr1).genCandTag = genCandTag; // A or B	
				message_para_to_workersSt.genCandTag = genCandTag;	
				// partition workload according to rank num
				// each worker only works on 
				(*mToWorkerVecItr1).indBegin = taski; // (aveTaskNum*availableRank + iRank);
				message_para_to_workersSt.indBegin = taski ; //(aveTaskNum*availableRank + iRank);

				(*mToWorkerVecItr1).indEnd = taski; //(aveTaskNum*availableRank + iRank);
				message_para_to_workersSt.indEnd = taski; //(aveTaskNum*availableRank + iRank);

				(*mToWorkerVecItr1).indNum = taski;
				message_para_to_workersSt.indNum = taski;
				#ifdef DEBUG_myNeplanTaskScheduler
				cout << "In myNeplanTaskScheduler(), At generation " << (*mToWorkerVecItr1).generation << ", My rank is " << myRank << ", this is T2 unevenly, send left task, the aveTaskNum is " << aveTaskNum << ", sending left task 1 task per node, leftTaskNum is " << leftTaskNum << ", the gen cand tag is " << (*mToWorkerVecItr1).genCandTag << ", setIndBegin is" << (*mToWorkerVecItr1).indBegin << ", setIndEnd is " << (*mToWorkerVecItr1).indEnd << ". This is task "<< taski+1 <<  ", out of total tasks "  << taskTotalNum << " , it is sent to " << destRank << " , the sentout xdada taskTag is " << (*mToWorkerVecItr1).taskTag << " , message_para_to_workersSt.packNum is " <<  message_para_to_workersSt.packNum << " , destRank is " << destRank << " , the task sent by taskTag downStreamTaskTag+taskCounterT2 " << downStreamTaskTag+taskCounterT2 << " . \n\n" << endl; 
				#endif	
				// master sends out a T2 package to worker nodes
					
				// send head info
				MPI_Isend(&message_para_to_workersSt, 1, Mpara_to_workers_type, destRank, downStreamTaskTag+taskCounterT2, MPI_COMM_WORLD,  &mToWorkerT2Req);
	
				xdataV[0] = (*mToWorkerVecItr1).indNum;
				int jt = 1;
				for (xitr = ((*mToWorkerVecItr1).xreal).begin() ; xitr != ((*mToWorkerVecItr1).xreal).end(); jt++, xitr++)
				{
					xdataV[jt] = (*xitr); // size of xreal is nreal
					++counter;
				}
				for (xitr = ((*mToWorkerVecItr).xbin).begin() ; xitr != ((*mToWorkerVecItr).xbin).end(); jt++, xitr++)
				{
					xdataV[jt]=(*xitr); // size of nbin
					++counter;
  				}
				for (xitr = ((*mToWorkerVecItr1).obj).begin() ; xitr != ((*mToWorkerVecItr1).obj).end(); jt++, xitr++)
				{
					xdataV[jt] = (*xitr); // size of obj is nobj
					++counter;
				}
				for (xitr = ((*mToWorkerVecItr1).constr).begin() ; xitr != ((*mToWorkerVecItr1).constr).end(); jt++, xitr++)
				{
					xdataV[jt] = (*xitr); // size of ncon
					++counter;
				}

				// send xdata
				//MPI_Isend(&xdataV[0], xdataV.size(), MPI_DOUBLE, destRank, (*mToWorkerVecItr).taskTag,MPI_COMM_WORLD,  &mToWorkerT2XReq);
				MPI_Isend(&xdataV[0], 1, xdata_to_workers_type, destRank, (*mToWorkerVecItr1).taskTag, MPI_COMM_WORLD,  &mToWorkerT2XReq);
				#ifdef DEBUG_myNeplanTaskScheduler	
				cout << "In myNeplanTaskScheduler(), At generation " << (*mToWorkerVecItr1).generation << ", My rank is " << myRank << ", this is T2 unevenly part, the aveTaskNum is " << aveTaskNum << ", leftTaskNum is " << leftTaskNum << ", the gen cand tag is " << (*mToWorkerVecItr1).genCandTag << ", setIndBegin is" << (*mToWorkerVecItr1).indBegin << ", setIndEnd is " << (*mToWorkerVecItr1).indEnd << ". This is task "<< taski+1 <<  ", out of total tasks "  << taskTotalNum << " , it is sent to " << destRank << " , the check sentout xdada taskTag is " << (*mToWorkerVecItr1).taskTag << " , the check xdataV[0] is " << xdataV[0] << " . \n\n" << endl; 
				#endif
				// https://rqchp.ca/modules/cms/checkFileAccess.php?file=local.rqchpweb_udes/mpi/exemples_cpp/ex06_ISend-IRecv_EN.cpp
				
				// master sends out left T2 tasks to worker nodes 
				if (genCandTag == 'A')
				{
					sendReqsT2AVec.push_back(mToWorkerT2Req);
					sendReqsT2AXVec.push_back(mToWorkerT2XReq);
					++taskT2ACounter;
				}
				else if (genCandTag == 'B')
				{
					sendReqsT2BVec.push_back(mToWorkerT2Req);
					sendReqsT2BXVec.push_back(mToWorkerT2XReq);
					++taskT2BCounter;
				}
				// master post a recv for a T2 task assigned with a distinct tag to get result from worker
				resultSourceRank = destRank;
				myUpStreamTaskTag = (*mToWorkerVecItr1).taskTag;

				mToMasterT2Req = MPI::COMM_WORLD.Irecv(&(resultTaskPackageT12[taski][0]), objSize, MPI_DOUBLE, destRank, myUpStreamTaskTag);	
				#ifdef DEBUG_myNeplanTaskScheduler
				cout << "In myNeplanTaskScheduler(), At generation " << (*mToWorkerVecItr1).generation << ", My rank is " << myRank << ", this is T2, the aveTaskNum is " << aveTaskNum << ", master send leftTaskNum is " << leftTaskNum << ", the gen cand tag is " << (*mToWorkerVecItr1).genCandTag << ", setIndBegin is" << (*mToWorkerVecItr1).indBegin << ", setIndEnd is " << (*mToWorkerVecItr1).indEnd << ". This is task "<< taski+1 <<  ", out of total tasks "  << taskTotalNum << " \n\n " << endl ;
				#endif
															
				if (genCandTag == 'A')
				{	
					recvReqsT2AVec.push_back(mToMasterT2Req);
				}
				else if (genCandTag == 'B')
				{
					recvReqsT2BVec.push_back(mToMasterT2Req);
				}
				#ifdef DEBUG_myNeplanTaskScheduler	
				cout << "In myNeplanTaskScheduler(), At generation " << (*mToWorkerVecItr1).generation << ", My rank is " << myRank << " , in T2 left task, after posting isend, I post irecv, the (resultTaskPackageT12[" << taski << "].size() is " << (resultTaskPackageT12[taski].size()) << ", I need to get the task result with tag as "<< myUpStreamTaskTag << " from node " << resultSourceRank << " , and sizeof(resultTaskPackageT12[" << taski << "][" << 0 << "] is " << sizeof(resultTaskPackageT12[taski][0]) << " , genCandTag is " << genCandTag << " , recvReqsT2AVec.size() is " << recvReqsT2AVec.size() << " , recvReqsT2BVec.size() is " << recvReqsT2BVec.size() << " , master post irecv for destrank " << destRank << " , the myUpStreamTaskTag is " << myUpStreamTaskTag << " , recvReqsT2AVec.size() is " << recvReqsT2AVec.size() << " . \n\n" << endl; 
				#endif		
				++taskCounterT3;
									
				++startRank;
				//++taskCounterT2;
				
				#ifdef PRINTFOUT
				//fprintf(paraT2File, " %3d   %6d   %6d   %6d   %6d    %6d \n ", destRank, taski, myTaskPackage.getOperandA(), myTaskPackage.getOperandB() ,  myTaskPackage.getTaskOperationType(), myTaskPackage.getOperandA()+myTaskPackage.getOperandB() ) ;
				#endif
			}
			taskPerRank.push_back(taskPerRank[destRank] + leftTaskNum);
		//} // end of send left task
		} // end of if (leftTaskNum-1 != 0)

		// send the left grouped partitioned tasks to master ------------------------
		// taski get the latest value out of the last loop  --------------------------
		#ifdef DEBUG_myNeplanTaskScheduler
		cout << "In myNetplanScheduler(), I am rank " << myRank <<  " before send T2 left tasks to master itself , taski = " << taski << " taskTotalNum =  " << taskTotalNum << " leftTaskNum is " << leftTaskNum << "\n\n" << endl;
		#endif
		//struct CPLEX myNetplan; 
		//struct CPLEX* netplanPr;  netplanPr = &myNetplan; netplanPr->LoadProblem();	
		for (mToWorkerVecItr = myPopParaVec.begin() + taski; taski < taskTotalNum ; taski++, mToWorkerVecItr++)
		{	// send left inds to master
			//ImportIndices();
					 
			//masterRunTask(myChildpop, resultTaskPackageT12[taski],taski,objSize, myGenerationNum, genCandTag);
			masterRunTask(myChildpop, resultTaskPackageT12[taski],taski,objSize, myGenerationNum, genCandTag);
			//masterRunTask(myChildpop, resultTaskPackageT12[taski],taski,objSize, myGenerationNum, genCandTag, netplanPr);	
			#ifdef DEBUG_myNeplanTaskScheduler
			cout << "In myNetplanScheduler(), I am rank " << myRank << " after send and run T2 left task " << taski << " to master itself \n\n" << endl;
			#endif	
		}
		//delete netplan;	
		}  // end of if it is T2 
		#ifdef DEBUG_myNeplanTaskScheduler	
		// end of T2 task
		cout << "In myNetplanScheduler(), I am rank " << myRank << ", I have sent out all tasks. The taskTotalNum is " << taskTotalNum << " , taskCounterT1 is " << taskCounterT1 << " , taskCounterT2 is " << taskCounterT2 << " . I am  waiting for results from other nodes. \n\n" <<  endl;
		#endif
	}
	
	return 0;			   
  }
   // end of if (myRank == 0)
}
// EOF


