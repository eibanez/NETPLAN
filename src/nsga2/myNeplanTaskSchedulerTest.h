/* ****************************************************
Parallelization for NETPLAN  NSGA-IIb
Copyright (C) Jinxu Ding 2010-2011
The Netplan scheduler test function (run by master) that 
collects results from workers or wait for results until 
they are available
*******************************************************/
#ifndef _myNeplanTaskSchedulerTest_H_ 
#define _myNeplanTaskSchedulerTest_H_

#include <iostream>
#include <vector>
#include <sys/utsname.h>
#include "globalName.h"

//void childPopCombination(population *child_pop, int indBegin, int indEnd);
#ifdef USE_BOOST_MPI
bool myNeplanTaskSchedulerTest(char genCandTag, population * myChildpop, mpi::communicator world)
#else
//bool myNeplanTaskSchedulerTest(char genCandTag, population * myChildpop, int mySize, int myRank, int myT1Flag, int myT2Flag, vector< vector<double> >& resultTaskPackageT1, vector< vector<double> >& resultTaskPackageT2Pr, vector<double>& xdataRV, vector< vector<double> >& xdata2DRV, int objSize, vector< vector<double> >& resultTaskPackageT12)
bool myNeplanTaskSchedulerTest(char genCandTag, population * myChildpop, int mySize, int myRank, int myT1Flag, int myT2Flag, int objSize, vector< vector<double> >& resultTaskPackageT12, int genNum)
#endif
{
	double startTime, endTime; 	
	#ifdef USE_BOOST_MPI
	mpi::timer       seqTimer;
	mpi::timer       paraTimerT1;
	mpi::timer       paraTimerT2;
	#else
	
   	startTime = MPI_Wtime(); 
	#endif

	t2FinishStatusCounter = taskTotalNum;
	taskCounter  = 1;
	#ifdef DEBUG_myNeplanTaskSchedulerTest
	cout << "In myNeplanTaskSchedulerTest(), I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << " objSize is " << objSize << " genNum = " << genNum << " . \n\n" << endl;

	#endif 
	// Master needs to get T1 task results from workers
	if (myT1Flag == 1 && myT2Flag == 0) // T1
	{
		if (genCandTag == 'A')
		{
			#ifdef USE_BOOST_MPI
			mpi::wait_all(sendReqsT1AVec.begin(), sendReqsT1AVec.end());

			mpi::wait_all(recvReqsT1AVec.begin(), recvReqsT1AVec.end());

			#else

			int vecSizeT1A =  sendReqsT1AVec.size();
			int vecSizeT1AX =  sendReqsT1AXVec.size();
			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest(), I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << " , sendReqsT1AVec.size() is " << sendReqsT1AVec.size() << " . \n\n" << endl;  

			#endif

			int i;
			MPI_Request* sendReqsT1APtr;
			MPI_Request* sendReqsT1AXPtr;
			MPI_Status*  sendReqsT1AStatusPtr;
			MPI_Status*  sendReqsT1AXStatusPtr;
			sendReqsT1APtr 		= new  MPI_Request[vecSizeT1A];
			sendReqsT1AXPtr 	= new  MPI_Request[vecSizeT1AX];
			sendReqsT1AStatusPtr 	= new MPI_Status[vecSizeT1A];
			sendReqsT1AXStatusPtr 	= new MPI_Status[vecSizeT1AX];

			vector<MPI_Request>::iterator itrT1;
			// wait for master sending head info to workers
			for (i = 0, itrT1 = sendReqsT1AVec.begin(); i < vecSizeT1A && itrT1 != sendReqsT1AVec.end(); i++, itrT1++)
			{
				sendReqsT1APtr[i] = *itrT1; 
			}	
			// wait for master sending xdata to workers
			
			for (i = 0, itrT1 = sendReqsT1AXVec.begin(); i < vecSizeT1AX && itrT1 != sendReqsT1AXVec.end(); i++, itrT1++)
			{
				sendReqsT1AXPtr[i] = *itrT1; 
			}
	
			// http://publib.boulder.ibm.com/infocenter/zos/v1r9/index.jsp?topic=/com.ibm.zos.r9.fomp200/ipezps00240.htm
			//boost::mpi::wait_all(sendReqsT1AVec.begin(), sendReqsT1AVec.end());
			//MPI::Request::Waitall(vecSize, sendReqsT1Ptr);
			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest(), I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << ", I am waiting for send head to workers req, before MPI_Waitall(vecSize, sendReqsT1APtr, sendReqsT1AStatusPtr). \n\n" << endl;
			#endif
			// wait for head info recv by workers
			int returnWaitAValue = MPI_Waitall(vecSizeT1A, sendReqsT1APtr,  sendReqsT1AStatusPtr);
			
			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest(), I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << ", I am waiting for finishing send xdata to workers req, before MPI_Waitall(vecSizeT1AX, sendReqsT1AXPtr,  sendReqsT1AXStatusPtr). \n\n" << endl;
			#endif
			// wait for xdata sent to workers
			int returnWaitAXValue = MPI_Waitall(vecSizeT1AX, sendReqsT1AXPtr,  sendReqsT1AXStatusPtr);

			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest(), I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << ", I am after waiting for send xdata to workers, after MPI_Waitall(vecSizeT1AX, sendReqsT1AXPtr,  sendReqsT1AXStatusPtr), the returnWaitValue is " << returnWaitAValue << " , the returnWaitAXValue is " << returnWaitAXValue << " . \n\n" << endl;
			#endif

			delete [] sendReqsT1APtr;
			delete [] sendReqsT1AXPtr;
			delete [] sendReqsT1AStatusPtr;
			delete [] sendReqsT1AXStatusPtr;

			// wait for recv results from workers
			// vecSize =  recvReqsT1AVec.size();
			MPI_Request* 	recvReqsT1APtr;
			MPI_Status*  	recvReqsT1AStatusPtr;
			int vecSizeT1AR 	= 	recvReqsT1AVec.size();
			recvReqsT1APtr 		= new  MPI_Request[vecSizeT1AR];
			recvReqsT1AStatusPtr 	= new MPI_Status[vecSizeT1AR];
			
			for (i = 0, itrT1 = recvReqsT1AVec.begin(); i < vecSizeT1AR && itrT1 != recvReqsT1AVec.end(); i++, itrT1++)
			{
				recvReqsT1APtr[i] = *itrT1; 
			}

			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest() wait for recv req, I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << ", I am before MPI_Waitall(vecSize, recvReqsT1APtr, recvReqsT1AStatusPtr). The recvReqsT1AVec.size() is  " << recvReqsT1AVec.size() << " , it should be the taskTotalNum as " << taskTotalNum << "  \n\n" << endl;
			#endif
			// wait for resuls recv by master
			int returnWaitT1ARecvValue = MPI_Waitall(vecSizeT1AR, recvReqsT1APtr, recvReqsT1AStatusPtr);
			if (returnWaitT1ARecvValue != 0 )
			{
				cerr << "In myNeplanTaskSchedulerTest(), I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , error in MPI_Waitall(vecSizeT1AR, recvReqsT1APtr, recvReqsT1AStatusPtr) \n\n" << endl; 
				exit(1);
			}	
			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest(), I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << ", I am after MPI_Waitall(vecSize, recvReqsT1APtr, recvReqsT1AStatusPtr), returnWaitT1ARecvValue  is  " << returnWaitT1ARecvValue << " . The recvReqsT1AVec.size() is  " << recvReqsT1AVec.size() << " , it should be objSize as " << objSize << " . \n\n" << endl;
			#endif
			recvReqsT1AVec.clear();	
			delete [] recvReqsT1APtr;
			delete [] recvReqsT1AStatusPtr;

			#endif
		}
		else if (genCandTag == 'B')
		{
			#ifdef USE_BOOST_MPI
			mpi::wait_all(sendReqsT1BVec.begin(), sendReqsT1BVec.end());
			
			#else
			//MPI::Request::Waitall(sendReqsT1BVec.size(),sendReqsT1BVec.begin(), sendReqsT1BVec.end());
			//boost::mpi::wait_all(sendReqsT1BVec.begin(), sendReqsT1BVec.end());
			int vecSizeT1B 	= sendReqsT1BVec.size();
			int vecSizeT1BX = sendReqsT1BXVec.size();
			int i;
			
			MPI_Request* sendReqsT1BPtr;
			MPI_Request* sendReqsT1BXPtr;
			MPI_Status*  sendReqsT1BStatusPtr;
			MPI_Status*  sendReqsT1BXStatusPtr;
			sendReqsT1BPtr 		= new  MPI_Request[vecSizeT1B];
			sendReqsT1BXPtr 	= new  MPI_Request[vecSizeT1BX];
			sendReqsT1BStatusPtr 	= new MPI_Status[vecSizeT1B];
			sendReqsT1BXStatusPtr 	= new MPI_Status[vecSizeT1BX];
			vector<MPI_Request>::iterator itrT1;
			// wait for head info sent to workers
			
			for (i = 0, itrT1 = sendReqsT1BVec.begin(); i < vecSizeT1B && itrT1 != sendReqsT1BVec.end(); i++, itrT1++)
			{
				sendReqsT1BPtr[i] = *itrT1; 
			}
			// wait for xdata sent to workers
 
			for (i = 0, itrT1 = sendReqsT1BXVec.begin(); i < vecSizeT1BX, itrT1 != sendReqsT1BXVec.end(); i++, itrT1++)
			{
				sendReqsT1BXPtr[i] = *itrT1; 
			}
		
			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest(), I am rank "<< myRank << " , t1Flag is "<< t1Flag << ", t2Flag is " << t2Flag << " , genCandTag is "<<  genCandTag << ", I am before MPI_Waitall(vecSize, sendReqsT1BPtr, sendReqsT1BStatusPtr). \n\n" << endl;
			#endif
			//wait for head info sent to workers 
			int waitBValue = MPI_Waitall(vecSizeT1B, sendReqsT1BPtr, sendReqsT1BStatusPtr);
			// wait for xdata sent to workers
			int waitBXValue = MPI_Waitall(vecSizeT1BX, sendReqsT1BPtr, sendReqsT1BXStatusPtr);

			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest(), I am rank "<< myRank << " , t1Flag is "<< t1Flag << ", t2Flag is " << t2Flag << " , genCandTag is "<<  genCandTag << ", I am after MPI_Waitall(vecSize, sendReqsT1BPtr, sendReqsT1BStatusPtr). waitBValue is " << waitBValue << " , waitBXValue is " << waitBXValue  << "  \n\n" << endl;
			#endif
			sendReqsT1BVec.clear();
			sendReqsT1BXVec.clear();
			delete [] sendReqsT1BPtr;
			delete [] sendReqsT1BXPtr;
			delete [] sendReqsT1BStatusPtr;
			delete [] sendReqsT1BXStatusPtr;

			// wait for recv results from workers
			// vecSize =  recvReqsT1AVec.size();
			MPI_Request* 		recvReqsT1BPtr;
			MPI_Status*  		recvReqsT1BStatusPtr;
			int vecSizeT1BR 	= recvReqsT1BVec.size();
			recvReqsT1BPtr 		= new  MPI_Request[vecSizeT1BR];
			recvReqsT1BStatusPtr 	= new MPI_Status[vecSizeT1BR];
			
			for (i = 0, itrT1 = recvReqsT1BVec.begin(); i < vecSizeT1BR && itrT1 != recvReqsT1BVec.end(); i++, itrT1++)
			{
				recvReqsT1BPtr[i] = *itrT1; 
			}

			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest() wait for recv req, I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << ", I am before MPI_Waitall(vecSize, recvReqsT1BPtr, recvReqsT1BStatusPtr). The recvReqsT1BVec.size() is  " << recvReqsT1BVec.size() << " , it should be objSize as " << objSize  << "  \n\n" << endl;
			#endif
			// wait for resuls recv by master
			int returnWaitT1BRecvValue = MPI_Waitall(vecSizeT1BR, recvReqsT1BPtr, recvReqsT1BStatusPtr);

			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest() wait for recv req, I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << ", I am after MPI_Waitall(vecSize, recvReqsT1BPtr, recvReqsT1BStatusPtr), returnWaitT1BRecvValue  is  " << returnWaitT1BRecvValue << " . The recvReqsT1BVec.size() is  " << recvReqsT1BVec.size() << " , it should be objSize as " << objSize << " . \n\n" << endl;
			#endif
			recvReqsT1BVec.clear();	
			delete [] recvReqsT1BPtr;
			delete [] recvReqsT1BStatusPtr;

			#endif	
		}	
	}
	else if (myT1Flag == 0 && myT2Flag == 1) // Master needs to get T2 task results from workers
	{
		if (genCandTag == 'A')
		{
			#ifdef USE_BOOST_MPI
			mpi::wait_all(sendReqsT2AVec.begin(), sendReqsT2AVec.end());
			
			#else
			//MPI::Request::Waitall(sendReqsT2AVec.size(), sendReqsT2AVec.begin(), sendReqsT2AVec.end());
			//boost::mpi::wait_all(sendReqsT2AVec.begin(), sendReqsT2AVec.end());
			int vecSizeT2A =  sendReqsT2AVec.size();
			int vecSizeT2AX =  sendReqsT2AXVec.size();
			int i;
			
			MPI_Request* sendReqsT2APtr;
			MPI_Request* sendReqsT2AXPtr;
			MPI_Status*  sendReqsT2AStatusPtr;
			MPI_Status*  sendReqsT2AXStatusPtr;
			sendReqsT2APtr = new  MPI_Request[vecSizeT2A];
			sendReqsT2AXPtr = new  MPI_Request[vecSizeT2AX];
			sendReqsT2AStatusPtr = new MPI_Status[vecSizeT2A];
			sendReqsT2AXStatusPtr = new MPI_Status[vecSizeT2AX];
			vector<MPI_Request>::iterator itrT1;
			
			for (i = 0, itrT1 = sendReqsT2AVec.begin(); i < vecSizeT2A && itrT1 != sendReqsT2AVec.end(); i++, itrT1++)
			{
				sendReqsT2APtr[i] = *itrT1; 
			}		

			for (i = 0, itrT1 = sendReqsT2AXVec.begin(); i < vecSizeT2AX && itrT1 != sendReqsT2AXVec.end(); i++, itrT1++)
			{
				sendReqsT2AXPtr[i] = *itrT1; 
			}
			
			int waitT2AValue = MPI_Waitall(vecSizeT2A, sendReqsT2APtr, sendReqsT2AStatusPtr);

			int waitT2AXValue = MPI_Waitall(vecSizeT2AX, sendReqsT2APtr, sendReqsT2AStatusPtr);
			
			sendReqsT2AVec.clear();
			sendReqsT2AXVec.clear();
			delete [] sendReqsT2APtr;
			delete [] sendReqsT2AXPtr;
			delete [] sendReqsT2AStatusPtr;
			delete [] sendReqsT2AXStatusPtr;

			// wait for recv T2A results from workers
			// vecSize =  recvReqsT1AVec.size();
			MPI_Request* 		recvReqsT2APtr;
			MPI_Status*  		recvReqsT2AStatusPtr;
			int vecSizeT2AR 	= recvReqsT2AVec.size();
			recvReqsT2APtr 		= new  MPI_Request[vecSizeT2AR];
			recvReqsT2AStatusPtr 	= new MPI_Status[vecSizeT2AR];
			MPI_Status		testStatusArray[vecSizeT2AR], testStatus;
			int 			T2ARflag=0, T2ARflagAll=0;
			for (i = 0, itrT1 = recvReqsT2AVec.begin(); i < vecSizeT2AR && itrT1 != recvReqsT2AVec.end(); i++, itrT1++)
			{
				recvReqsT2APtr[i] = *itrT1; 
			}

			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest() wait for recv req, I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << ", I am waiting for master get result from workers, before MPI_Waitall(vecSize, recvReqsT2APtr, recvReqsT2AStatusPtr). The recvReqsT2AVec.size() is  " << recvReqsT2AVec.size() << " , it should be taskTotalNum as " << taskTotalNum  << "  \n\n" << endl;
			#endif
			int testResult=0;	
			// test request for recv T2A 
            		/*
			do
			{
			for (int ti = 0 ; ti < vecSizeT2AR ; ti++)
			{
				MPI_Test(&(recvReqsT2APtr[ti]), &T2ARflag, &testStatus);
	
				if (T2ARflag ==1 )
					testResult = 1;
				else 
					testResult = 0;

				cout << "In myNeplanTaskSchedulerTest() test recv T2A req, I am rank "<< myRank << " , T2ARflag is " << T2ARflag << " , testResult is " << testResult << " , for test index ti is " << ti << " . \n\n " << endl;
					
        		}
			MPI_Testall(vecSizeT2AR, recvReqsT2APtr, &T2ARflagAll, testStatusArray);
			sleep(1);
			} while(!T2ARflagAll);
			*/
			// wait for resuls recv by master
			int returnWaitT2ARecvValue = MPI_Waitall(vecSizeT2AR, recvReqsT2APtr, recvReqsT2AStatusPtr);
			if ( returnWaitT2ARecvValue != 0)
			{
				cerr << "In myNeplanTaskSchedulerTest(), I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << ", I am after MPI_Waitall(vecSize, recvReqsT2APtr, recvReqsT2AStatusPtr), returnWaitT2ARecvValue  is  " << returnWaitT2ARecvValue << " . The recvReqsT2AVec.size() is  " << recvReqsT2AVec.size() << " , it should be taskTotalNum as " << taskTotalNum << " error in returnWaitT2ARecvValue. \n\n" << endl;
				exit(1);
			}	
			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest(), I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << ", I am after MPI_Waitall(vecSize, recvReqsT2APtr, recvReqsT2AStatusPtr), returnWaitT2ARecvValue  is  " << returnWaitT2ARecvValue << " . The recvReqsT2AVec.size() is  " << recvReqsT2AVec.size() << " , it should be taskTotalNum as " << taskTotalNum << " . \n\n" << endl;
			#endif
			recvReqsT2AVec.clear();
			delete [] recvReqsT2APtr;
			delete [] recvReqsT2AStatusPtr;

			#endif	
		}
		else if (genCandTag == 'B')
		{
			#ifdef USE_BOOST_MPI
			mpi::wait_all(sendReqsT2BVec.begin(), sendReqsT2BVec.end());
			
			#else
			//MPI::Request::Waitall(sendReqsT2BVec.size(), sendReqsT2BVec.begin(), sendReqsT2BVec.end());
			//boost::mpi::wait_all(sendReqsT2BVec.begin(), sendReqsT2BVec.end());
			int vecSizeT2B = sendReqsT2BVec.size();
			int vecSizeT2BX = sendReqsT2BXVec.size();  
			int i;
			
			MPI_Request* sendReqsT2BPtr;
			MPI_Request* sendReqsT2BXPtr;
			MPI_Status*  sendReqsT2BStatusPtr;
			MPI_Status*  sendReqsT2BXStatusPtr;
			sendReqsT2BPtr = new  MPI_Request[vecSizeT2B];
			sendReqsT2BXPtr = new  MPI_Request[vecSizeT2BX];
			sendReqsT2BStatusPtr = new MPI_Status[vecSizeT2B];
			sendReqsT2BXStatusPtr = new MPI_Status[vecSizeT2BX];
			vector<MPI_Request>::iterator itrT1;
  
			for (i = 0, itrT1 = sendReqsT2BVec.begin(); i < vecSizeT2B && itrT1 != sendReqsT2BVec.end(); i++, itrT1++)
			{
				sendReqsT2BPtr[i] = *itrT1; 
			}
		
			for (i = 0, itrT1 = sendReqsT2BXVec.begin(); i < vecSizeT2BX && itrT1 != sendReqsT2BXVec.end(); i++, itrT1++)
			{
				sendReqsT2BXPtr[i] = *itrT1; 
			}		
			int waitT2BValue =  MPI_Waitall(vecSizeT2B, sendReqsT2BPtr, sendReqsT2BStatusPtr);

			int waitT2BXValue =  MPI_Waitall(vecSizeT2BX, sendReqsT2BXPtr, sendReqsT2BXStatusPtr);

			sendReqsT2BVec.clear();
			sendReqsT2BXVec.clear();
			delete [] sendReqsT2BPtr;
			delete [] sendReqsT2BXPtr;
			delete [] sendReqsT2BStatusPtr;
			delete [] sendReqsT2BXStatusPtr;

			// wait for recv T2B results from workers
			// vecSize =  recvReqsT1AVec.size();
			MPI_Request* 		recvReqsT2BPtr;
			MPI_Status*  		recvReqsT2BStatusPtr;
			int vecSizeT2BR 	= recvReqsT2BVec.size();
			recvReqsT2BPtr 		= new  MPI_Request[vecSizeT2BR];
			recvReqsT2BStatusPtr 	= new MPI_Status[vecSizeT2BR];
			
			for (i = 0, itrT1 = recvReqsT2BVec.begin(); i < vecSizeT2BR && itrT1 != recvReqsT2BVec.end(); i++, itrT1++)
			{
				recvReqsT2BPtr[i] = *itrT1; 
			}

			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest() wait for recv req, I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << ", I am before MPI_Waitall(vecSize, recvReqsT2BPtr, recvReqsT2BStatusPtr). The recvReqsT2BVec.size() is  " << recvReqsT2BVec.size() << " , it should be taskTotalNum as " << taskTotalNum  << "  \n\n" << endl;
			#endif
			// wait for resuls recv by master
			int returnWaitT2BRecvValue = MPI_Waitall(vecSizeT2BR, recvReqsT2BPtr, recvReqsT2BStatusPtr);

			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest() wait for recv req, I am rank "<< myRank << " , myT1Flag is "<< myT1Flag << ", myT2Flag is " << myT2Flag << " , genCandTag is "<<  genCandTag << ", I am after MPI_Waitall(vecSize, recvReqsT2BPtr, recvReqsT2BStatusPtr), returnWaitT2BRecvValue  is  " << returnWaitT2BRecvValue << " . The recvReqsT2BVec.size() is  " << recvReqsT2BVec.size() << " , it should be taskTotalNum as " << taskTotalNum << " . \n\n" << endl;
			#endif
			recvReqsT2BVec.clear();	
			delete [] recvReqsT2BPtr;
			delete [] recvReqsT2BStatusPtr;

			#endif	
		}
	}
	// test deadslock
	#ifdef DEBUG_myNeplanTaskSchedulerTest	
	cout << "In myNeplanTaskSchedulerTest(), I am rank " << myRank << ", I have colleted results from all ranks for generation " << genNum << " this is the "<< myGenerationNum << "th called " << " with genCandTag as " << genCandTag << " with total tasks " << taskTotalNum << " myT1Flag = " << myT1Flag << " myT2Flag is " << myT2Flag << " .\n\n"  << endl;
	#endif	
	#ifdef PRINTFOUT
	fprintf(paraT1File, "In node %d , taskTotalNum = %d, availableRank = %d \n\n", myRank, taskTotalNum, availableRank ); 
	fprintf(paraT1File,"========================================================= \n\n") ;
	fprintf(paraT1File, " node  |  task # | result | taskPerRank[iRank] | \n") ;
		
	fprintf(paraT2File, "In node %d , taskTotalNum = %d, availableRank = %d \n\n", myRank, taskTotalNum, availableRank ); 
	fprintf(paraT2File,"========================================================= \n\n") ;
	fprintf(paraT2File, " node  |  task # | result |   \n") ;
	#endif
	tempResult1 =0 ;
	tempResult2 =0 ;
	int indBegin, indEnd;
	// master print out resuls collected from workers
	// master collects T1 results from workers
	int nobj = initPara.nobj, indNum, constr_violation;
	// --------- master integrate final results ----------- //
	vector<vector<double> >::iterator t1TaskResultItr_i; 
	vector<vector<double> >::iterator t2TaskResultItr_ii; 
	if(myT1Flag == 1 || myT2Flag == 1) // get T1 or T2 tasks results for parallel version 
	{
		#ifdef PRINTFOUT
		fprintf(paraT1File, " this is results collected from workers for T1. \n\n");
		#endif
		cout << "In myNeplanTaskSchedulerTest(), I am rank " << myRank << " , I have finished waitAll, and try to integreat T1 or T2 task results myT1Flag = " << myT1Flag  << " myT2Flag = " << myT2Flag <<  " genNum is " << genNum << " genCandTag is " << genCandTag << " resultTaskPackageT12.size() is " << resultTaskPackageT12.size() << " \n\n "<< endl;
		// master get T1 task result from the pointers recevied from workers 
		//for (t1TaskResultItr_i = (resultTaskPackageT1Pr.begin()); t1TaskResultItr_i != (resultTaskPackageT1Pr).end();  t1TaskResultItr_i++)
		// master get results from workers the elements 
		// generation,. genCan , indNum worked by a worker, constr_violation , obj ... 
		// 0		1	 2 				3		4	
		#ifdef DEBUG_myNeplanTaskSchedulerTest
		for (int ii =0 ; ii < resultTaskPackageT12.size(); ii++)  // ii is ind
		{
			for (int jj =0 ; jj < resultTaskPackageT12[ii].size(); jj++) // jj is to index received result
			{
			 	cout << "In myNeplanTaskSchedulerTest(), check recv result, I am rank " << myRank << " , the resultTaskPackageT12[" << ii << "][" << jj << "] is " << resultTaskPackageT12[ii][jj] << " genNum = " << genNum << " genCandTag is " << genCandTag << endl; 
			}
			cout << "\n " << endl ; 
		}
		#endif
		int posiObj= 4, generation; char genCand; 	

		//((*myChildpop).ind) = new individual[resultTaskPackageT12.size()];
		// master pick up results returned by workers 
		for (t1TaskResultItr_i = (resultTaskPackageT12.begin()); t1TaskResultItr_i != (resultTaskPackageT12).end();  t1TaskResultItr_i++)
		{
			//tempResult1 = tempResult1  + (**t1TaskResultItr_i).getResult() ;

			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest(), I am rank " << myRank << " genNum = " << genNum << " genCand is " << genCand << " , I have passed waitAll, and try to access T1flag = " << myT1Flag << " T2flag = " << myT2Flag << "  task results. resultTaskPackageT12.size() is " << resultTaskPackageT12.size() << " , (*t1TaskResultItr_i).size() is " << (*t1TaskResultItr_i).size() << " , sizeof(*t1TaskResultItr_i) is " <<  sizeof(*t1TaskResultItr_i) << " \n\n " << endl ;
			#endif
			//tempResult1 = tempResult1  + (**t1TaskResultItr_i).getResult() ;
			generation	   = (int)((*t1TaskResultItr_i)[0]);
			genCand		   = (char)((*t1TaskResultItr_i)[1]);
			indNum 		   = (int)((*t1TaskResultItr_i)[2]);
			constr_violation   = (int)((*t1TaskResultItr_i)[3]);
			// combine pop ind to form a child_pop
			//childPopCombination(myChildpop, (*t1TaskResultItr_i).obj, indBegin, indEnd);
			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest(), before checking obj one by one I am rank " << myRank << " genNum = " << genNum << " genCand is " << genCand << " , I have passed waitAll, and begins to intergreat T1flag = " << myT1Flag << " T2flag = " << myT2Flag << " task results. resultTaskPackageT12.size() is " << resultTaskPackageT12.size() << " the *t1TaskResultItr_i[0] gene is " << (*t1TaskResultItr_i)[0]<< " , the (*t1TaskResultItr_i)[1] genCandTag is " << (*t1TaskResultItr_i)[1] << " , the (*t1TaskResultItr_i)[2] indNum is " << (*t1TaskResultItr_i)[2] <<" ,  (*t1TaskResultItr_i)[3] const_vio is " << (*t1TaskResultItr_i)[3] << " \n\n "<< endl;
			#endif
			
			// (((*myChildpop).ind)[indNum]).obj = new double[nobj];
				// collect obj vector from workers
				for (int j = 0; j< nobj ; j++)
				{
					(((*myChildpop).ind)[indNum]).obj[j] = (*t1TaskResultItr_i)[j+posiObj];
		
					#ifdef DEBUG_myNeplanTaskSchedulerTest
					cout << "In myNeplanTaskSchedulerTest(), check obj one by one I am rank " << myRank << " genNum is " << genNum << " genCandTag is " << genCandTag << " , I have passed waitAll, and am doing integreat myT1Flag is " << myT1Flag << " myT2Flag is " << myT2Flag << " task results of indNum is " << indNum  << " , the sizeof((((*myChildpop).ind)[" << indNum <<"]).obj[" << j << "] is " << sizeof((((*myChildpop).ind)[indNum]).obj[j]) << " , (((*myChildpop).ind)[" << indNum << "]).obj[" << j << "] is "<< (((*myChildpop).ind)[indNum]).obj[j] << " , sizeof(*t1TaskResultItr_i)[" << j+posiObj << "] is "<<  sizeof((*t1TaskResultItr_i)[j+posiObj]) << " , (*t1TaskResultItr_i).obj[" << j+posiObj <<"] is " << (*t1TaskResultItr_i)[j+posiObj] << "\n\n" << endl;
					#endif
				}
			
				(((*myChildpop).ind)[indNum]).constr_violation = constr_violation;
			//}
			
			#ifdef PRINTFOUT
			fprintf(paraT1File, "%6d    %6d    \n ", (*t1TaskResultItr_i).taskNum, tempResult1 ) ;
			#endif
			//std::cout << "\n\n " << "At generation " <<  myGenerationNum << ", T1 task, " <<  ", I am rank " << world.rank() << ", I have got results from node " << (**t1TaskResultItr_i).getSourceRank() << ", its result value is " << (**t1TaskResultItr_i).getResult() << ". This is task "<< (**t1TaskResultItr_i).getTaskNum() << " out of total task "  << (**t1TaskResultItr_i).getTotalTaskNum() <<", the current accumulated result tempResult2 is " <<  tempResult2 << ", its T1 task tag is " << (**t1TaskResultItr_i).getTaskTag() << " .\n\n"  << std::endl;		
		}
	} 
	// end of if(myT1Flag == 1 || myT2Flag == 1)	
	#ifdef DEBUG_myNeplanTaskSchedulerTest		
	cout << "In myNeplanTaskSchedulerTest(), I am rank " << myRank << ", for generation " << genNum << " this is the " << myGenerationNum << "th called " << " and genCandTag is " << genCandTag << " , I have got all results from workers. \n\n " << endl;
	#endif
	taskCounter =0 ; t1Flag == 0; t2Flag == 0;
	++myGenerationNum;
	
	// seq time
	endTime = MPI_Wtime();
	#ifdef PRINTFOUT
	fprintf(pFile, "I am rank is %d, the seq final result is %d. I use time %d seconds \n\n ", myRank, tempResult, endTime - startTime);
	#endif

	// -------------------------- print out timing results ----------------//
	if (availableRank < 1) // sequential
	{
		endTime = MPI_Wtime();
		#ifdef DEBUG_myNeplanTaskSchedulerTest
		cout << "\n\n " << "In myNeplanTaskSchedulerTest(), I am rank " << myRank << ", this is sequential algo, with total task num " << taskTotalNum << ", availableRank = " << availableRank << ", I use total sequential time " <<  endTime - startTime << ", the SLEEPTIME is "<< SLEEPTIME << ", myT1Flag is " << myT1Flag << " , myT2Flag is " << myT2Flag << " . \n\n" << endl;
		#endif
	}
	else // parallel
	{
		if (taskTotalNum <= availableRank+1)
		{
			endTime = MPI_Wtime();
			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "\n\n " << "In myNeplanTaskSchedulerTest(), I am rank " << myRank << " genNum is " << genNum << " genCandTag is " << genCandTag << ", T1 taskTotalNum = " << taskTotalNum << ", availableRank = " << availableRank << ", I use total parallel time " <<  endTime - startTime << ", the SLEEPTIME is "<< SLEEPTIME << ", the tempResult1 is "<< tempResult1 << ", the leftTaskNum is " << leftTaskNum << ", the aveTaskNum is " << aveTaskNum << ". \n\n" << endl;
			#endif
		}
		
		if (taskTotalNum > availableRank+1)
		{
			endTime = MPI_Wtime();
			#ifdef DEBUG_myNeplanTaskSchedulerTest
			cout << "In myNeplanTaskSchedulerTest(), I am rank " << myRank << " genNum is " << genNum << "genCandTag is " << genCandTag << ", T2 taskTotalNum = " << taskTotalNum << ", availableRank = " << availableRank << ", I use total parallel time " <<  endTime - startTime << ", the SLEEPTIME is "<< SLEEPTIME  << ", the tempResult2 is "<< tempResult2 << ", the leftTaskNum is " << leftTaskNum << ", the aveTaskNum is " << aveTaskNum << endl;
			#endif
		}
	//resultTaskPackageT12.clear(); //
	//resultTaskPackageT12.erase(resultTaskPackageT12.begin(), resultTaskPackageT12.end()); //	
	}
	#ifdef PRINTFOUT
	fprintf(paraT1File, "In node %d , T1 taskTotalNum = %d, availableRank = %d, master use time %d , the SLEEPTIME is %d . \n\n", myRank, taskTotalNum, availableRank, endTime - startTime, SLEEPTIME );
	
	fprintf(paraT2File, "In node %d , T2 taskTotalNum = %d, availableRank = %d, master use time %d, the SLEEPTIME is %d . \n\n", myRank, taskTotalNum, availableRank, endTime - startTime, SLEEPTIME );

        fclose(pFile);	fclose(paraT2File); fclose(paraT1File);
	#endif 
	#ifdef DEBUG_myNeplanTaskSchedulerTest
	cout << "In myNeplanTaskSchedulerTest(), I am rank " << myRank << " genNum is " << genNum << " genCandTag is " << genCandTag << " , myT1Flag is " << myT1Flag<< " , myT2Flag is " << myT2Flag <<  " , after resultTaskPackageT12.clear(), I return " << ISTRUE << " . \n\n " << endl ;
	#endif
	return ISTRUE;
} 

#endif
//EOF



