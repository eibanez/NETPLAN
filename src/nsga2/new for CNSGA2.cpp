
/* Routine to send data to worker nodes */
void CNSGA2::sendPop(population *pop) {
	for (int i=0; i<popsize; i++) {
		// Send the following array to worker nodes (there it is called "variables")
		(&pop->ind[i])->xbin
	}
}


/* Routine to receive data from worker nodes */
void CNSGA2::sendPop(population *pop) {
	for (int i=0; i<popsize; i++) {
		// JINXU: Receive "objectives" array from worker node and store it in the variable below
		(&pop->ind[i])->obj
		(&pop->ind[i])->constr_violation = 0.0;							// JINXU: Leave this line in
	}
}

