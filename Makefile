# ---------------------------------------------------------------------
# CPLEX options 
# ---------------------------------------------------------------------
CPLEXDIR      = /usr/local/cplex/ILOG/CPLEX_Studio_AcademicResearch122/cplex
CONCERTDIR    = /usr/local/cplex/ILOG/CPLEX_Studio_AcademicResearch122/concert
SYSTEM = x86-64_sles10_4.1
LIBFORMAT = static_pic

# ---------------------------------------------------------------------
# Compiler options 
# ---------------------------------------------------------------------
CCOPT = -m64 -O -fPIC -fexceptions -DNDEBUG -DIL_STD

# ---------------------------------------------------------------------
# Link options and libraries
# ---------------------------------------------------------------------
CPLEXLIBDIR   = $(CPLEXDIR)/lib/$(SYSTEM)/$(LIBFORMAT)
CONCERTLIBDIR = $(CONCERTDIR)/lib/$(SYSTEM)/$(LIBFORMAT)

CONCERTINCDIR = $(CONCERTDIR)/include
CPLEXINCDIR   = $(CPLEXDIR)/include

CCLNFLAGS = -L$(CPLEXLIBDIR) -lilocplex -lcplex -L$(CONCERTLIBDIR) -lconcert -lm -pthread
CCFLAGS = $(CCOPT) -I$(CPLEXINCDIR) -I$(CONCERTINCDIR)

# ---------------------------------------------------------------------
# NETPLAN folders
# ---------------------------------------------------------------------
SRCDIR        = src
NGSADIR       = src/nsga2

# ---------------------------------------------------------------------
# Files to compile
# ---------------------------------------------------------------------
MAIN = prep post nsga2 nsga2b postnsga
SUB = step.o global.o node.o arc.o read.o write.o index.o
SOLVER = solver.o
NSGA = CNSGA2.o CRand.o CQuicksort.o CLinkedList.o CFileIO.o

all: $(MAIN)

prep: $(SRCDIR)/preprocess.cpp $(SRCDIR)/netscore.h $(SUB)
	g++ $(SRCDIR)/preprocess.cpp $(SUB) -o prep
node.o: $(SRCDIR)/node.cpp $(SRCDIR)/node.h
	g++ -c $(SRCDIR)/node.cpp
arc.o: $(SRCDIR)/arc.cpp $(SRCDIR)/arc.h
	g++ -c $(SRCDIR)/arc.cpp
global.o: $(SRCDIR)/global.cpp $(SRCDIR)/global.h
	g++ -c $(SRCDIR)/global.cpp
step.o: $(SRCDIR)/step.cpp $(SRCDIR)/step.h
	g++ -c $(SRCDIR)/step.cpp
read.o: $(SRCDIR)/read.cpp $(SRCDIR)/read.h
	g++ -c $(SRCDIR)/read.cpp
write.o: $(SRCDIR)/write.cpp $(SRCDIR)/write.h
	g++ -c $(SRCDIR)/write.cpp
index.o: $(SRCDIR)/index.cpp $(SRCDIR)/index.h
	g++ -c $(SRCDIR)/index.cpp

solver.o: $(SRCDIR)/solver.cpp $(SRCDIR)/solver.h
	g++ -c $(CCFLAGS) $(SRCDIR)/solver.cpp

post: post.o $(SUB) $(SOLVER)
	g++ $(CCFLAGS) post.o $(SOLVER) $(SUB) -o post $(CCLNFLAGS)
post.o: $(SRCDIR)/postprocess.cpp 
	g++ -c $(CCFLAGS) $(SRCDIR)/postprocess.cpp -o post.o

postnsga: postnsga.o $(SUB) $(SOLVER)
	g++ $(CCFLAGS) postnsga.o $(SOLVER) $(SUB) -o postnsga $(CCLNFLAGS)
postnsga.o: $(SRCDIR)/postnsga.cpp 
	g++ -c $(CCFLAGS) $(SRCDIR)/postnsga.cpp -o postnsga.o

nsga2: $(NGSADIR)/main.cpp $(NSGA) $(SUB) $(SOLVER)
	g++ $(CCFLAGS) $(NGSADIR)/main.cpp $(NSGA) $(SOLVER) $(SUB) -o nsga2 $(CCLNFLAGS)
nsga2b: $(NGSADIR)/main-seq.cpp $(NSGA) $(SUB) $(SOLVER)
	g++ $(CCFLAGS) $(NGSADIR)/main-seq.cpp $(NSGA) $(SOLVER) $(SUB) -o nsga2b $(CCLNFLAGS)
CNSGA2.o: $(NGSADIR)/CNSGA2.cpp $(NGSADIR)/CNSGA2.h
	g++ -c $(CCFLAGS) $(NGSADIR)/CNSGA2.cpp -o CNSGA2.o
CRand.o: $(NGSADIR)/CRand.cpp $(NGSADIR)/CRand.h
	g++ -c $(NGSADIR)/CRand.cpp
CQuicksort.o: $(NGSADIR)/CQuicksort.cpp $(NGSADIR)/CQuicksort.h
	g++ -c $(NGSADIR)/CQuicksort.cpp
CLinkedList.o: $(NGSADIR)/CLinkedList.cpp $(NGSADIR)/CLinkedList.h
	g++ -c $(NGSADIR)/CLinkedList.cpp
CFileIO.o: $(NGSADIR)/CFileIO.cpp $(NGSADIR)/CFileIO.h
	g++ -c $(CCFLAGS) $(NGSADIR)/CFileIO.cpp

# ------------------------------------------------------------
clean :
	/bin/rm -rf *.o *~ *.class
	/bin/rm -rf $(MAIN)
	/bin/rm -rf *.dat *.log
	/bin/rm -f $(OBJS)

cleanmps :
	/bin/rm -rf *.mps *.ord *.sos *.lp *.sav *.net *.msg *.log *.clp *.in
	/bin/rm -f prepdata/*.*