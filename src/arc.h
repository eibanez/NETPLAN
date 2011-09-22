// --------------------------------------------------------------
//    NETSCORE Version 2
//    arc.h -- Definition of node functions
//    2009-2011 (c) Eduardo Ibanez
// --------------------------------------------------------------

#ifndef _ARC_H_
#define _ARC_H_

// Declare class to store arc information
struct Arc {
	Arc();
	Arc(const Arc& rhs);
	Arc(const Arc& rhs, const bool reverse);
	~Arc();
	Arc& operator=(const Arc& rhs);
	
	string Get(const string& selector) const;
	string GetYear() const;
	double GetDouble(const string& selector) const;
	void Set(const string& selector, const string& input);
	void Set(const string& selector, const bool input);
	void Add(const string& selector, const string& input);
	void Multiply(const string& selector, const double value);
	int Time() const;
	
	string WriteEnergy2Trans() const;
	string WriteTrans2Energy() const;
	
	string ArcUbNames() const;
	string ArcCapNames() const;
	string ArcDcNames() const;
	string ArcColumns() const;
	string InvArcColumns() const;
	string CapArcColumns(int selector) const;
	vector<string> Events() const;
	string ArcRhs() const;
	string ArcBounds() const;
	string ArcInvBounds() const;
	
	bool isFirstinYear() const;
	bool InvArc() const;
	bool InvertEff() const;
	bool isDCflow() const;
	bool isStorage() const;
	bool isBidirect() const;
	bool isFirstBidirect() const;
	bool isTransport() const;
	bool isFirstTransport() const;
	
	// Values
	vector<string> Properties, Trans2Energy;
	bool Energy2Trans;
};

// Find the index for a arc property selector
int FindArcSelector(const string& selector);

#endif  // _NODE_H_
