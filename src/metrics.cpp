// --------------------------------------------------------------
//    NETSCORE Version 1
//    metrics.cpp -- Implementation of metrics
//    2009-2010 (c) Eduardo Ibáñez
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "index.h"
#include "metrics.h"

double MinAverage(vector<double>& v, Index Idx, const int start) {
	// This function calculates the average value of reserve margin for each year and then selects the minimum
	vector<double> avg = AvgByRow(v, Idx, start);
	
	double min = avg[0];
	for (unsigned int i = 1; i < avg.size(); ++i)
		if (avg[i] < min) min = avg[i];
	
	return min;	
}

double EmissionIndex(vector<double>& v, const int start) {
	// This function calculates an emission index
	double em_zero = v[start], max = v[start], min = v[start], reduction = 0.02 * v[start], increase = 0.02, sum = 0;
	int first_year = 5, j = 0;
	vector<double> index(0);
	
	for (int i = 1; i < SLength[0]; ++i) {
		// max: worst case scenario emissions
		max = max * (1 + increase);
		// min: best case scenario emissions
		min -= reduction;
		if (i > first_year) {
			// Find index for the emissions at year i and carry a sum
			sum += (v[start+i]-min)/(max-min);
			++j;
		}
	}
	// Find average value for the index
	return sum/j;	
}

vector<double> SumByRow(vector<double>& v, Index Idx, const int start) {
	// This function sums each row for an index across years
	int last_index = -1, j=0;
	double sum = 0;
	vector<double> result(0);
	
	for (int i = 0; i < Idx.GetSize(); ++i) {
		if ( (last_index != Idx.GetPosition(i)) && (last_index != -1) ) {
			result.push_back(sum);
			sum = 0; j=0;
			last_index = Idx.GetPosition(i);
		} else {
			if (last_index == -1)
				last_index = Idx.GetPosition(i);
			sum += v[start + i];
			++j;
		}
	}
	result.push_back(sum);
	
	return result;	
}

vector<double> AvgByRow(vector<double>& v, Index Idx, const int start) {
	// This function sums each row for an index across years
	int last_index = -1, j=0;
	double sum = 0;
	vector<double> result(0);
	
	for (int i = 0; i < Idx.GetSize(); ++i) {
		if ( (last_index != Idx.GetPosition(i)) && (last_index != -1) ) {
			result.push_back(sum/j);
			sum = 0; j=0;
			last_index = Idx.GetPosition(i);
		} else {
			if (last_index == -1)
				last_index = Idx.GetPosition(i);
			sum += v[start + i];
			++j;
		}
	}
	result.push_back(sum/j);
	
	return result;	
}
