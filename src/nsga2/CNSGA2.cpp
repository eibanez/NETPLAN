#include "../solver.h"
#include "CNSGA2.h"

CNSGA2::CNSGA2(bool output, double seed) {
	randgen = new CRand(seed);
	if (output) fileio = new CFileIO(this);
	quicksort = new CQuicksort(randgen);
	linkedlist = new CLinkedList();
}

CNSGA2::~CNSGA2(void) {
	delete randgen;
	delete fileio;
	delete quicksort;
	delete linkedlist;
	
	if (nreal != 0) {
		free (min_realvar);
		free (max_realvar);
	}
	if (nbin!=0) {
		free (min_binvar);
		free (max_binvar);
		free (nbits);
	}
	
	deallocate_memory_pop (parent_pop, popsize);
	deallocate_memory_pop (child_pop, popsize);
	deallocate_memory_pop (mixed_pop, 2*popsize);
	
	free (parent_pop);
	free (child_pop);
	free (mixed_pop);
}

void CNSGA2::Init(const char* param) {
	FILE *file = fopen(param, "r");
	if (file != NULL) {
		char line[100];
		// Size of population (multiple of 4)
		fgets(line, sizeof line, file);
		popsize = strtol(line, NULL, 10);
		// Number of generations
		fgets(line, sizeof line, file);
		ngen = strtol(line, NULL, 10);
		// Number of objectives
		fgets(line, sizeof line, file);
		nobj = strtol(line, NULL, 10);
		// Number of constraints
		fgets(line, sizeof line, file);
		ncon = strtol(line, NULL, 10);
		// Number of real variables
		fgets(line, sizeof line, file);
		nreal = strtol(line, NULL, 10);
		
		// Min-max variables
		if (nreal != 0) {
			min_realvar = (double *)malloc(nreal*sizeof(double));
			max_realvar = (double *)malloc(nreal*sizeof(double));
			for (int i = 0; i < nreal; i++) {
				fgets(line, sizeof line, file);
				char* t_read = strtok(line," ");
				min_realvar[i] = strtod(t_read, NULL);
				t_read = strtok(NULL," ");
				max_realvar[i] = strtod(t_read, NULL);
			}
		}
		
		// the probability of crossover of real variable (0.6-1.0)
		fgets(line, sizeof line, file);
		pcross_real = strtod(line, NULL);
		// the probablity of mutation of real variables (1/nreal)
		fgets(line, sizeof line, file);
		pmut_real = strtod(line, NULL);
		// the value of distribution index for crossover (5-20)
		fgets(line, sizeof line, file);
		eta_c = strtod(line, NULL);
		// the value of distribution index for mutation (5-50)
		fgets(line, sizeof line, file);
		eta_m = strtod(line, NULL);
		
		// Number of binary variables
		fgets(line, sizeof line, file);
		nbin = strtol(line, NULL, 10);
		bitlength = 0;
	
		if (nbin != 0) {
			nbits = (int *)malloc(nbin*sizeof(int));
			min_binvar = (double *)malloc(nbin*sizeof(double));
			max_binvar = (double *)malloc(nbin*sizeof(double));
			
			for (int i = 0; i < nbin; i++) {
				// the number of bits for binary variable i
				fgets(line, sizeof line, file);
				char* t_read = strtok(line," ");
				nbits[i] = strtol(t_read, NULL, 10);
				// lower limit for binary variable i
				t_read = strtok(NULL," ");
				min_binvar[i] = strtod(t_read, NULL);
				// upper limit for binary variable i
				t_read = strtok(NULL," ");
				max_binvar[i] = strtod(t_read, NULL);
			}
			
			// the probability of crossover of binary variable (0.6-1.0)
			fgets(line, sizeof line, file);
			pcross_bin = strtod(line, NULL);
			// the probability of mutation of binary variables (1/nbits)
			fgets(line, sizeof line, file);
			pmut_bin = strtod(line, NULL);
		}
		
		for (int i = 0; i < nbin; i++)
			bitlength += nbits[i];
	}
	
	fclose(file);
	
	nbinmut = 0;
	nrealmut = 0;
	nbincross = 0;
	nrealcross = 0;
}

// This function allocates the memory needed for the given populations
void CNSGA2::InitMemory() {
	parent_pop  = (population *)malloc(sizeof(population));
	child_pop   = (population *)malloc(sizeof(population));
	mixed_pop   = (population *)malloc(sizeof(population));
	
	allocate_memory_pop(parent_pop, popsize);
	allocate_memory_pop(child_pop, popsize);
	allocate_memory_pop(mixed_pop, 2*popsize);
}

// Function to allocate memory to a population
void CNSGA2::allocate_memory_pop(population *pop, int size) {
	pop->ind = (individual *)malloc(size*sizeof(individual));
	for (int i=0; i < size; i++)
		allocate_memory_ind (&(pop->ind[i]));
}

// Function to allocate memory to an individual
void CNSGA2::allocate_memory_ind(individual *ind) {
	int j;
	if (nreal != 0) {
		ind->xreal = (double *)malloc(nreal*sizeof(double));
	}
	if (nbin != 0) {
		ind->xbin = (double *)malloc(nbin*sizeof(double));
		ind->gene = (int **)malloc(nbin*sizeof(int *));
		for (j=0; j<nbin; j++)
			ind->gene[j] = (int *)malloc(nbits[j]*sizeof(int));
	}
	ind->obj = (double *)malloc(nobj*sizeof(double));
	if (ncon != 0) {
		ind->constr = (double *)malloc(ncon*sizeof(double));
	}
	return;
}

// Function to deallocate memory to a population
void CNSGA2::deallocate_memory_pop(population *pop, int size) {
	for (int i = 0; i < size; i++)
		deallocate_memory_ind (&(pop->ind[i]));
	free (pop->ind);
}

// Function to deallocate memory to an individual
void CNSGA2::deallocate_memory_ind(individual *ind) {
	if (nreal != 0)
		free(ind->xreal);
	
	if (nbin != 0) {
		for (int j=0; j<nbin; j++)
			free(ind->gene[j]);
		free(ind->xbin);
		free(ind->gene);
	}
	free(ind->obj);
	if (ncon != 0)
		free(ind->constr);
}

// Initialize a population randomly
void CNSGA2::InitPop(population *pop, double prob) {
	for (int i = 0; i < popsize; i++)
		InitInd(&(pop->ind[i]), prob);
}

// Randomly initialize individuals
void CNSGA2::InitInd(individual *ind, double prob) {
	// Initialize real variables
	if (nreal!=0) {
		for (int j = 0; j < nreal; j++)
			ind->xreal[j] = randgen->rndreal (min_realvar[j], max_realvar[j]);
	}
	
	// Initialize binary variables
	if (nbin!=0) {
		for (int j = 0; j < nbin; j++) {
			for (int k = 0; k < nbits[j]; k++) {
				if (randgen->randomperc() >= prob)
					ind->gene[j][k] = 0;
				else
					ind->gene[j][k] = 1;
			}
		}
	}
}

// Resume a population
void CNSGA2::ResumePop(population *pop, const char* fileinput) {
	FILE *file = fopen(fileinput, "r");
	char line[10000];
	int number_imports = 0;
	
	if (file != NULL) {
		// Discard first two lines (comments)
		fgets(line, sizeof line, file);
		fgets(line, sizeof line, file);
		
		for (int i = 0; i < popsize; i++) {
			// Read a line from the file and finish if empty is read
			char tmp[80];
			int output = fscanf(file, "%s", tmp);
			
			if (output == EOF) {
				break;
			}
			
			// Skip objectives
			for (int j=1; j < Nobj; ++j) {
				fscanf (file, "%s", tmp);
			}
			
			// Initialize binary variables
			int f;
			for (int j = 0; j < nbin; j++) {
				for (int k = 0; k < nbits[j]; k++) {
					fscanf (file, "%d", &f);
					(&pop->ind[i])->gene[j][k] = f;
				}
			}
			
			// Skip last 3 parameters in the line
			for (int j=0; j < 3; ++j) {
				fscanf (file, "%s", tmp);
			}
			++number_imports;
		}
		cout << "- Succesfully imported " << number_imports << " initial solutions" << endl;
	} else {
		cout << "\tWarning: Initial NSGA-II result file" << fileinput << " not found!" << endl;
	}
}

// Decode a population to find out the binary variable values based on its bit pattern
void CNSGA2::decodePop(population *pop) {
	if (nbin!=0) {
		for (int i = 0; i < popsize; i++)
			decodeInd (&(pop->ind[i]));
	}
}

// Decode an individual to find out the binary variable values based on its bit pattern
void CNSGA2::decodeInd(individual *ind) {
	int sum;
	if (nbin!=0) {
		for (int j = 0; j < nbin; j++) {
			sum=0;
			for (int k=0; k<nbits[j]; k++) {
				if (ind->gene[j][k]==1)
					sum += (int)pow((double)2,nbits[j]-1-k);
			}
			ind->xbin[j] = min_binvar[j] + (double)sum*(max_binvar[j] - min_binvar[j])/(double)(pow(2.0f,nbits[j])-1);
		}
	}
}

/* Routine to evaluate objective function values and constraints for a population */
void CNSGA2::evaluatePop(population *pop, CPLEX& netplan, const double events[]) {
	for (int i=0; i<popsize; i++) {
		cout << "\tIndividual: " << i+1 << endl;
		netplan.SolveProblem((&pop->ind[i])->xbin, (&pop->ind[i])->obj, events);
		(&pop->ind[i])->constr_violation = 0.0;
		//evaluateInd (&(pop->ind[i]), events, netplan);
	}
}

/* Routine to evaluate objective function values and constraints for a population */
void CNSGA2::sendPop(population *pop) {
	for (int i=0; i<popsize; i++) {
		// Send to queue:		(&pop->ind[i])->xbin,
		// (&pop->ind[i])->obj, events);						// JINXU: Connect this line to nsga2-individual.cpp
		(&pop->ind[i])->constr_violation = 0.0;
		//evaluateInd (&(pop->ind[i]), events, netplan);
	}
}

/* Routine to evaluate objective function values and constraints for a population */
void CNSGA2::receivePop(population *pop) {
	for (int i=0; i<popsize; i++) {
		// Receive from workers:  (&pop->ind[i])->obj			// JINXU: Connect this line to nsga2-individual.cpp
		(&pop->ind[i])->constr_violation = 0.0;
	}
}

/* Routine to evaluate objective function values and constraints for an individual */
/*void CNSGA2::evaluateInd(individual *ind, const double events[], CPLEX& netplan) {
	netplan.SolveProblem(ind->xbin, ind->obj, events);
	// test_problem (ind->xreal, ind->xbin, ind->gene, ind->obj, ind->constr);
	if (ncon==0)
		ind->constr_violation = 0.0;
	else {
		ind->constr_violation = 0.0;
		for (int j=0; j<ncon; j++) {
			if (ind->constr[j]<0.0)
				ind->constr_violation += ind->constr[j];
		}
	}
}*/

// Assign rank and crowding distance to a population of size pop_size
void CNSGA2::assignRankCrowdingDistance(population *new_pop) {
	int flag, end;
	
	int front_size 	= 0;
	int rank 		= 1;
	
	list *orig;
	list *cur;
	list *temp1, *temp2;
	
	orig = (list *)malloc(sizeof(list));
	cur  = (list *)malloc(sizeof(list));
	
	orig->index = -1;
	orig->parent = NULL;
	orig->child = NULL;
	
	cur->index = -1;
	cur->parent = NULL;
	cur->child = NULL;
	
	temp1 = orig;
		
	for (int i = 0; i < popsize; i++) {
		linkedlist->insert (temp1,i);
		temp1 = temp1->child;
	}
	do {
		if (orig->child->child == NULL) {
			new_pop->ind[orig->child->index].rank = rank;
			new_pop->ind[orig->child->index].crowd_dist = INF;
			break;
		}
		temp1 = orig->child;
		linkedlist->insert (cur, temp1->index);
		front_size = 1;
		
		temp2 = cur->child;
		temp1 = linkedlist->del (temp1);
		temp1 = temp1->child;
		do {
			temp2 = cur->child;
			do {
				end = 0;
				flag = checkDominance (&(new_pop->ind[temp1->index]), &(new_pop->ind[temp2->index]));
				if (flag == 1) {
					linkedlist->insert (orig, temp2->index);
					temp2 = linkedlist->del (temp2);
					front_size--;
					temp2 = temp2->child;
				}
				
				if (flag == 0)
					temp2 = temp2->child;
				
				if (flag == -1)
					end = 1;
			} while (end!=1 && temp2!=NULL);
			if (flag == 0 || flag == 1) {
				linkedlist->insert (cur, temp1->index);
				front_size++;
				temp1 = linkedlist->del (temp1);
			}
			temp1 = temp1->child;
		} while (temp1 != NULL);
		
		temp2 = cur->child;
		do {
			new_pop->ind[temp2->index].rank = rank;
			temp2 = temp2->child;
		} while (temp2 != NULL);
		
		assignCrowdingDistanceList(new_pop, cur->child, front_size);
		temp2 = cur->child;
		do {
			temp2 = linkedlist->del(temp2);
			temp2 = temp2->child;
		} while (cur->child !=NULL);
		rank+=1;
	} while (orig->child!=NULL);
	
	free (orig);
	free (cur);
	free (temp1); free (temp2);  // Added by Eduardo
}


/* Routine for usual non-domination checking
   It will return the following values
   1 if a dominates b
   -1 if b dominates a
   0 if both a and b are non-dominated */
int CNSGA2::checkDominance(individual *a, individual *b) {
	int flag1 = 0;
	int flag2 = 0;
	
	if (a->constr_violation<0 && b->constr_violation<0) {
		if (a->constr_violation > b->constr_violation)
			return (1);
		else {
			if (a->constr_violation < b->constr_violation)
				return (-1);
			else
				return (0);
		}
	} else {
		if (a->constr_violation < 0 && b->constr_violation == 0)
			return (-1);
		else {
			if (a->constr_violation == 0 && b->constr_violation <0)
				return (1);
			else {
				for (int i = 0; i < nobj; i++) {
					if (a->obj[i] < b->obj[i])
						flag1 = 1;
					else {
						if (a->obj[i] > b->obj[i])
							flag2 = 1;
					}
				}
				if (flag1==1 && flag2==0)
					return (1);
				else {
					if (flag1==0 && flag2==1)
						return (-1);
					else
						return (0);
				}
			}
		}
	}
}

// Method to compute crowding distance based on objective function values when the population in in the form of a list
void CNSGA2::assignCrowdingDistanceList(population *pop, list *lst, int front_size) {
	int **obj_array;
	int *dist;
	
	list *temp;
	temp = lst;
	
	if (front_size==1) {
		pop->ind[lst->index].crowd_dist = INF;
		return;
	}
	
	if (front_size==2) {
		pop->ind[lst->index].crowd_dist = INF;
		pop->ind[lst->child->index].crowd_dist = INF;
		return;
	}
		
	obj_array = (int **)malloc(nobj*sizeof(int));
	dist = (int *)malloc(front_size*sizeof(int));

	for (int i = 0; i < nobj; i++)
		obj_array[i] = (int *)malloc(front_size*sizeof(int));
	
	for (int i = 0; i < front_size; i++) {
		dist[i] = temp->index;
		temp = temp->child;
	}
	
	assignCrowdingDistance (pop, dist, obj_array, front_size);
	
	free (dist);
	
	for (int i = 0; i < nobj; i++)
		free (obj_array[i]);
	
	free (obj_array);
	
	free (temp);  // Added by Eduardo
}

/* Routine to compute crowding distances */
void CNSGA2::assignCrowdingDistance(population *pop, int *dist, int **obj_array, int front_size) {
	for (int i = 0; i < nobj; i++) {
		for (int j = 0; j < front_size; j++)
			obj_array[i][j] = dist[j];
		
		quicksort->quicksort_front_obj (pop, i, obj_array[i], front_size);
	}
	for (int i = 0; i < front_size; i++)
		pop->ind[dist[i]].crowd_dist = 0.0;
	
	for (int i = 0; i < nobj; i++)
		pop->ind[obj_array[i][0]].crowd_dist = INF;
	
	for (int i = 0; i < nobj; i++) {
		for (int j = 1; j < front_size-1; j++) {
			if (pop->ind[obj_array[i][j]].crowd_dist != INF) {
				if (pop->ind[obj_array[i][front_size-1]].obj[i] == pop->ind[obj_array[i][0]].obj[i])
					pop->ind[obj_array[i][j]].crowd_dist += 0.0;
				else
					pop->ind[obj_array[i][j]].crowd_dist += (pop->ind[obj_array[i][j+1]].obj[i] - pop->ind[obj_array[i][j-1]].obj[i])/(pop->ind[obj_array[i][front_size-1]].obj[i] - pop->ind[obj_array[i][0]].obj[i]);
			}
		}
	}
	for (int j = 0; j < front_size; j++) {
		if (pop->ind[dist[j]].crowd_dist != INF)
			pop->ind[dist[j]].crowd_dist = (pop->ind[dist[j]].crowd_dist)/nobj;
	}
}

/* Routine to compute crowding distance based on objective function values when the population in in the form of an array */
void CNSGA2::assignCrowdingDistanceIndices(population *pop, int c1, int c2) {
	int **obj_array;
	int *dist;
	int i, j;
	int front_size;
	front_size = c2-c1+1;
	if (front_size==1) {
		pop->ind[c1].crowd_dist = INF;
		return;
	}
	if (front_size==2) {
		pop->ind[c1].crowd_dist = INF;
		pop->ind[c2].crowd_dist = INF;
		return;
	}
	
	obj_array = (int **)malloc(nobj*sizeof(int));
	dist = (int *)malloc(front_size*sizeof(int));
	for (i=0; i<nobj; i++)
		obj_array[i] = (int *)malloc(front_size*sizeof(int));
	
	for (j=0; j<front_size; j++)
		dist[j] = c1++;
	
	assignCrowdingDistance (pop, dist, obj_array, front_size);
	free (dist);
	for (i=0; i<nobj; i++)
		free (obj_array[i]);
	
	free (obj_array);
}

/* Routine for tournament selection, it creates a new_pop from old_pop by performing tournament selection and the crossover */
void CNSGA2::selection(population *old_pop, population *new_pop) {
	int *a1, *a2;
	int temp, rand;
	
	individual *parent1, *parent2;
	
	a1 = (int *)malloc(popsize*sizeof(int));
	a2 = (int *)malloc(popsize*sizeof(int));
	
	for (int i=0; i<popsize; i++)
		a1[i] = a2[i] = i;
	
	for (int i=0; i<popsize; i++) {
		rand = randgen->rnd (i, popsize-1);
		temp = a1[rand];
		a1[rand] = a1[i];
		a1[i] = temp;
		rand = randgen->rnd (i, popsize-1);
		temp = a2[rand];
		a2[rand] = a2[i];
		a2[i] = temp;
	}
	
	for (int i=0; i<popsize; i+=4) {
		parent1 = tournament (&old_pop->ind[a1[i]], &old_pop->ind[a1[i+1]]);
		parent2 = tournament (&old_pop->ind[a1[i+2]], &old_pop->ind[a1[i+3]]);
		crossover (parent1, parent2, &new_pop->ind[i], &new_pop->ind[i+1]);

		parent1 = tournament (&old_pop->ind[a2[i]], &old_pop->ind[a2[i+1]]);
		parent2 = tournament (&old_pop->ind[a2[i+2]], &old_pop->ind[a2[i+3]]);
		crossover (parent1, parent2, &new_pop->ind[i+2], &new_pop->ind[i+3]);
	}
	free (a1);
	free (a2);
}

/* Routine for binary tournament */
individual* CNSGA2::tournament(individual *ind1, individual *ind2) {
	int flag;
	flag = checkDominance (ind1, ind2);
	if (flag==1)
		return (ind1);
	
	if (flag==-1)
		return (ind2);
	
	if (ind1->crowd_dist > ind2->crowd_dist)
		return(ind1);
	
	if (ind2->crowd_dist > ind1->crowd_dist)
		return(ind2);
	
	if ((randgen->randomperc()) <= 0.5)
		return(ind1);
	
	else
		return(ind2);
}

/* Function to cross two individuals */
void CNSGA2::crossover(individual *parent1, individual *parent2, individual *child1, individual *child2) {
	if (nreal!=0)
		realcross(parent1, parent2, child1, child2);
	if (nbin!=0)
		bincross(parent1, parent2, child1, child2);
}

/* Routine for real variable SBX crossover */
void CNSGA2::realcross(individual *parent1, individual *parent2, individual *child1, individual *child2) {
	int i;
	double rand;
	double y1, y2, yl, yu;
	double c1, c2;
	double alpha, beta, betaq;
	if (randgen->randomperc() <= pcross_real) {
		nrealcross++;
		for (i=0; i<nreal; i++) {
			if (randgen->randomperc()<=0.5) {
				if (fabs(parent1->xreal[i]-parent2->xreal[i]) > EPS) {
					if (parent1->xreal[i] < parent2->xreal[i]) {
						y1 = parent1->xreal[i];
						y2 = parent2->xreal[i];
					} else {
						y1 = parent2->xreal[i];
						y2 = parent1->xreal[i];
					}
					
					yl = min_realvar[i];
					yu = max_realvar[i];
					
					rand = randgen->randomperc();
					
					beta = 1.0 + (2.0*(y1-yl)/(y2-y1));
					alpha = 2.0 - pow(beta,-(eta_c+1.0));
					if (rand <= (1.0/alpha))
						betaq = pow ((rand*alpha),(1.0/(eta_c+1.0)));
					
					else
						betaq = pow ((1.0/(2.0 - rand*alpha)),(1.0/(eta_c+1.0)));
					
					c1 = 0.5*((y1+y2)-betaq*(y2-y1));
					beta = 1.0 + (2.0*(yu-y2)/(y2-y1));
					alpha = 2.0 - pow(beta,-(eta_c+1.0));
					
					if (rand <= (1.0/alpha))
						betaq = pow ((rand*alpha),(1.0/(eta_c+1.0)));
					
					else
						betaq = pow ((1.0/(2.0 - rand*alpha)),(1.0/(eta_c+1.0)));
					
					c2 = 0.5*((y1+y2)+betaq*(y2-y1));
					
					if (c1<yl)
						c1=yl;
					if (c2<yl)
						c2=yl;
					if (c1>yu)
						c1=yu;
					if (c2>yu)
						c2=yu;
					
					if (randgen->randomperc()<=0.5) {
						child1->xreal[i] = c2;
						child2->xreal[i] = c1;
					} else {
						child1->xreal[i] = c1;
						child2->xreal[i] = c2;
					}
				} else {
					child1->xreal[i] = parent1->xreal[i];
					child2->xreal[i] = parent2->xreal[i];
				}
			} else {
				child1->xreal[i] = parent1->xreal[i];
				child2->xreal[i] = parent2->xreal[i];
			}
		}
	} else {
		for (i=0; i<nreal; i++) {
			child1->xreal[i] = parent1->xreal[i];
			child2->xreal[i] = parent2->xreal[i];
		}
	}
	return;
}

/* Routine for two point binary crossover */
void CNSGA2::bincross(individual *parent1, individual *parent2, individual *child1, individual *child2) {
	int i, j;
	int temp, site1, site2;
	for (i=0; i<nbin; i++) {
		if (randgen->randomperc() <= pcross_bin) {
			nbincross++;
			site1 = randgen->rnd(0,nbits[i]-1);
			site2 = randgen->rnd(0,nbits[i]-1);
			if (site1 > site2) {
				temp = site1;
				site1 = site2;
				site2 = temp;
			}
			for (j=0; j<site1; j++) {
				child1->gene[i][j] = parent1->gene[i][j];
				child2->gene[i][j] = parent2->gene[i][j];
			}
			for (j=site1; j<site2; j++) {
				child1->gene[i][j] = parent2->gene[i][j];
				child2->gene[i][j] = parent1->gene[i][j];
			}
			for (j=site2; j<nbits[i]; j++) {
				child1->gene[i][j] = parent1->gene[i][j];
				child2->gene[i][j] = parent2->gene[i][j];
			}
		} else {
			for (j=0; j<nbits[i]; j++) {
				child1->gene[i][j] = parent1->gene[i][j];
				child2->gene[i][j] = parent2->gene[i][j];
			}
		}
	}
	return;
}

/* Function to perform mutation in a population */
void CNSGA2::mutatePop(population *pop) {
	for (int i=0; i < popsize; i++)
		mutateInd(&(pop->ind[i]));
}

/* Function to perform mutation of an individual */
void CNSGA2::mutateInd (individual *ind) {
	if (nreal!=0)
		realMutateInd(ind);
	
	if (nbin!=0)
		binMutateInd(ind);
}

/* Routine for binary mutation of an individual */
void CNSGA2::binMutateInd (individual *ind) {
	for (int j = 0; j < nbin; j++) {
		for (int k = 0; k < nbits[j]; k++) {
			if (randgen->randomperc() <= pmut_bin) {
				if (ind->gene[j][k] == 0)
					ind->gene[j][k] = 1;
				else
					ind->gene[j][k] = 0;
				nbinmut+=1;
			}
		}
	}
}

/* Routine for real polynomial mutation of an individual */
void CNSGA2::realMutateInd(individual *ind) {
	double rnd, delta1, delta2, mut_pow, deltaq;
	double y, yl, yu, val, xy;
	for (int j = 0; j < nreal; j++) {
		if (randgen->randomperc() <= pmut_real) {
			y = ind->xreal[j];
			yl = min_realvar[j];
			yu = max_realvar[j];
			delta1 = (y-yl)/(yu-yl);
			delta2 = (yu-y)/(yu-yl);
			
			rnd = randgen->randomperc();
			mut_pow = 1.0/(eta_m+1.0);
			
			if (rnd <= 0.5) {
				xy = 1.0-delta1;
				val = 2.0*rnd+(1.0-2.0*rnd)*(pow(xy,(eta_m+1.0)));
				deltaq =  pow(val,mut_pow) - 1.0;
			} else {
				xy = 1.0-delta2;
				val = 2.0*(1.0-rnd)+2.0*(rnd-0.5)*(pow(xy,(eta_m+1.0)));
				deltaq = 1.0 - (pow(val,mut_pow));
			}
			y = y + deltaq*(yu-yl);
			if (y<yl)
				y = yl;
			if (y>yu)
				y = yu;
			ind->xreal[j] = y;
			nrealmut+=1;
		}
	}
	return;
}

/* Routine to merge two populations into one */
void CNSGA2::merge(population *pop1, population *pop2, population *pop3) {
	int i, k;
	for (i = 0; i < popsize; i++)
		copyInd (&(pop1->ind[i]), &(pop3->ind[i]));

	for (i = 0, k = popsize; i < popsize; i++, k++)
		copyInd (&(pop2->ind[i]), &(pop3->ind[k]));
}

/* Routine to copy an individual 'ind1' into another individual 'ind2' */
void CNSGA2::copyInd(individual *ind1, individual *ind2) {
	ind2->rank = ind1->rank;
	ind2->constr_violation = ind1->constr_violation;
	ind2->crowd_dist = ind1->crowd_dist;
	
	if (nreal!=0) {
		for (int i = 0; i < nreal; i++)
			ind2->xreal[i] = ind1->xreal[i];
	}
	if (nbin!=0) {
		for (int i = 0; i < nbin; i++) {
			ind2->xbin[i] = ind1->xbin[i];
			for (int j = 0; j < nbits[i]; j++)
				ind2->gene[i][j] = ind1->gene[i][j];
		}
	}
	for (int i = 0; i < nobj; i++)
		ind2->obj[i] = ind1->obj[i];
		
	if (ncon!=0) {
		for (int i = 0; i < ncon; i++)
			ind2->constr[i] = ind1->constr[i];
	}
}

/* Routine to perform non-dominated sorting */
void CNSGA2::fillNondominatedSort (population *mixed_pop, population *new_pop) {
	int flag;
	int i, j;
	int end;
	int front_size;
	int archieve_size;
	int rank=1;
	list *pool;
	list *elite;
	list *temp1, *temp2;
	pool = (list *)malloc(sizeof(list));
	elite = (list *)malloc(sizeof(list));
	front_size = 0;
	archieve_size=0;
	pool->index = -1;
	pool->parent = NULL;
	pool->child = NULL;
	elite->index = -1;
	elite->parent = NULL;
	elite->child = NULL;
	temp1 = pool;
	for (i=0; i<2*popsize; i++) {
		linkedlist->insert (temp1,i);
		temp1 = temp1->child;
	}
	i=0;
	do {
		temp1 = pool->child;
		linkedlist->insert (elite, temp1->index);
		front_size = 1;
		temp2 = elite->child;
		temp1 = linkedlist->del (temp1);
		temp1 = temp1->child;
		do {
			temp2 = elite->child;
			if (temp1==NULL)
				break;
			
			do {
				end = 0;
				flag = checkDominance (&(mixed_pop->ind[temp1->index]), &(mixed_pop->ind[temp2->index]));
				if (flag == 1) {
					linkedlist->insert (pool, temp2->index);
					temp2 = linkedlist->del (temp2);
					front_size--;
					temp2 = temp2->child;
				}
				
				if (flag == 0)
					temp2 = temp2->child;
				
				if (flag == -1)
					end = 1;
			} while (end!=1 && temp2!=NULL);
			if (flag == 0 || flag == 1) {
				linkedlist->insert (elite, temp1->index);
				front_size++;
				temp1 = linkedlist->del (temp1);
			}
			temp1 = temp1->child;
		} while (temp1 != NULL);
		temp2 = elite->child;
		j=i;
		if ((archieve_size+front_size) <= popsize) {
			do {
				copyInd (&mixed_pop->ind[temp2->index], &new_pop->ind[i]);
				new_pop->ind[i].rank = rank;
				archieve_size+=1;
				temp2 = temp2->child;
				i+=1;
			} while (temp2 != NULL);
			assignCrowdingDistanceIndices(new_pop, j, i-1);
			rank+=1;
		} else {
			crowdingFill (mixed_pop, new_pop, i, front_size, elite);
			archieve_size = popsize;
			for (j=i; j<popsize; j++)
				new_pop->ind[j].rank = rank;
		}
		temp2 = elite->child;
		do {
			temp2 = linkedlist->del (temp2);
			temp2 = temp2->child;
		} while (elite->child !=NULL);
	} while (archieve_size < popsize);
	while (pool!=NULL) {
		temp1 = pool;
		pool = pool->child;
		free (temp1);
	}
	while (elite!=NULL) {
		temp1 = elite;
		elite = elite->child;
		free (temp1);
	}
	
	free (temp2); free (pool); free(elite); // Added by Eduardo
}

/* Routine to fill a population with individuals in the decreasing order of crowding distance */
void CNSGA2::crowdingFill(population *mixed_pop, population *new_pop, int count, int front_size, list *elite) {
	int *dist;
	list *temp;
	int i, j;
	assignCrowdingDistanceList (mixed_pop, elite->child, front_size);
	dist = (int *)malloc(front_size*sizeof(int));
	temp = elite->child;
	
	for (j=0; j<front_size; j++) {
		dist[j] = temp->index;
		temp = temp->child;
	}
	
	quicksort->quicksort_dist (mixed_pop, dist, front_size);
	for (i=count, j=front_size-1; i<popsize; i++, j--)
		copyInd(&mixed_pop->ind[dist[j]], &new_pop->ind[i]);
	
	free (dist);
	free (temp); // Added by Eduardo
}


/*void CNSGA2::test_problem(double *xreal, double *xbin, int **gene, double *objective, double *constr) {
	double f1, f2, g, h;
	int i;
	f1 = 1.0 - (exp(-4.0*xreal[0]))*pow((sin(4.0*PI*xreal[0])),6.0);
	g = 0.0;
	for (i=1; i<10; i++) {
		g += xreal[i];
	}
	g = g/9.0;
	g = pow(g,0.25);
	g = 1.0 + 9.0*g;
	h = 1.0 - pow((f1/g),2.0);
	f2 = g*h;
	
	double g, h;
	g = 0.0f;
	for (int i = 1; i < 10; i++) {
		g += xreal[i];
	}
	
	h = 1/g;
	
	obj[0] = g; //f1;
	obj[1] = h; //f2;
	
	SolveProblem(xbin, objective);
	
	return;
}*/
