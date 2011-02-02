// --------------------------------------------------------------
//    NETSCORE Version 1
//    metrics.h -- Definition of metricss
//    2009-2010 (c) Eduardo Ibáñez
// --------------------------------------------------------------

#ifndef _METRICS_H_
#define _METRICS_H_

double EmissionIndex(vector<double>& v, const int start);
double MinAverage(vector<double>& v, Index Idx, const int start);
vector<double> SumByRow(vector<double>& v, Index Idx, const int start);
vector<double> AvgByRow(vector<double>& v, Index Idx, const int start);

#endif  // _WRITE_H_
