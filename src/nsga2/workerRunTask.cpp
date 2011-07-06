/***************************************************************************
 *   Copyright (C) 2010 by Jinxu Ding                                      *
 *   jxding@ece.iastate.edu                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/* ****************************************************
Parallelization for NETPLAN  NSGA-IIb
Copyright (C) Jinxu Ding 2010-2011
workers wait for tasks from matser
*******************************************************/
#include <iostream>
#include <vector>
#include <sys/utsname.h>
#include "globalName.h"
#include "defines.h"
#include <ilcplex/ilocplex.h>
// worker nodes recv task from manager node and then run it , after that return the result to manager			
//int workerRunTask(MPI_Datatype message_to_master_type, int nodeSize, int myRank, int xVSize, int objSize, MPI_Datatype xdata_to_workers_type, vector <double>& recvXDataVec, newCNSGA2& myNsga2, MPI_Datatype Mpara_to_workers_type, double* events, cplexType* netplan)
int workerRunTask(MPI_Datatype message_to_master_type, int nodeSize, int myRank, int xVSize, int objSize, MPI_Datatype xdata_to_workers_type, vector <double>& recvXDataVec, newCNSGA2& myNsga2, MPI_Datatype Mpara_to_workers_type, double events[]){
	
	// xVSize is size of xdata _ inbdNum, objSize is the size of return result
        int rank, mytaskTag=0, sourceRank, managerRank =0, destRank = 0, resultTask, i, myworkerUpStreamTaskTag=0;
	
	MPI_Request sendReqsW[1];
	MPI_Request workerTimer; 
		
	// a vector worker node receives task from managwer node
	MPI_Request resultToMasterReq;
	vector<MPI_Request> resultToMasterReqVec; 
 
	size_t xDVSize = xVSize;
	
	resultToMasterReqVec.clear();
	int downStreamTaskTagOffset = 0 ; char genCandTagLast; int workerCounter =0; int runTaskCounter = 0 ; int packNum=0;
	int generation=0; int taskNum =0 ; int indBegin =0 ; int indEnd=0; char genCandTag; int indNum =0 ;
	//message_para_to_workersT message_para_to_one_workerW;
	//vector<double> message_para_to_one_workerVec(objSize, 0.0);
	//message_para_to_workersTNew message_para_to_workersTNewSt;
	/*
	char genCandTag;
	double 	constr_violation;
    	double 	crowd_dist;
	int 	rank;
	int indNum; // the indNum in the next xdata package assigned to a worker
  	int generation;
  	int taskNum;
	int taskTag; // worker use it to return result to master
  	int indBegin;
  	int indEnd;
	int packNum; // packN
        message_para_to_workersTNewSt.indEnd = 0;
	message_para_to_workersTNewSt.indBegin = 0;
	message_para_to_workersTNewSt.packNum = 0;
	message_para_to_workersTNewSt.indNum = 0;
	message_para_to_workersTNewSt.taskNum = 0;
	message_para_to_workersTNewSt.rank = 0;
	message_para_to_workersTNewSt.constr_violation = 0.0;
	message_para_to_workersTNewSt.crowd_dist = 0.0;
	message_para_to_workersTNewSt.genCandTag = 'C';
	*/
	
	//MPI::Status status;
	MPI_Status status, status1;
	population populp ; //= new population;
	//MPI::Status status1;
	// http://www.ieor.berkeley.edu/~atamturk/ieor264/samples/concert/lagrangian.cpp
	//IloEnv env;
	//IloCplex myCplex(env);
	//char cplexLogFile[50] ;
	//sprintf(cplexLogFile, "myCplexlog.log.%d", myRank);
	//ofstream fout(cplexLogFile);
	//myCplex.setOut(fout);
	//myCplex.setOut(env.getNullStream());
	do
	{
		// world.recv(managerRank, downStreamTaskTag, resultTaskPackageW);
		// after receving broadcast basic pop para, workers receive pop from master
		double* sendResultArrayPr = new double[objSize];
		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank " << myRank << " mem allocated for sendResultArrayPr size objSize is " << objSize << " \n\n" ;
		#endif
 
		if (sendResultArrayPr == 0)
		{
			cerr << "In workerRunTask(), no mem allocated for sendResultArrayPr \n\n" ;
			exit(1);
		}
		double endTime, startTime; 
		
		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank "<< myRank << ", I am before Recv(message_para_to_one_worker), the downStreamTaskTag+ downStreamTaskTagOffset is "<< downStreamTaskTag+downStreamTaskTagOffset << " , downStreamTaskTag is " << downStreamTaskTag << " , downStreamTaskTagOffset is " << downStreamTaskTagOffset << " , packNum is " << packNum << " runTaskCounter is " << runTaskCounter << " . \n\n" << endl;
		#endif

		int retRecv = MPI_Recv(&message_para_to_workersTNewSt, 1, Mpara_to_workers_type, 0, downStreamTaskTag + downStreamTaskTagOffset, MPI_COMM_WORLD, &status);

		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank "<< myRank << ", I am after Recv(message_para_to_one_worker), the downStreamTaskTag+ downStreamTaskTagOffset is "<< downStreamTaskTag+downStreamTaskTagOffset << " , sizeof(message_para_to_workersTNewSt) is " << sizeof(message_para_to_workersTNewSt) << " . \n\n" << endl;
		#endif

		if (retRecv != 0)
		{
		 	cerr << "In workerRunTask(), I am rank " << myRank << " , an error in  MPI::COMM_WORLD.Recv(&message_para_to_workersTNewSt " << "\n\n" << endl ;
			exit(1);
		}
		#ifdef DEBUG_workerRunTask
		//cout << "In workerRunTask(), I am rank " <<  myRank << "  , seconda check sendResultVec.size() is " << sendResultVec.size() << " , sendResultVec.capacity() is " << sendResultVec.capacity() << " , sizeof(message_para_to_one_workerW) is " << sizeof(message_para_to_one_workerW)  << " \n\n" << endl; // << " , sendResultVec1.size() is " << sendResultVec1.size() << " , sendResultVec1.capacity() is" << sendResultVec1.capacity() << " . \n\n" << endl; 
		#endif

		// worker use taskTag to recev xdata package and return result to master	
		//mytaskTag 	= message_para_to_one_workerW.taskTag; 
		//int generation 	= message_para_to_one_workerW.generation;
		//int taskNum 	= message_para_to_one_workerW.taskNum;
		//int indBegin	= message_para_to_one_workerW.indBegin;
		//int indEnd	= message_para_to_one_workerW.indEnd;
		
		// the data elements in Mpara_to_workers_type
		//char genCandTag; double constr_violation; double crowd_dist; int rank; int indNum;int generation;int taskNum; int taskTag; // 
  		//	0			1		2		3	    4		5	        6	    7		
		//int indBegin; 	int indEnd;	int packNum; // packNum is the ind num per node 
		//	8		9		10
		//mytaskTag 	= (int)message_para_to_one_workerVec[7]; 
		mytaskTag 	= (message_para_to_workersTNewSt.taskTag);
		
		generation 	= (message_para_to_workersTNewSt.generation);
		
		taskNum 	= (message_para_to_workersTNewSt.taskNum);
		
		indBegin	= (message_para_to_workersTNewSt.indBegin);
		
 		indEnd		= (message_para_to_workersTNewSt.indEnd);
		
		genCandTag	= (message_para_to_workersTNewSt.genCandTag);

		indNum		= (message_para_to_workersTNewSt.indNum);
		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank " <<  myRank << " , after recv head info my taskTag is " << mytaskTag << " , generation is " << generation << " , taskNum is " << taskNum <<  " , indBegin is " <<  indBegin <<  " , indEnd is " <<  indEnd << " , genCandTag is " << genCandTag  << " , indNum is " << indNum << " . \n\n" ; 
		#endif
		// determine loop times
		taskNumPerNode = message_para_to_workersTNewSt.packNum;
		packNum        = message_para_to_workersTNewSt.packNum;

		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank " <<  myRank << " , after after recv head info my taskTag is " << mytaskTag << " , generation is " << generation << " , taskNum is " << taskNum <<  " , indBegin is " <<  indBegin <<  " , indEnd is " <<  indEnd << " , genCandTag is " << genCandTag << " , packNum is " << packNum << " , taskNumPerNode is " << taskNumPerNode << " , indNum is " << indNum << " . \n\n" ;
		#endif
		//}
		#ifdef DEBUG_workerRunTask
		//cout << "In workerRunTask(), I am rank " <<  myRank << "  , secondc check sendResultVec.size() is " << sendResultVec.size() << " , sendResultVec.capacity() is " << sendResultVec.capacity()  << " . \n\n" << endl;
		#endif
		if ((downStreamTaskTagOffset +1 == packNum) || (downStreamTaskTagOffset +1 > packNum))
		{
			downStreamTaskTagOffset  = 0;
		} 
		else
		{
			++downStreamTaskTagOffset;
		}	

		//int indNum	= message_para_to_one_workerW.indNum;// the indNum in the next xdata package assigned to a worker
		//int indNum	= (int)message_para_to_one_workerVec[4];
		int indNum	= message_para_to_workersTNewSt.indNum;
		#ifdef DEBUG_workerRunTask
		//cout << "In workerRunTask(), I am rank " <<  myRank << "  , third check sendResultVec.size() is " << sendResultVec.size() << " , sendResultVec.capacity() is " << sendResultVec.capacity()  << " . \n\n" << endl;
		#endif
		#ifdef DEBUG_workerRunTask
		//cout << "In workerRunTask(), I am rank "<< myRank << ", after recving head info fro master, I am before recv recvXDataVec from master by mytaskTag as " << mytaskTag << " , message_para_to_one_worker.generation is " << message_para_to_one_workerW.generation << " , the message_para_to_one_worker.taskNum is " << message_para_to_one_workerW.taskNum << " , the message_para_to_one_worker.indBegin is " << message_para_to_one_workerW.indBegin << " , the message_para_to_one_worker.indEnd is "<< message_para_to_one_worker.indEnd << " , the message_para_to_one_worker.taskTag is " << message_para_to_one_workerW.taskTag  << " , the message_para_to_one_worker.genCandTag is " << message_para_to_one_workerW.genCandTag << " , the sizeof(recvXDataVec) is " << sizeof(recvXDataVec) << " , recvXDataVec.size() is " << recvXDataVec.size() << " , and  xVSize is " << xVSize << " . \n\n" << endl;
		#endif

		recvXDataVec.resize(xVSize);

		// after recving basic headinfo, worker recv xdata to work on it.
		
		int retValue2 = MPI_Recv(&(recvXDataVec[0]), 1, xdata_to_workers_type, 0, mytaskTag,MPI_COMM_WORLD,  &status1);
		if (retValue2 != 0)
		{
		 	cerr << "In workerRunTask(), I am rank " << myRank << " , an error in  MPI::COMM_WORLD.Recv(&(recvXDataVec[0]) " << "\n\n" << endl ;
			exit(1);
		}
		// http://www.cplusplus.com/reference/stl/vector/vector/
		 
		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank " <<  myRank << " \n\n" << endl ; // "  , fourth check sendResultVec.size() is " << sendResultVec.size()  << " . \n\n" << endl;
		#endif
		if (indNum != (int)recvXDataVec[0])
		{
			cerr << "I am rank " << myRank << " , generation is " << generation << " , genCandTag is " << genCandTag << " , taskNum is " << taskNum  << " , indNum != (int)recvXDataVec[0], the next recved xdata is not for the previous head info, (int)recvXDataVec[0] is " << (int)recvXDataVec[0] <<  " , and indNum is " << indNum << " , the taskTag of recv xdata is mytaskTag " << mytaskTag <<  " . \n\n " << endl;
		
			exit (1);
		}
		startTime = MPI_Wtime();
	
		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank "<< myRank << ", I am after Recv(recvXDataVec), and (indNum != (int)recvXDataVec[0]), the mytasktag is "<< mytaskTag << " , the indNum is " << indNum << " , the (int)recvXDataVec[0] is " << (int)recvXDataVec[0] << " , the sizeof(recvXDataVec) is " << sizeof(recvXDataVec) << " , the recvXDataVec.size() is "  << recvXDataVec.size() << ", it should be xDVSize as " << xDVSize << " . \n\n" << endl;
		#endif
	
		//sleep(SLEEPTIME);
		#ifdef DEBUG_workerRunTask
		//cout << "In workerRunTask(), I am rank " << myRank << ", I recived task from node " << managerRank << ", the recved task generation is "<< message_para_to_one_workerW.generation << ", its gen tag is " << message_para_to_one_workerW.genCandTag << ". This is the task " << message_para_to_one_workerW.taskNum << " with gen tag is " << message_para_to_one_workerW.genCandTag << ", the taskNumPerNode is " << taskNumPerNode << " , message_para_to_one_worker.packNum is " << message_para_to_one_workerW.packNum << " , runTaskCounter is " << runTaskCounter << " , the sizeof(message_para_to_one_worker.xreal) is " << sizeof(message_para_to_one_workerW.xreal) << " , and the (message_para_to_one_worker.xreal).size() is " << (message_para_to_one_workerW.xreal).size() <<  " \n\n" << endl; // " , check sendResultVec.size() is " << sendResultVec.size()  << " . \n\n" << endl;
		#endif

		// master get results from workers
		// the elements : generation,. genCan , indNum worked by a worker
		//			1	2	3	
		//		  constr_violation , obj
		//			4	     5	
		// 
		sendResultArrayPr[0] = generation;
		
		sendResultArrayPr[1] = genCandTag;
		
		sendResultArrayPr[2] = indNum;	
		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I an rank " << myRank << " after recv xdata, I run my task. \n\n" << endl;
		#endif
		
		//----------------- perform task operations according to the taskpackage
		//nsga2->evaluatePop(nsga2->child_pop, resultTaskPackageSend.getIndBegin(), resultTaskPackageSend.getIndEnd());
		//population populp ; //= new population;
		populp.ind = new individual;

		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank "<< myRank << ", I am after  populp.ind = new individual .\n\n" << endl;  
		#endif

		(populp.ind)->rank = (int)message_para_to_workersTNewSt.rank;

		(populp.ind)->constr_violation = message_para_to_workersTNewSt.constr_violation;
			
		(populp.ind)->crowd_dist = (int)message_para_to_workersTNewSt.crowd_dist;
		#ifdef DEBUG_workerRunTask
		//cout << "In workerRunTask(), I am rank "<< myRank << ", I am after assigning crowd_dist, it is " << (populp.ind)->crowd_dist << " , message_para_to_one_worker.crowd_dist is " << message_para_to_one_workerW.crowd_dist << " , message_para_to_one_worker.indBegin is " << message_para_to_one_workerW.indBegin << " , message_para_to_one_worker.indBegin is " << message_para_to_one_workerW.indBegin << " .\n\n" << endl;  
		#endif

		int nreal = initPara.nreal; int nobj  = initPara.nobj;	
		int ncon  = initPara.ncon; int i=0; int nbin = initPara.nbin; 
		// xdata sent from master to worker 
		// indNum nreal nbin nobj ncon
		//int BinPosition = nreal + nobj + ncon;
		int BinPosition = nreal+1 ;
		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank "<< myRank << ", I am before assigning xreal and obj to populp.ind , neal is " << nreal << ", nobj is " << nobj << " nbin is "<< nbin << " ncon is " << ncon << " .\n\n" << endl;  
		#endif
		// recvXDataVec has indNum(1) + xreal (nreal) + obj (nobj) + const(ncon) , which is xVSize
		(populp.ind)->xreal 	= new double[nreal];
		(populp.ind)->obj 	= new double[nobj];
		(populp.ind)->constr 	= new double[ncon];
		(populp.ind)->xbin 	= new double[nbin];
		if ((populp.ind)->xreal == NULL || (populp.ind)->obj == NULL || (populp.ind)->constr == NULL || (populp.ind)->xbin == NULL )
		{
			#ifdef DEBUG_workerRunTask
			cout << "In workerRunTask(), I am rank "<< myRank << " (populp.ind)->xreal or (populp.ind)->obj or (populp.ind)->constr or (populp.ind)->xbin is NULL .\n\n" << endl;  
			#endif
			exit(1);
		}	
		for (i = 0 ; i < nreal ; i++)
		{
			#ifdef DEBUG_workerRunTask
			cout << "In workerRunTask(), I am rank "<< myRank << ", I am before assigning xreal to populp.ind the indNum is "<< indNum << " , recvXDataVec[" << i+1 << "] is " << recvXDataVec[i+1] << " .\n\n" << endl;  
			#endif

			//(populp.ind)->xreal[i] = message_para_to_one_worker.xreal[i];
			
			((populp.ind)->xreal)[i] = recvXDataVec[i+1]; 
			#ifdef DEBUG_workerRunTask
			cout << "In workerRunTask(), I am rank "<< myRank << " , gen is " << generation << " , genCandTag is " << genCandTag << " , indNum is " << indNum << " , I am after assigning xreal to populp.ind, recvXDataVec[" << i+1 << "] is " << recvXDataVec[i+1] << " , and (populp.ind)->xreal[" << i << "] is " <<  (populp.ind)->xreal[i] << " .\n\n" << endl;  
			#endif
	
		}
		
		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank "<< myRank << ", I am after assigning xreal neal is " << nreal << ", nobj is " << nobj << " .\n\n" << endl;  
		#endif

		for (i = 0 ; i < nbin ; i++)
		{
			((populp.ind)->xbin)[i] = recvXDataVec[i+BinPosition];  // get xbin from master 
		}
		int objPosi = 1+nreal;
		//for (i =0 ; i < nobj ; i++)
		//{
			//(populp.ind)->obj[i] = message_para_to_one_worker.obj[i];
		//	(populp.ind)->obj[i] = recvXDataVec[i+objPosi]; 
		//}
		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank "<< myRank << ", I am after assigning obj to populp.ind , nobj is " << nobj << " ncon is " << ncon << " .\n\n" << endl;  
		#endif	
		// xdata sent from master to worker 
		// indNum nreal nbin nobj ncon
		//int conPosi = objPosi + nobj ;
		int conPosi = 1 + nreal + nbin + nobj ;
		for (i =0 ; i < ncon ; i++)
		{
			//(populp.ind)->constr[i] = message_para_to_one_worker.constr[i];
			(populp.ind)->constr[i] = recvXDataVec[i+conPosi]; 
		}
	
		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank "<< myRank << ", I am before myNsga2->evaluatePop(&populp, workerPopSize)) indNum is " << indNum << " \n\n" << endl;  
		#endif
		int 	objRSize = 0;
		
		// Solve problem
		//ImportIndices(); CPLEX netplan; netplan.LoadProblem();		 
		//myNsga2.evaluatePop(&populp, workerPopSize, ncon ,1); // int paraFLag = 1 evaluate one ind per time
		//myNsga2.evaluatePopInd(&populp, events, 0, objRSize, myRank, netplan); // int paraFLag = 1 evaluate one ind per time
		try{
		myNsga2.evaluatePopInd(&populp, events, 0, objRSize, myRank); // the real application commented out for debug
		}
		catch (IloException& e) {
			cerr << "In worker rank " << myRank << " gen is " << generation << " , genCandTag is " << genCandTag << " , indNum is " << indNum << " Concert exception caught: " << e << endl;
		} catch (...) {
			cerr << "In worker rank " << myRank << " gen is " << generation << " , genCandTag is " << genCandTag << " , indNum is " << indNum  << "Unknown exception caught" << endl;
		}

		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), I am rank "<< myRank << ", I am after  myNsga2->evaluatePopInd) objRSize =" << objRSize << " indNum =" << indNum <<  " nobj is " << nobj << "\n\n" << endl;  
		#endif

		// use local results to load result to vec sent to master
		
		//sendResultVec.push_back(populp.ind->constr_violation);
		sendResultArrayPr[3] = populp.ind->constr_violation;
		// sendResultArrayPr[4] = objRSize; // tell master how many obj results returned from workers
		int validObjPosition = 0;		
		for (i =0 ; i < nobj ; i++)
		//for (i =0 ; i < objRSize ; i++)	
		{
			//(message_to_master.obj)[i] = (populp.ind->obj)[i];
			//sendResultVec.push_back((populp.ind->obj)[i]);
			sendResultArrayPr[i+4] = (populp.ind->obj)[i];
			#ifdef DEBUG_workerRunTask
			cout << "In workerRunTask(), I am rank "<< myRank << ", generation is " << generation << " , genCandTag is " << genCandTag << " , indNum is " << indNum << ", after myNsga2->evaluatePopInd, sendResultArrayPr[" << i+4 << "] is " << sendResultArrayPr[i+4] << " , and (populp.ind->obj)["<< i << "] is " << (populp.ind->obj)[i] << " \n\n" << endl;  
			#endif
			//validObjPosition = i+4;
		}	
		/*
		if (objRSize < nobj) // some elements in returned obj[] value are not valid
		{
			for (i =0 ; i < nobj - objRSize ; i++)	
			{	
				sendResultArrayPr[validObjPosition] = 1.0E+10;
				#ifdef DEBUG_workerRunTask
				cout << "In workerRunTask(), I am rank "<< myRank << ", in objRSize < nobj, generation is " << generation << " , genCandTag is " << genCandTag << " , indNum is " << indNum << ", after myNsga2->evaluatePopInd, sendResultArrayPr[" << validObjPosition << "] is " << sendResultArrayPr[validObjPosition] << " \n\n" << endl;  
				#endif
			}
		}	
		*/
		myworkerUpStreamTaskTag = mytaskTag;
		// return result to manager node
		//world.send(destRank, workerUpStreamTaskTag, returnTaskPackageW);
			
		// master get results from workers
		// the elements : generation,. genCan , indNum worked by a worker
		//			1	2	3	
		//		  constr_violation , obj
		//			4	     5	

		#ifdef DEBUG_workerRunTask
		//cout << "In workerRunTask(), I am rank "<< myRank << ", I am before send sendResultVec to master by mytaskTag " << mytaskTag << " , sendResultVec[0] gene is "<<  sendResultVec[0] << " , sendResultVec[1] gencand is " << (char)sendResultVec[1] << " , sendResultVec[2] indNum is "<<  sendResultVec[2] << " , sendResultVec[3] constr_violation is " << sendResultVec[3]  << ", the myworkerUpStreamTaskTag is " << myworkerUpStreamTaskTag << " \n\n" << endl;  // << " , the sizeof(sendResultVec) is " << sizeof(sendResultVec) << " , sendResultVec.size() is " << sendResultVec.size()  << " . \n\n" << endl;
		#endif
		// worker returns results to master
		
		MPI_Isend(&sendResultArrayPr[0], objSize, MPI_DOUBLE, 0, myworkerUpStreamTaskTag, MPI_COMM_WORLD, &resultToMasterReq);
		
		resultToMasterReqVec.push_back(resultToMasterReq);
		endTime = MPI_Wtime(); 

		++runTaskCounter ;
		// the elements : generation,. genCan , indNum worked by a worker
		//			1	2	3	
		//		  constr_violation , obj
		//			4	     5	
		#ifdef DEBUG_workerRunTask
		cout << "In workerRunTask(), my rank is " << myRank << ", after sending a result to master for generation " << sendResultArrayPr[0]  << " . This is the taskNum " << taskNum << ", the mytasktag is " << mytaskTag << ", I use time  seconds " << endTime - startTime  << ", the myworkerUpStreamTaskTag is  " << myworkerUpStreamTaskTag  << " , runTaskCounter is " << runTaskCounter << " , its gen cand tag is " << (char)sendResultArrayPr[1]  << " , gen is " << sendResultArrayPr[0] << " , indNum is " << sendResultArrayPr[2] << " , myworkerUpStreamTaskTag is " << myworkerUpStreamTaskTag << ", check resultToMasterReqVec.size() is " << resultToMasterReqVec.size() << " , obj[0] is " << sendResultArrayPr[4] << " , obj[1] is " << sendResultArrayPr[5] << " , constr_violation is " << sendResultArrayPr[3] << " , runTaskCounter is " << runTaskCounter << " (2*taskNumPerNode * (initPara.ngen)) is " << (2*taskNumPerNode * (initPara.ngen)) << " taskNumPerNode is " << taskNumPerNode << "  initPara.ngen is " << initPara.ngen << " \n\n" << endl;  // " , sendResultVec.size() is " << sendResultVec.size() << " , then sendResultVec will be cleared. \n\n" << endl;
		#endif
		//std::cout << "---------------------------------------------------------"  << endl;
		recvXDataVec.clear();
		//sendResultVec.clear();
		//delete [] recvDataArray;
		
		delete [] (populp.ind)->xreal ;
		delete [] (populp.ind)->xbin ;
		delete [] (populp.ind)->obj ;
		delete [] (populp.ind)->constr ;
		delete [] sendResultArrayPr;
		delete populp.ind;

		//delete myNsga2;//delete myNsga2;
	}while(runTaskCounter < (2*taskNumPerNode * initPara.ngen)); // each gen has 2 cand types  
	//}while(runTaskCounter < (2*taskNumPerNode * (initPara.ngen-1)));
	// end fo do-while
	// else if (message_para_to_one_worker.endRun == ISTRUE) // if end taks is true, donot wait for new tasks

	//fout.close();

	#ifdef DEBUG_workerRunTask
	cout << "In workerRunTask(), I am rank " << myRank << ", I recived task from node 0. I am told that the total ngen is " << initPara.ngen << " , taskNumPerNode is " << taskNumPerNode << " , and runTaskCounter is " << runTaskCounter-1 << " So, they are equal. I do not wait for new tasks. \n\n" << endl;
	#endif
	// int returnWaitAValue = MPI_Waitall(vecSizeT1A, sendReqsT1APtr,  sendReqsT1AStatusPtr);
	
	MPI_Request* resultToMasterReqArray = new MPI_Request[resultToMasterReqVec.size()];
	MPI_Status*  resultToMasterReqStatusArray = new MPI_Status[resultToMasterReqVec.size()];
	vector<MPI_Request>::iterator itrr;
	int jj;
	for(jj= 0 , itrr = resultToMasterReqVec.begin(); itrr != resultToMasterReqVec.end(); itrr++, jj++) 
	{
		resultToMasterReqArray[jj] = *itrr;
	}	
        #ifdef DEBUG_workerRunTask
	cout << "IN workerRunTask(), I am rank " << myRank << " , after finishing all tasks, I am before wait for all send result to master req finished . \n\n " << endl; 
	#endif

	int returnWaitSendValue =  MPI_Waitall(resultToMasterReqVec.size(), resultToMasterReqArray, resultToMasterReqStatusArray);
	if (returnWaitSendValue != 0 )
	{
		cerr << "IN workerRunTask(), I am rank " << myRank << " , returnWaitSendValue is " << returnWaitSendValue << " , error in MPI_Waitall(resultToMasterReqVec.size() \n\n " << endl; 
		exit(1) ;
	}
	#ifdef DEBUG_workerRunTask
	cout << "IN workerRunTask(), I am rank " << myRank << " , returnWaitSendValue is " << returnWaitSendValue << " , after finishing all tasks, I am after wait for all send result to master req finished, now I return 0. resultToMasterReqVec.size() is " << resultToMasterReqVec.size() << " \n\n " << endl; 
	#endif
	if (resultToMasterReqVec.size() != 0){
		delete [] resultToMasterReqArray;
		delete [] resultToMasterReqStatusArray;
	}
	else{
		delete resultToMasterReqArray;
		delete resultToMasterReqStatusArray;
	}	
	
	resultToMasterReqArray = NULL;
	resultToMasterReqStatusArray = NULL;

	runTaskCounter = 0; taskNumPerNode =0 ;
			
	return 0;
}

//int testWorker(MPI_Datatype message_to_master_type, int mySize, int myRank, int xRVSize, int objSize, MPI_Datatype xdata_to_workers_type, vector <double>& recvXDataVec, newCNSGA2& myNsga2, MPI_Datatype Mpara_to_workers_type, double eventsInput[]){
//	cout << "I am in int testWorker() \n\n " << endl ;
//}

// EOF
