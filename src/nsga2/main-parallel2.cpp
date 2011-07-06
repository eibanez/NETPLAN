// --------------------------------------------------------------
//    NETSCORE Version 2
//    Implementation of parallel NSGA-II
//    2009-2011 (c) Eduardo Ibanez and others
//    For more info:
//        http://natek85.blogspot.com/2009/07/c-nsga2-code.html
//        http://www.iitk.ac.in/kangal/codes.shtml
// --------------------------------------------------------------

using namespace std;
#include "CNSGA2.h"
#include <fstream>
#include <string>
#include <vector>
#include "../netscore.h"
#include "globalName.h"
#include "globalNameDefine.h"
#include "myNeplanTaskSchedulerTest.h"
//#include "workerRunTask.cpp"

//CNSGA2* nsga2a = new CNSGA2(true, 1.0);
//CNSGA2* nsga2b = new CNSGA2(false, 0.33);

int main (int argc, char **argv) {
	printHeader("nsga-parallel");
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &mySize);

	double endTimeM, startTimeM; 

	startTimeM = MPI_Wtime();
	
	cout << "In main-parallel2.cpp, mySize is " << mySize << " , myRank is " << myRank << endl; 

	int blockcounts_to_workers[8], blockcounts_to_workers1[1], blockcounts_to_master[1], bcount_xdata[1], blockcounts_Mpara_workers[3];

	MPI_Datatype oldtypes_workers[8], oldtypes_workers1[1], oldtypes_master[1], oldtypes_xdata[1], oldtypes_Mpara[3];
	//
	MPI_Datatype  message_para_to_workers_type1, message_to_master_type;
	MPI_Datatype xdata_to_workers_type, Mpara_to_workers_type;
	/* MPI_Aint type used to be consistent with syntax of */
	/* MPI_Type_extent routine */

	MPI_Aint   offsets_workers[8], offsets_workers1[1], offsets_master[1] , extent_workers, extent_master, offsets_xdata[1], extent_xworkers , offsets_Mpara[3],  extent_para ;

	initPara.popsize = 0;
	initPara.ngen = 0 ;
	initPara.nobj = 0 ;
	initPara.ncon = 0 ;
	initPara.nreal = 0 ;
	initPara.nbin = 0;
	if (myRank == 0) {
		// Read indices
		ImportIndices();
		// -- Initialization of A -- //
		nsga2a->randgen->randomize();					// Initialize random number generator
		nsga2a->Init("prepdata/param.in");				// This sets all variables related to GA
		nsga2a->InitMemory();							// This allocates memory for the populations
		nsga2a->InitPop(nsga2a->parent_pop, Np_start);	// Initialize parent population randomly
		nsga2a->fileio->recordConfiguration();			// Records all variables related to GA configuration
		// master broadcast these basic para to all workers
		initPara.popsize = nsga2a->getPopsize();
		initPara.ngen	 = nsga2a->getNgen();
		initPara.nobj	 = nsga2a->getNobj();
		initPara.ncon	 = nsga2a->getNcon();
		initPara.nreal	 = nsga2a->getNreal();
		initPara.nbin	 = nsga2a->getNbin();
	}
	#ifdef DEBUG_main
	cout << "In main-parallel2.cpp, initPara.nobj is " << initPara.nobj << " , myRank is " << myRank << endl; 
	#endif
	// derived data structure to transfer basic para to workers
	/* data for popsize  */	
	/* Setup description of the 1 MPI_INT fields type */
	/* Need to first figure offset by getting size of MPI_INT */
	offsets_workers1[0] = 0;
	oldtypes_workers1[0] = MPI_INT;
	blockcounts_to_workers1[0] = 6;	

	/* Now define structured type and commit it */
	MPI_Type_struct(1, blockcounts_to_workers1, offsets_workers1, oldtypes_workers1, &message_para_to_workers_type1);
	MPI_Type_commit(&message_para_to_workers_type1);

	// a MPI data structure to transfer pop parameter to workers
	// define for message_para_to_workersTNew message_para_to_workersSt;
	offsets_Mpara[0] = 0;
	oldtypes_Mpara[0] = MPI_CHAR;
	blockcounts_Mpara_workers[0] = 1;

	MPI_Type_extent(MPI_CHAR, &extent_para);
	offsets_Mpara[1] = 1 * extent_para + offsets_Mpara[0];
	oldtypes_Mpara[1] = MPI_DOUBLE;
	blockcounts_Mpara_workers[1] = 2;

	MPI_Type_extent(MPI_DOUBLE, &extent_para);
	offsets_Mpara[2] = 2 * extent_para + offsets_Mpara[1];
	oldtypes_Mpara[2] = MPI_INT;
	blockcounts_Mpara_workers[2] = 8;

	MPI_Type_struct(3, blockcounts_Mpara_workers, offsets_Mpara, oldtypes_Mpara, &Mpara_to_workers_type);
	MPI_Type_commit(&Mpara_to_workers_type);

	if (mySize > 1)	
		MPI::COMM_WORLD.Bcast(&initPara, 1, message_para_to_workers_type1,0);

	const int popsize = initPara.popsize;
	const int ngen = initPara.ngen;
	const int nobj = initPara.nobj;
	const int ncon = initPara.ncon;
	const int nreal = initPara.nreal;
	const int nbin = initPara.nbin;

	// master and workers build up derived data sgtructure to transfer population information to workers 
	
	// data structure for xdata to support data transfer from master to worker
	// xreal , obj, con
	size_t xdataVSize = nreal+ncon+nobj+1+nbin;
	int xVSize = nreal+ncon+nobj+1+nbin ;
	vector<double> xdataV(xVSize, 0.0) ; // to get T1 task results

	// master get results from workers
	// the elements : generation,. genCan , indNum worked by a worker
	//  constr_violation , obj
	size_t objSize = nobj+4;  // nobj is variable and depends on the worker computing results
	vector<double> xdataRV ;
	int xRVSize = nobj+4;

	vector <double> recvXDataVec(xVSize, 0.0);;

	vector< vector <double> > resultTaskPackageT1(popsize, vector<double>(objSize, 0.0));
	
	vector< vector <double> > resultTaskPackageT2Pr(popsize, vector<double>(objSize, 0.0));

	vector <vector <double> > resultTaskPackageT12A(popsize, vector<double>(objSize, 0.0));

	vector <vector <double> > resultTaskPackageT12B(popsize, vector<double>(objSize, 0.0));	

	#ifdef DEBUG_main
	cout << "In main(), myRank is " << myRank << " , after MPI::COMM_WORLD.Bcast, initPara.popsize is " << initPara.popsize << " , initPara.nreal is  " << initPara.nreal << " , initPara.nobj is " <<  initPara.nobj << " , initPara.ncon is " << initPara.ncon << " , xVSize is "<< xVSize << " , xRVSize is " << xRVSize << " , objSize is " << objSize << " , xdataVSize is "<< xdataVSize <<  " , xdataV.size() is "<< xdataV.size()  << " , xdataRV.size() is " << xdataRV.size() <<  " , popsize is " << popsize << " , objSize is " << objSize << " , resultTaskPackageT12A[0].size() is " << resultTaskPackageT12A[0].size() << " , resultTaskPackageT12B[0].size() is " << resultTaskPackageT12B[0].size() << " \n\n " << endl;
	#endif

	offsets_xdata[0] = 0;
	oldtypes_xdata[0] = MPI_DOUBLE;
	bcount_xdata[0] = nreal+ncon+nobj+nbin;
	
	#ifdef DEBUG_main
	cout << "In main-parallel2 myRank is " << myRank << ", nreal+ncon+nobj+nbin is " << nreal+ncon+nobj+nbin << " \n\n" << endl;
	#endif

	/* Now define structured type and commit it */
	
	MPI_Type_struct(1, bcount_xdata, offsets_xdata, oldtypes_xdata, &xdata_to_workers_type);
	MPI_Type_commit(&xdata_to_workers_type);
	
	#ifdef DEBUG_main
	cout << "In main-parallel2 myRank is " << myRank << ", xdata_to_workers_type definition is done\n\n" << endl;
	#endif
	
	#ifdef DEBUG_main
	cout << "In main-parallel2 myRank is " << myRank << ", message_para_to_workers_type definition is done\n\n" << endl;
	#endif

	//ImportIndices(); 
	//CPLEX netplan ; 
	//netplan.LoadProblem();
	
	//double events[(SLength[0] + IdxCap.GetSize()) * (Nevents+1)];
	//ReadEvents(events, "prepdata/bend_events.csv");

	int myT1Flag=0, myT2Flag=0;

	//int myNeplanTaskScheduler(CNSGA2* nsga2, int popSize, int mySize, int myRank, population * myChildpop, char genTag, int generationNum, message_para_to_workers_VecT& myPopVec, MPI_Datatype message_to_master_type, int& myT1Flag, int& myT2Flag, vector< vector <double> >& resultTaskPackageT1, vector< vector <double> >&  resultTaskPackageT2Pr, vector<double>& xdataV, int objSize, vector <vector <double> >& resultTaskPackageT12AB, MPI_Datatype xdata_to_workers_type, int myGenerationNum, MPI_Datatype Mpara_to_workers_type , int nconNum, CPLEX netplan);	

	int myNeplanTaskScheduler(CNSGA2* nsga2, int popSize, int mySize, int myRank, population * myChildpop, char genTag, int generationNum, message_para_to_workers_VecT& myPopVec, MPI_Datatype message_to_master_type, int& myT1Flag, int& myT2Flag, vector< vector <double> >& resultTaskPackageT1, vector< vector <double> >&  resultTaskPackageT2Pr, vector<double>& xdataV, int objSize, vector <vector <double> >& resultTaskPackageT12AB, MPI_Datatype xdata_to_workers_type, int myGenerationNum, MPI_Datatype Mpara_to_workers_type , int nconNum);	

	//double* eventsInput;
	//int testWorker(MPI_Datatype message_to_master_type, int mySize, int myRank, int xRVSize, int objSize, MPI_Datatype xdata_to_workers_type, vector <double>& recvXDataVec, newCNSGA2& myNsga2, MPI_Datatype Mpara_to_workers_type, double eventsInput[]);
	int workerRunTask(MPI_Datatype message_to_master_type, int mySize, int myRank, int xRVSize, int objSize, MPI_Datatype xdata_to_workers_type, vector <double>& recvXDataVec, newCNSGA2& myNsga2, MPI_Datatype Mpara_to_workers_type, double eventsInput[]);
	//int workerRunTask(MPI_Datatype message_to_master_type, int mySize, int myRank, int xRVSize, int objSize, MPI_Datatype xdata_to_workers_type, vector <double>& recvXDataVec, newCNSGA2& myNsga2, MPI_Datatype Mpara_to_workers_type, double* eventsInput, cplexType* myNetplan);
	//int workerRunTask(MPI_Datatype message_to_master_type, int mySize, int myRank, int xRVSize, int objSize, MPI_Datatype xdata_to_workers_type, vector <double>& recvXDataVec, CNSGA2& myNsga2, MPI_Datatype Mpara_to_workers_type, double* eventsInput, CPLEX netplan);

	int masterRunTask(population* myChildpop, vector<double>& resultTaskPackage, int iRank, int objsize, int myGenerationNum, char genCandTag);
	//int masterRunTask(population* myChildpop, vector<double>& resultTaskPackage, int iRank, int objsize, int myGenerationNum, char genCandTag, cplexType* myNetplan );
	//int masterRunTask(population* myChildpop, vector<double>& resultTaskPackage, int iRank, int objsize, int myGenerationNum, char genCandTag, CPLEX netplan);

	void initPopPara(population* myPop, message_para_to_workers_VecT& myPara, initParaType& myIpara, int mySize, int myRank, vector<double>& xdataV);
	
	// -- Send and receive results from workers -- // 
	if (myRank == 0) {
		// request handel
		std::vector<MPI_Request> paraToWorkerReqa(mySize);
		std::vector<MPI_Request> paraToWorkerReqb(mySize);
		
		int dest; 
 		int myGenerationNum=1;
		message_para_to_workers_VecT  message_para_to_workers_Vec(popsize);
		
		for (int tt =0 ; tt < popsize; tt++) {
			(message_para_to_workers_Vec[tt].xreal).resize(initPara.nreal);
			(message_para_to_workers_Vec[tt].obj).resize(initPara.nobj);
			(message_para_to_workers_Vec[tt].constr).resize(initPara.ncon);
			(message_para_to_workers_Vec[tt].xbin).resize(initPara.nbin);
		}
		#ifdef DEBUG_main
		cout << "In main-parallel2.cpp myRank is " << myRank << ", Initialization is done, now performing first generation \n\n" << endl;
		#endif

		nsga2a->decodePop(nsga2a->parent_pop);
		#ifdef DEBUG_main
		cout << "In main-parallel2.cpp myRank is " << myRank << " , nsga2a->decodePop(nsga2a->parent_pop) is done. \n\n" << endl;
		#endif	

		initPopPara(nsga2a->parent_pop, message_para_to_workers_Vec, initPara, mySize, myRank, xdataV);
		int genNum = 1;
		//nsga2a->sendPop(nsga2a->parent_pop);		
		#ifdef DEBUG_main
		cout << "In main-parallel2.cpp myRank is " << myRank << ", this is gen " << genNum << "A , after initPopPara(nsga2a->parent_pop, message_para_to_workers_Vec, initPara, mySize, myRank) is done, I am before myNeplanTaskScheduler(nsga2a). The sizeof(message_para_to_workers_Vec[0].xreal) is " << sizeof(message_para_to_workers_Vec[0].xreal) << ", its size is " << (message_para_to_workers_Vec[0].xreal).size() << " , message_para_to_workers_Vec[0].xreal[0] is " << message_para_to_workers_Vec[0].xreal[0] << " . \n\n" << endl;
		#endif

		// This is to divide workload for each worker and then
		// initialize message with para and send tasks to workers
		// scheduler do not wait for results available at this point
		// Vector of capacity losses for events
		// compute 1A  -----------------
		myNeplanTaskScheduler(nsga2a, nsga2a->popsize, mySize, myRank, nsga2a->parent_pop, 'A', genNum, message_para_to_workers_Vec, message_to_master_type, myT1Flag, myT2Flag, resultTaskPackageT1, resultTaskPackageT2Pr, xdataV, objSize, resultTaskPackageT12A, xdata_to_workers_type, myGenerationNum, Mpara_to_workers_type, ncon);		
		//myNeplanTaskScheduler(nsga2a, nsga2a->popsize, mySize, myRank, nsga2a->parent_pop, 'A', genNum, message_para_to_workers_Vec, message_to_master_type, myT1Flag, myT2Flag, resultTaskPackageT1, resultTaskPackageT2Pr, xdataV, objSize, resultTaskPackageT12A, xdata_to_workers_type, myGenerationNum, Mpara_to_workers_type, ncon, netplan);
		#ifdef DEBUG_main
		cout << "In main-parallel2.cpp myRank is " << myRank << ", this is gen " << genNum << "B , myNeplanTaskScheduler(nsga2a) gen A has been run. \n\n " << endl;
		#endif
	
		// JINXU: This function puts the individuals in the queue
		// -- Initialization of B -- //
		nsga2b->randgen->randomize();					// Initialize random number generator
		nsga2b->Init("prepdata/param.in");				// This sets all variables related to GA
		nsga2b->InitMemory();							// This allocates memory for the populations
		nsga2b->InitPop(nsga2b->parent_pop, Np_start);	// Initialize parent population randomly
		
		// -- Send 1B -- // 
		nsga2b->decodePop(nsga2b->parent_pop);
		//nsga2b->sendPop(nsga2b->parent_pop);
		// JINXU: This function puts the individuals in the queue\

		initPopPara(nsga2b->parent_pop, message_para_to_workers_Vec, initPara, mySize, myRank, xdataV);
		// compute 1B---------------------
		myNeplanTaskScheduler(nsga2b, nsga2b->popsize, mySize, myRank, nsga2b->parent_pop, 'B', genNum, message_para_to_workers_Vec, message_to_master_type, myT1Flag, myT2Flag, resultTaskPackageT1, resultTaskPackageT2Pr, xdataV, objSize, resultTaskPackageT12B, xdata_to_workers_type, myGenerationNum, Mpara_to_workers_type, ncon);	
		//myNeplanTaskScheduler(nsga2b, nsga2b->popsize, mySize, myRank, nsga2b->parent_pop, 'B', genNum, message_para_to_workers_Vec, message_to_master_type, myT1Flag, myT2Flag, resultTaskPackageT1, resultTaskPackageT2Pr, xdataV, objSize, resultTaskPackageT12B, xdata_to_workers_type, myGenerationNum, Mpara_to_workers_type, ncon, netplan);
		// -- Receive 1A --------------------------- //
		// nsga2a->receivePop(nsga2a->parent_pop);	
		// JINXU: This function waits for the solution queue
		if (mySize > 1) {
			testAResult = myNeplanTaskSchedulerTest('A', nsga2a->parent_pop, mySize, myRank, myT1Flag, myT2Flag, xRVSize, resultTaskPackageT12A, 1);
		
			#ifdef DEBUG_main
			cout << "In main.cpp, myRank is " << myRank << ", this is gen " << genNum << " , after running myNeplanTaskSchedulerTest('A'), testAResult is " << testAResult << " , I am before while(1). \n\n " << endl;
			#endif
		
			if (testAResult != ISTRUE) {
				while(1) {
					if (myNeplanTaskSchedulerTest('A', nsga2a->parent_pop, mySize, myRank, myT1Flag, myT2Flag, xRVSize, resultTaskPackageT12A, 1) == ISTRUE)
						break; 
				}
			}
		}
		nsga2a->assignRankCrowdingDistance(nsga2a->parent_pop); 
		
		// -- Report 1A -- //
		nsga2a->fileio->report_pop (nsga2a->parent_pop, nsga2a->fileio->fpt1);
		fprintf(nsga2a->fileio->fpt4,"# gen = 1A\n");
		nsga2a->fileio->report_pop(nsga2a->parent_pop,nsga2a->fileio->fpt4); // print all_pop
		// print out 1A results
		//nsga2a->fileio->report_pop1A(nsga2a->parent_pop,nsga2a->fileio->fpt7); // print 1A
		
		nsga2a->fileio->flushIO();
		
		
	
		// -- Receive 1B ------------------------------ //
		// nsga2b->receivePop(nsga2b->parent_pop);		
		// JINXU: This function waits for the solution queue
		if (mySize > 1) {
			testBResult = myNeplanTaskSchedulerTest('B', nsga2b->parent_pop, mySize, myRank, myT1Flag, myT2Flag, xRVSize,  resultTaskPackageT12B, 1);
			#ifdef DEBUG_main
			cout << "In main.cpp, myRank is " << myRank << ", this is gen " << genNum << " , after running myNeplanTaskSchedulerTest('B'), testBResult is " << testBResult << " , I am before while(1). \n\n " << endl;
			#endif

			if (testBResult != ISTRUE) {
				while(1) {
					if (myNeplanTaskSchedulerTest('B', nsga2b->parent_pop, mySize, myRank, myT1Flag, myT2Flag, xRVSize, resultTaskPackageT12B, 1) == ISTRUE)
						break; 
				}
			}
		}
		nsga2b->assignRankCrowdingDistance(nsga2b->parent_pop); 
	
		// -- Report 1B -- //
		nsga2a->fileio->report_pop(nsga2b->parent_pop,nsga2a->fileio->fpt1);
		fprintf(nsga2a->fileio->fpt4,"# gen = 1B\n");
		nsga2a->fileio->report_pop(nsga2b->parent_pop,nsga2a->fileio->fpt4);
		nsga2a->fileio->flushIO();

		cout <<  "In main-parallel2, nsga2a->ngen is " << nsga2a->ngen <<  " current gen is " << 1 << " \n\n " << endl ;

		for ( int i = 2; i <= nsga2a->ngen; i++ ) {
			#ifdef DEBUG_main
			cout <<  "In main-parallel2, nsga2a->ngen is " << nsga2a->ngen <<  " current gen is " << i << " \n\n " << endl ;
			#endif
			myGenerationNum = i ;
			//#ifdef RUN_2A_IN_1GEN
			if (i == 2) { 
				// -- Generate and send 2A -------------------------------- //
				nsga2a->selection(nsga2a->parent_pop, nsga2a->child_pop);
				nsga2a->mutatePop(nsga2a->child_pop);
				nsga2a->decodePop(nsga2a->child_pop);
			
				//nsga2a->sendPop(nsga2a->child_pop);
				// JINXU: This function puts the individuals in the queue
				initPopPara(nsga2a->child_pop, message_para_to_workers_Vec, initPara, mySize, myRank, xdataV);
				// schedule 2A -----------------------
				
				myNeplanTaskScheduler(nsga2a, nsga2a->popsize, mySize, myRank, nsga2a->child_pop, 'A', i, message_para_to_workers_Vec, message_to_master_type, myT1Flag, myT2Flag, resultTaskPackageT1, resultTaskPackageT2Pr, xdataV, objSize, resultTaskPackageT12A, xdata_to_workers_type, i, Mpara_to_workers_type, ncon);	
				//myNeplanTaskScheduler(nsga2a, nsga2a->popsize, mySize, myRank, nsga2a->child_pop, 'A', i, message_para_to_workers_Vec, message_to_master_type, myT1Flag, myT2Flag, resultTaskPackageT1, resultTaskPackageT2Pr, xdataV, objSize, resultTaskPackageT12A, xdata_to_workers_type, i, Mpara_to_workers_type, ncon, netplan);
			}
			//#endif

			// -- Generate and send (i)B ----------------------------- //
			nsga2b->selection(nsga2b->parent_pop, nsga2b->child_pop);
			nsga2b->mutatePop(nsga2b->child_pop);
			nsga2b->decodePop(nsga2b->child_pop);
			//nsga2b->sendPop(nsga2b->child_pop);			
			// JINXU: This function puts the individuals in the queue
			initPopPara(nsga2b->child_pop, message_para_to_workers_Vec, initPara, mySize, myRank, xdataV);		
			// schedule B
			
			myNeplanTaskScheduler(nsga2b, nsga2b->popsize, mySize, myRank, nsga2b->child_pop, 'B', i, message_para_to_workers_Vec, message_to_master_type, myT1Flag, myT2Flag, resultTaskPackageT1, resultTaskPackageT2Pr, xdataV, objSize, resultTaskPackageT12B, xdata_to_workers_type, myGenerationNum, Mpara_to_workers_type, ncon);
			//myNeplanTaskScheduler(nsga2b, nsga2b->popsize, mySize, myRank, nsga2b->child_pop, 'B', i, message_para_to_workers_Vec, message_to_master_type, myT1Flag, myT2Flag, resultTaskPackageT1, resultTaskPackageT2Pr, xdataV, objSize, resultTaskPackageT12B, xdata_to_workers_type, myGenerationNum, Mpara_to_workers_type, ncon, netplan);
			// -- Receive (i)A -------------------------------- // djx: recv last A
			//nsga2a->receivePop(nsga2a->child_pop);		
			// JINXU: This function waits for the solution queue
			if (mySize > 1) {
				testAResult = myNeplanTaskSchedulerTest('A', nsga2a->child_pop, mySize, myRank, myT1Flag, myT2Flag, xRVSize, resultTaskPackageT12A, i);

				#ifdef DEBUG_main
				cout << "In main.cpp, i = " << i << " i <= nsga2a->ngen mySize is " << mySize << " myRank is " << myRank << ", this is gen " << i << "A, after running myNeplanTaskSchedulerTest('A'), testAResult is " << testAResult << " , I am before while(1). \n\n " << endl;
				#endif
				if (testAResult != ISTRUE) {
					while(1) {
						if (myNeplanTaskSchedulerTest('A', nsga2a->child_pop, mySize, myRank, myT1Flag, myT2Flag, xRVSize, resultTaskPackageT12A, i) == ISTRUE)
							break; 
					}
				}
			}
			nsga2a->merge(nsga2b->parent_pop, nsga2a->child_pop, nsga2a->mixed_pop);
			nsga2a->fillNondominatedSort(nsga2a->mixed_pop, nsga2a->parent_pop);
			
			// -- Report (i)A -- //
			fprintf(nsga2a->fileio->fpt4,"# gen = %dA\n",i);
			nsga2a->fileio->report_pop(nsga2a->parent_pop,nsga2a->fileio->fpt4);
			nsga2a->fileio->flushIO();
			
			if (i < nsga2a->ngen) {
				// -- Generate and send (i+1)A -- //
				nsga2a->selection(nsga2a->parent_pop, nsga2a->child_pop);
				nsga2a->mutatePop(nsga2a->child_pop);
				nsga2a->decodePop(nsga2a->child_pop);
				// nsga2a->sendPop(nsga2a->child_pop);
				initPopPara(nsga2a->child_pop, message_para_to_workers_Vec, initPara, mySize, myRank, xdataV);	
				// schedule A
				
				myNeplanTaskScheduler(nsga2a, nsga2a->popsize, mySize, myRank, nsga2a->child_pop, 'A', i, message_para_to_workers_Vec, message_to_master_type, myT1Flag, myT2Flag, resultTaskPackageT1, resultTaskPackageT2Pr, xdataV, objSize, resultTaskPackageT12A, xdata_to_workers_type, i, Mpara_to_workers_type, ncon);
				//myNeplanTaskScheduler(nsga2a, nsga2a->popsize, mySize, myRank, nsga2a->child_pop, 'A', i, message_para_to_workers_Vec, message_to_master_type, myT1Flag, myT2Flag, resultTaskPackageT1, resultTaskPackageT2Pr, xdataV, objSize, resultTaskPackageT12A, xdata_to_workers_type, i, Mpara_to_workers_type, ncon, netplan);
			}
					
			// -- Receive (i)B -- //
			//nsga2b->receivePop(nsga2b->child_pop);	
			// JINXU: This function waits for the solution queue
			if (mySize > 1) {	
				testAResult = myNeplanTaskSchedulerTest('B', nsga2b->child_pop, mySize, myRank, myT1Flag, myT2Flag, xRVSize, resultTaskPackageT12B, i);
			
				#ifdef DEBUG_main
				cout << "In main.cpp, myRank is " << myRank << ", this is gen " << i << "B , after running myNeplanTaskSchedulerTest('B'), testBResult is " << testAResult << " , I am before nsga2b->merge(nsga2a->parent_pop, nsga2b->child_pop, nsga2b->mixed_pop). \n\n " << endl;
				#endif
				if (testBResult != ISTRUE) {
					while(1) {
						if (myNeplanTaskSchedulerTest('B', nsga2b->child_pop, mySize, myRank, myT1Flag, myT2Flag, xRVSize, resultTaskPackageT12B, i) == ISTRUE)
							break; 
					}
				}	
			}
			nsga2b->merge(nsga2a->parent_pop, nsga2b->child_pop, nsga2b->mixed_pop);
			nsga2b->fillNondominatedSort(nsga2b->mixed_pop, nsga2b->parent_pop);
			
			// -- Report (i)B ------------------------------------------- //
			fprintf(nsga2a->fileio->fpt4,"# gen = %dB\n",i);
			nsga2a->fileio->report_pop(nsga2b->parent_pop,nsga2a->fileio->fpt4);
			nsga2a->fileio->flushIO();
		}
		
		// -- Report final solution -- //
		nsga2a->fileio->report_pop(nsga2b->parent_pop,nsga2a->fileio->fpt2);
		nsga2a->fileio->report_feasible(nsga2b->parent_pop,nsga2a->fileio->fpt3);
		if (nsga2a->nreal!=0) {
			fprintf(nsga2a->fileio->fpt5,"\n Number of crossover of real variable = %d",nsga2a->nrealcross);
			fprintf(nsga2a->fileio->fpt5,"\n Number of mutation of real variable = %d",nsga2a->nrealmut);
		}
		if (nsga2a->nbin!=0) {
			fprintf(nsga2a->fileio->fpt5,"\n Number of crossover of binary variable = %d",nsga2a->nbincross);
			fprintf(nsga2a->fileio->fpt5,"\n Number of mutation of binary variable = %d",nsga2a->nbinmut);
		}
    } //--------------- end of (myRank == 0) ----------------------------------------------
	else if (myRank != 0) {
		printHeader("postprocessor");
		double endTimeW, startTimeW;
		cout << "In main(), I am rank " << myRank << " , I am before workerRunTask, begin timing . \n\n"  << endl ;
		//sleep(1);	
		newCNSGA2 myNsga2(1);
		//CNSGA2 myNsga2;	
		// Read global parameters
		//sleep(myRank);
		//ReadParameters("data/parameters.csv");
		// Import indices to export data
		ImportIndices();
		
		// Declare variables to store the optimization model
		//CPLEX netplan;
		
		// Read master and subproblems
		//sleep(myRank);
		//netplan.LoadProblem();

		// Vector of capacity losses for events
		double events[(SLength[0] + IdxCap.GetSize()) * (Nevents+1)];
		//sleep(myRank);
		ReadEvents(events, "prepdata/bend_events.csv");

		startTimeW = MPI_Wtime();
		//testWorker(message_to_master_type, mySize, myRank, xVSize, objSize,  xdata_to_workers_type, recvXDataVec, myNsga2, Mpara_to_workers_type, events);

		workerRunTask(message_to_master_type, mySize, myRank, xVSize, objSize,  xdata_to_workers_type, recvXDataVec, myNsga2, Mpara_to_workers_type, events);
		//workerRunTask(message_to_master_type, mySize, myRank, xVSize, objSize,  xdata_to_workers_type, recvXDataVec, myNsga2, Mpara_to_workers_type, events, netplan);

		endTimeW = MPI_Wtime();

		cout << "In main(), I am rank " << myRank << " , I am after workerRunTask I use time " << endTimeW - startTimeW << " seconds,  SLEEPTIME is " << SLEEPTIME << " \n\n"  << endl ;	  	
	}	// end of else if (myRank != 0) 
     
	cout << "In main(), I am rank " << myRank << " , I am before MPI_Barrier(MPI_COMM_WORLD). \n\n"  << endl ;

	MPI_Barrier(MPI_COMM_WORLD);

	cout << "In main(), I am rank " << myRank << " , I am before MPI_Finalize() and after MPI_Barrier(MPI_COMM_WORLD). \n\n"  << endl ;

	endTimeM = MPI_Wtime();
	MPI_Finalize();

	if (myRank == 0) {
 		cout << "In main(), I am rank " << myRank << " , the total time is " << endTimeM - startTimeM << " seconds, total ngen is " << ngen << " . popSize is " << popsize << " , total processors num is " << mySize << " , SLEEPTIME is " << SLEEPTIME << " .\n\n " << endl ;  
	}
	cout << "In main(), I am rank " << myRank << " , I am after MPI_Finalize(), then return 0 . \n\n"  << endl ;
	
	printHeader("completed");
	return (0);
} // end of main()

#include "myNetplanScheduler.cpp"
#include "workerRunTask.cpp"
#include "masterRunTask.cpp"

//int workerRunTask(MPI_Datatype message_to_master_type, int mySize, int myRank, int xRVSize, int objSize, MPI_Datatype xdata_to_workers_type, vector <double>& recvXDataVec, newCNSGA2& myNsga2, MPI_Datatype Mpara_to_workers_type, double eventsInput[]){}
		

void initPopPara(population* myPop , message_para_to_workers_VecT& myPopVec, initParaType& myIpara, int nodeSize, int myRank, vector<double>& xdataV) {
	int i, j, myindNum;
	// xreal[nreal]
	int nreal = myIpara.nreal;
	//int nbin  = myIpara.nbin;
	//int nbins = myIpara.nbins;
	int nobj  = myIpara.nobj;
	int ncon  = myIpara.ncon;
	int nbin  = myIpara.nbin;
	int popsize = myIpara.popsize;
	//vector<message_para_to_workersT>::iterator itr ;// data structure for master to send pop para to workers 
	
	//for (myindNum = 0, itr = myPopVec.begin() ; myindNum < popsize ; myindNum++, itr++)
	for (myindNum = 0 ; myindNum < popsize ; myindNum++) {
		// . precedence is higher than *
		//(*itr).xreal.clear();
		//(*itr).obj.clear();
		//(*itr).constr.clear();
		//myPopVec[myindNum].xreal.clear();
		//myPopVec[myindNum].obj.clear();
		//myPopVec[myindNum].constr.clear();
		#ifdef DEBUG_initPopPara
		cout << "myRank is " << myRank << " in initPopPara(), I am doing initialization for input myPop, this is myindNum " << myindNum << " , I am before initializing rank. \n\n" << endl;
		#endif
		//(*itr).rank 		= (myPop->ind[myindNum]).rank;
		(myPopVec[myindNum]).rank	= (myPop->ind[myindNum]).rank;
		#ifdef DEBUG_initPopPara
		cout << "myRank is " << myRank << " in initPopPara(), I am doing initialization for input myPop, this is myindNum " << myindNum << " , I have initilized rank. \n\n" << endl;
		#endif
		//(*itr).constr_violation = ((myPop->ind)[myindNum]).constr_violation;
		(myPopVec[myindNum]).constr_violation = ((myPop->ind)[myindNum]).constr_violation;
		#ifdef DEBUG_initPopPara
		cout << "myRank is " << myRank << " in initPopPara(), I am doing initialization for input myPop, this is myindNum " << myindNum << " , I have initilized constr_violation. \n\n" << endl;
		#endif
		for (i =0; i < nreal ; i++)  {
			//((*itr).xreal).push_back(((myPop->ind)[myindNum]).xreal[i]);
			//myPopVec[myindNum].xreal.push_back(((myPop->ind)[myindNum]).xreal[i]);
			((myPopVec[myindNum]).xreal)[i] = (((myPop->ind)[myindNum]).xreal[i]);
			//xdataStruct.xreal.push_back(((myPop->ind)[indNum]).xreal[i]);

			#ifdef DEBUG_initPopPara
			cout << "myRank is " << myRank << " in initPopPara(), I am doing initialization for input myPop, this is mymindNum " << myindNum << " , I have push_back xreal[" << i <<  "] , nreal is " << nreal << ", the sizeof(double) is " << sizeof(double) << " , the sizeof(((myPop->ind)[" << i << "]).xreal) is  "<< sizeof(((myPop->ind)[i]).xreal) << ", the sizeof((((myPop->ind)[myindNum]).xreal[" << i <<"])) is " << sizeof(((myPop->ind)[myindNum]).xreal[i]) << ", the value of (((myPop->ind)[myindNum]).xreal["<< i <<"]) is " << (((myPop->ind)[myindNum]).xreal[i]) << " , ((myPopVec[myindNum]).xreal)[" << i << "] is " << ((myPopVec[myindNum]).xreal)[i] << " \n\n" << endl; 
			//cout << "myRank is " << myRank << " in initPopPara(), I am doing initialization for input myPop, this is mymindNum " << myindNum << " , I have push_back xreal[" << i <<  "]. The sizeof(((*itr).xreal)) is " <<  sizeof(((*itr).xreal)) << ", nreal is " << nreal << ", the sizeof(double) is " << sizeof(double) << " , the sizeof(((myPop->ind)[" << i << "]).xreal) is  "<< sizeof(((myPop->ind)[i]).xreal) << ", the sizeof((((myPop->ind)[myindNum]).xreal[" << i <<"])) is " << sizeof(((myPop->ind)[myindNum]).xreal[i]) << ", the value of (((myPop->ind)[myindNum]).xreal["<< i <<"]) is " << (((myPop->ind)[myindNum]).xreal[i]) << " , the value of (*itr).xreal[" << i << "] is " << (*itr).xreal[i] << " , (*itr).xreal[i] size is "<< sizeof((*itr).xreal[i]) << " \n\n" << endl; 	
			#endif
		}
		#ifdef DEBUG_initPopPara
		cout << "myRank is " << myRank << " in initPopPara(), I am doing initialization for inputn myPop, this is myindNum " << myindNum << " , I have push_back xreal[]. nreal is " << nreal << ", the sizeof(double) is " << sizeof(double) << " , the sizeof(((myPop->ind)[0]).xreal) is  "<< sizeof(((myPop->ind)[0]).xreal) << " , (((myPop->ind)[0]).xreal) size is " << nreal  << " \n\n" << endl; 
		//cout << "myRank is " << myRank << " in initPopPara(), I am doing initialization for inputn myPop, this is myindNum " << myindNum << " , I have push_back xreal[]. The sizeof(xreal) is " <<  sizeof(((*itr).xreal)) << ", nreal is " << nreal << ", the sizeof(double) is " << sizeof(double) << " , the sizeof(((myPop->ind)[0]).xreal) is  "<< sizeof(((myPop->ind)[0]).xreal) << " , (((myPop->ind)[0]).xreal) size is " << nreal  << " \n\n" << endl; 
		#endif
		// gene[nbin][nbins]
		//for (i =0; i < nbin ; i++) 
		//	for (int j =0; j < nbins ; j++) 
		//		itr->gene[i][j] = myPop->ind[inGdNum]->gene[i][j];
		// xbin[nbin]
		//for (i =0; i < nbin ; i++) 
		//	myPopVec.xbin[i] = myPop->ind[indNum]->xbin[i];
		// obj[nobj]
		for (i =0; i < nobj ; i++) {
			//((*itr).obj).push_back(((myPop->ind)[myindNum]).obj[i]);
			//myPopVec[myindNum].obj.push_back(((myPop->ind)[myindNum]).obj[i]);
			((myPopVec[myindNum]).obj)[i] = (((myPop->ind)[myindNum]).obj[i]);
			//xdataStruct.obj.push_back(((myPop->ind)[indNum]).obj[i]);
			#ifdef DEBUG_initPopPara
			cout << "myRank is " << myRank << " in initPopPara(), I am doing initialization for input myPop obj, this is myindNum " << myindNum << " , I check push_back obj[" << i <<  "]. Its value is " << ((myPop->ind)[myindNum]).obj[i] << " , ((myPopVec[myindNum]).obj)[" << i <<"] is " << ((myPopVec[myindNum]).obj)[i] <<  " . \n\n" << endl; 
			#endif

		}
		#ifdef DEBUG_initPopPara
		cout << "myRank is " << myRank << " in initPopPara(), I am doing initialization for input myPop, this is myindNum " << myindNum << " , I have push_back obj[]. ncon is " << ncon <<  " \n\n" << endl; 
		#endif
		// constr[ncon]
		for (i =0; i < ncon ; i++) {
			//((*itr).constr).push_back(((myPop->ind)[myindNum]).constr[i]);
			//myPopVec[myindNum].constr.push_back(((myPop->ind)[myindNum]).constr[i]);
			((myPopVec[myindNum]).constr)[i] = (((myPop->ind)[myindNum]).constr[i]);
			//xdataStruct.constr.push_back(((myPop->ind)[indNum]).constr[i]);
			#ifdef DEBUG_initPopPara
			cout << "myRank is " << myRank << " in initPopPara(), I am doing initialization for input myPop constr, this is myindNum " << myindNum << " , I have push_back constr[" << i <<  "]. Its value is " << ((myPop->ind)[myindNum]).constr[i] << " , ((myPopVec[myindNum]).constr)[" << i << "] is " << ((myPopVec[myindNum]).constr)[i]  << " , ncon is" << ncon << " . \n\n" << endl; 
			#endif

		}
		#ifdef DEBUG_initPopPara
		cout << "myRank is " << myRank << " in initPopPara(), I am doing initialization for input myPop, this is myindNum " << myindNum << " , I have push_back constr[]. \n\n" << endl; 
		#endif
		// crowd_dist
		//(*itr).crowd_dist = ((myPop->ind)[myindNum]).crowd_dist;
		(myPopVec[myindNum]).crowd_dist = ((myPop->ind)[myindNum]).crowd_dist;
		
		#ifdef DEBUG_initPopPara
		cout << "myRank is " << myRank << " in initPopPara(), I am doing initialization for input myPop, this is myindNum " << myindNum << " , I have initalized crowd_dist. nbin = " << nbin << " sizeof(((myPop->ind)[myindNum]).xbin) is " << sizeof(((myPop->ind)[myindNum]).xbin) << " \n\n" << endl;
		#endif
		//initialize xbin
		for (i =0; i < nbin ; i++) {
			//((*itr).constr).push_back(((myPop->ind)[myindNum]).constr[i]);
			//myPopVec[myindNum].constr.push_back(((myPop->ind)[myindNum]).constr[i]);
			(myPopVec[myindNum]).xbin[i] = ((myPop->ind)[myindNum]).xbin[i];
			#ifdef DEBUG_initPopPara
			//cout << "myRank is " << myRank << " in initPopPara(), I have done initialization for input myPop xbin, this is myindNum " << myindNum << " , I have push_back xbin[" << i <<  "]. Its value is " << ((myPop->ind)[myindNum]).xbin[i] << " , nbin is " << nbin << " . \n\n" << endl; 
			
			//cout << "myRank is " << myRank << " in initPopPara(), I have done initialization for input myPop xbin, this is myindNum " << myindNum << " , I have push_back xbin[" << i <<  "]. Its value is " << ((myPop->ind)[myindNum]).xbin[i] << " , ((myPopVec[myindNum]).xbin)[" << i << "] is " << (myPopVec[myindNum]).xbin[i]  << " , nbin is " << nbin << " . \n\n" << endl; 
			#endif
			//xdataStruct.constr.push_back(((myPop->ind)[indNum]).constr[i]);
		}
	}
}
