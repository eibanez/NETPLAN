// --------------------------------------------------------------
//    NETSCORE Version 1
//    node.h -- Definition of node functions
//    2009-2010 (c) Eduardo Ibáñez
// --------------------------------------------------------------

#ifndef _NODE_H_
#define _NODE_H_

// Declare class type to hold node information
class Node {
    public:
        Node();
        Node(const Node& rhs);
        ~Node();
        Node& operator=(const Node& rhs);
		
        string Get(const string& selector) const;
        double GetDouble(const string& selector) const;
        vector<string> GetVecStr() const;
        void Set(const string& selector, const string& input);
		void Multiply(const string& selector, const double value);
		int Time() const;

        string NodeNames() const;
        string NodeColumns() const;
		string NodePeakRows() const;
        string NodeRMColumns() const;
        string NodeRMBounds() const;
        string NodeRhs() const;
        string DCNodesBounds() const;
		
		bool isDCelect() const;
		bool isDCflow() const;
		
    private:
        vector<string> Properties;
};

// Find the index for a node property selector
int FindNodeSelector(const string& selector);

#endif  // _NODE_H_
