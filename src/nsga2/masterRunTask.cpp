/***************************************************************************
 *   Copyright (C) 2010-2011 by Jinxu Ding                                      *
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
matser run some tasks 
*******************************************************/
#include <iostream>
#include <vector>
#include <sys/utsname.h>
#include "globalName.h"
#include "defines.h"

// worker nodes recv task from manager node and then run it , after that return the result to manager		
int masterRunTask(population* myPop, vector <double>& resultTaskPackageT12, int beginInd, int nobj, int genNum, char candTag)
//int masterRunTask(population* myPop, vector <double>& resultTaskPackageT12, int beginInd, int nobj, int genNum, char candTag, CPLEX* netplan)
{       
	double starttime = MPI_Wtime(); static int masterRunTaskCounter = 0;
	// xVSize is size of xdata _ inbdNum, objSize is the size of return result
        int rank, mytaskTag=0, sourceRank, managerRank =0, destRank = 0, resultTask, i, myworkerUpStreamTaskTag=0;
	
	#ifdef DEBUG_MASTERRUNTASK

	cout << "In masterRunTask(), I am rank " << myRank << ", the matser seq run for Gen genNum = " << genNum << " , cand tag " << candTag << " beginInd is " << beginInd << " I am before ImportIndices " <<  " \n\n " << endl;
	#endif
	// Read global parameters
	//ReadParameters("data/parameters.csv");
	/*
	if (masterRunTaskCounter == 0)
	{
	// Import indices to export data
	ImportIndices();
	
	// Declare variables to store the optimization model
	CPLEX netplan;
	
	// Read master and subproblems
	netplan.LoadProblem();
	}	
	++masterRunTaskCounter;
	*/
	// Vector of capacity losses for events
	double events[(SLength[0] + IdxCap.GetSize()) * (Nevents+1)];
	ReadEvents(events, "prepdata/bend_events.csv");
	
	#ifdef DEBUG_MASTERRUNTASK
	cout << "In masterRunTask(), I am rank " << myRank << ", the seq run for genNum " << genNum << " candTag " << candTag << " , I am before nsga2.mEvaluatePopInd \n\n " << endl;
	#endif
	population* 	myPopul;  
	// nsga2->evaluatePop(nsga2->child_pop, jji, nconNum , 0); // int paraFlag =0 for seq, evaluate different ind indexed by jji per time
	//nsga2->evaluatePop(nsga2->child_pop, events); 	// master solve all pop ind seq. 
	newCNSGA2 nsga2(1); 	
	//CNSGA2 nsga2;
	nsga2.mEvaluatePopInd(myPop, events, resultTaskPackageT12, beginInd, nobj, genNum, candTag, myRank); 	// master solve all pop ind seq.
	//nsga2.mEvaluatePopInd(myPop, events, resultTaskPackageT12, beginInd, nobj, genNum, candTag, myRank, netplan); 	// master solve all pop ind seq.	
				
	//}
	double endtime = MPI_Wtime();	
	#ifdef DEBUG_MASTERRUNTASK
	cout << "In masterRunTask(), I am rank " << myRank << ", the seq run for genNum " << genNum << " candTag " << candTag << " for ind " << beginInd << " , I am after nsga2.mEvaluatePopInd , I used " << endtime - starttime <<  " seconds " << endl;
	#endif
			
	return 0;
}



// EOF
