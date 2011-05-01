// --------------------------------------------------------------
//    NETSCORE Version 2
//    step.h -- Definition of step functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#ifndef _STEP_H_
#define _STEP_H_

typedef vector<int> Step;

// Converts a string like 'y1m2' into the appropriate 'Step' (vector of integers)
Step Str2Step(const string& mystep);

// Converts the appropriate 'Step' (vector of integers) into its corresponding string (such as 'y1m2')
string Step2Str(const Step& mystep);

// Given a 'Step', finds the next one
Step NextStep(const Step& mystep);

// Sum two 'Step' variables
Step StepSum(const Step& a, const Step& b);

// Given a 'Step', it determines the column position (for reading properties).
// It goes like this: 'const' 'y1' 'y1m1' 'y1m1h1' 'y1m1h2' ... 'y1m2' etc.
int Step2Pos(const Step& mystep);

// Given a 'Step', it determines the column position (for writing output).
// It goes like this: 'const' 'y1' 'y2' ... 'y1m1' 'y1m2' ... 'y1m1h1' 'y1m1h2' etc.
int Step2Col(const Step& mystep);

// Given a 'Step', find its length in hours, which is stored in the global variable StepHours
string Step2Hours(const Step& mystep);

#endif  // _STEP_H_
