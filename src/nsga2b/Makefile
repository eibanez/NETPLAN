# ---------------------------------------------------------------------
# Compiler selection 
# ---------------------------------------------------------------------
CCC = g++

#------------------------------------------------------------
# Files to compile
#------------------------------------------------------------
MAIN = nsga2
NSGA = CNSGA2.o CRand.o CQuicksort.o CLinkedList.o CFileIO.o

all: $(MAIN)

nsga2: main.cpp $(NSGA)
	$(CCC) main.cpp $(NSGA) -o nsga2
CNSGA2.o: CNSGA2.cpp CNSGA2.h
	$(CCC) -c CNSGA2.cpp
CRand.o: CRand.cpp CRand.h
	$(CCC) -c CRand.cpp
CQuicksort.o: CQuicksort.cpp CQuicksort.h
	$(CCC) -c CQuicksort.cpp
CLinkedList.o: CLinkedList.cpp CLinkedList.h
	$(CCC) -c CLinkedList.cpp
CFileIO.o: CFileIO.cpp CFileIO.h
	$(CCC) -c CFileIO.cpp

# ------------------------------------------------------------
clean :
	/bin/rm -rf *.o *~ *.class
	/bin/rm -rf $(MAIN)
	/bin/rm -rf *.dat *.log