#include "CFileIO.h"

CFileIO::CFileIO(CNSGA2* nsga2) {
	fpt1 = fopen("nsgadata/initial_pop.out","w");
	fpt2 = fopen("nsgadata/final_pop.out","w");
	fpt3 = fopen("nsgadata/best_pop.out","w");
	fpt4 = fopen("nsgadata/all_pop.out","w");
	fpt5 = fopen("nsgadata/params.out","w");
	fprintf(fpt1,"# This file contains the data of initial population\n");
	fprintf(fpt2,"# This file contains the data of final population\n");
	fprintf(fpt3,"# This file contains the data of final feasible population (if found)\n");
	fprintf(fpt4,"# This file contains the data of all generations\n");
	fprintf(fpt5,"# This file contains information about inputs as read by the program\n");
	
	p_nsga2 = nsga2;
}

CFileIO::~CFileIO(void) {
	flushIO();
	
	fclose(fpt1);
	fclose(fpt2);
	fclose(fpt3);
	fclose(fpt4);
	fclose(fpt5);
}

void CFileIO::flushIO() {
	fflush(stdout);
	fflush(fpt1);
	fflush(fpt2);
	fflush(fpt3);
	fflush(fpt4);
	fflush(fpt5);
}

void CFileIO::recordConfiguration() {
	fprintf(fpt5,"\n Population size = %d",p_nsga2->popsize);
	fprintf(fpt5,"\n Number of generations = %d",p_nsga2->ngen);
	fprintf(fpt5,"\n Number of objective functions = %d",p_nsga2->nobj);
	fprintf(fpt5,"\n Number of constraints = %d",p_nsga2->ncon);
	fprintf(fpt5,"\n Number of real variables = %d",p_nsga2->nreal);
	if (p_nsga2->nreal!=0) {
		for (int i=0; i < p_nsga2->nreal; i++) {
			fprintf(fpt5,"\n Lower limit of real variable %d = %e",i+1,p_nsga2->min_realvar[i]);
			fprintf(fpt5,"\n Upper limit of real variable %d = %e",i+1,p_nsga2->max_realvar[i]);
		}
		fprintf(fpt5,"\n Probability of crossover of real variable = %e",p_nsga2->pcross_real);
		fprintf(fpt5,"\n Probability of mutation of real variable = %e",p_nsga2->pmut_real);
		fprintf(fpt5,"\n Distribution index for crossover = %e",p_nsga2->eta_c);
		fprintf(fpt5,"\n Distribution index for mutation = %e",p_nsga2->eta_m);
	}
	fprintf(fpt5,"\n Number of binary variables = %d",p_nsga2->nbin);
	if (p_nsga2->nbin!=0) {
		for (int i = 0; i < p_nsga2->nbin; i++) {
			fprintf(fpt5,"\n Number of bits for binary variable %d = %d",i+1,p_nsga2->nbits[i]);
			fprintf(fpt5,"\n Lower limit of binary variable %d = %e",i+1,p_nsga2->min_binvar[i]);
			fprintf(fpt5,"\n Upper limit of binary variable %d = %e",i+1,p_nsga2->max_binvar[i]);
		}
		fprintf(fpt5,"\n Probability of crossover of binary variable = %e",p_nsga2->pcross_bin);
		fprintf(fpt5,"\n Probability of mutation of binary variable = %e",p_nsga2->pmut_bin);
	}
	fprintf(fpt5,"\n Seed for random number generator = %e",p_nsga2->randgen->seed);
	
	fprintf(fpt1,"# of objectives = %d, # of constraints = %d, # of real_var = %d, # of bits of bin_var = %d, constr_violation, rank, crowding_distance\n",p_nsga2->nobj,p_nsga2->ncon,p_nsga2->nreal,p_nsga2->bitlength);
	fprintf(fpt2,"# of objectives = %d, # of constraints = %d, # of real_var = %d, # of bits of bin_var = %d, constr_violation, rank, crowding_distance\n",p_nsga2->nobj,p_nsga2->ncon,p_nsga2->nreal,p_nsga2->bitlength);
	fprintf(fpt3,"# of objectives = %d, # of constraints = %d, # of real_var = %d, # of bits of bin_var = %d, constr_violation, rank, crowding_distance\n",p_nsga2->nobj,p_nsga2->ncon,p_nsga2->nreal,p_nsga2->bitlength);
	fprintf(fpt4,"# of objectives = %d, # of constraints = %d, # of real_var = %d, # of bits of bin_var = %d, constr_violation, rank, crowding_distance\n",p_nsga2->nobj,p_nsga2->ncon,p_nsga2->nreal,p_nsga2->bitlength);
}

/* Function to print the information of a population in a file */
void CFileIO::report_pop (population *pop, FILE *fpt) {
	for (int i=0; i < p_nsga2->popsize; i++) {
		// Objectives
		for (int j=0; j<p_nsga2->nobj; j++)
			fprintf(fpt,"%e\t",pop->ind[i].obj[j]);
		
		// Constraints?
		if (p_nsga2->ncon != 0) {
			for (int j=0; j<p_nsga2->ncon; j++)
				fprintf(fpt,"%e\t",pop->ind[i].constr[j]);
		}
		
		// Reals?
		if (p_nsga2->nreal != 0) {
			for (int j=0; j<p_nsga2->nreal; j++)
				fprintf(fpt,"%e\t",pop->ind[i].xreal[j]);
		}
		
		// Binary?
		if (p_nsga2->nbin!=0) {
			for (int j=0; j < p_nsga2->nbin; j++) {
				for (int k=0; k < p_nsga2->nbits[j]; k++)
					fprintf(fpt,"%d\t",pop->ind[i].gene[j][k]);
			}
		}
		
		// Other info
		fprintf(fpt,"%e\t",pop->ind[i].constr_violation);
		fprintf(fpt,"%d\t",pop->ind[i].rank);
		fprintf(fpt,"%e\n",pop->ind[i].crowd_dist);
	}
}

// Print the information of feasible and non-dominated population in a file
void CFileIO::report_feasible (population *pop, FILE *fpt) {
	for (int i=0; i<p_nsga2->popsize; i++) {
		if (pop->ind[i].constr_violation == 0.0 && pop->ind[i].rank==1) {
			for (int j = 0; j < p_nsga2->nobj; j++)
				fprintf(fpt,"%e\t",pop->ind[i].obj[j]);
			
			// Constraints?
			if (p_nsga2->ncon!=0) {
				for (int j = 0; j < p_nsga2->ncon; j++)
					fprintf(fpt,"%e\t",pop->ind[i].constr[j]);
			}
			
			// Real?
			if (p_nsga2->nreal!=0) {
				for (int j = 0 ; j < p_nsga2->nreal; j++)
					fprintf(fpt,"%e\t",pop->ind[i].xreal[j]);
			}
			
			// Binary?
			if (p_nsga2->nbin!=0) {
				for (int j=0; j<p_nsga2->nbin; j++)
				{
					for (int k=0; k<p_nsga2->nbits[j]; k++)
						fprintf(fpt,"%d\t",pop->ind[i].gene[j][k]);
				}
			}
			
			// Other
			fprintf(fpt,"%e\t",pop->ind[i].constr_violation);
			fprintf(fpt,"%d\t",pop->ind[i].rank);
			fprintf(fpt,"%e\n",pop->ind[i].crowd_dist);
		}
	}
	return;
}
