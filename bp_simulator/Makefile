CC = g++
OPT = -O3
WARN = -Wall
CFLAGS = $(OPT) $(WARN) $(INC) $(LIB)

# List all your .cc files here (source files, excluding header files)
SIM_SRC = branch_predictor.cpp

# List corresponding compiled object files here (.o files)
SIM_OBJ = branch_predictor.o
 
#################################

# default rule

all: bpsim
	@echo "Done with make..."


# rule for making bpsim

bpsim: $(SIM_OBJ)
	$(CC) -o bpsim $(CFLAGS) $(SIM_OBJ) -lm
	@echo "-----------DONE WITH BPSIM-----------"


# generic rule for converting any .cc file to any .o file
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc


# type "make clean" to remove all .o files plus the bpsim binary

clean:
	rm -f *.o bpsim


# type "make clobber" to remove all .o files (leaves bpsim binary)

clobber:
	rm -f *.o


