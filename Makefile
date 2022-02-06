OBJ = buslock.o

ifeq ($(UNAME_S),Linux)
CXX = g++
endif

ifeq ($(UNAME_S),FreeBSD)
CXX = CC
endif

EXE = buslock
OPT = -O3
LIBS = -lpthread
CXXFLAGS = -std=c++11 -g $(OPT)
DEP = $(OBJ:.o=.d)

.PHONY: all clean

all: $(EXE)

$(EXE) : $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) $(LIBS) -o $(EXE)

%.o: %.cc
	$(CXX) -MMD $(CXXFLAGS) -c $< 

-include $(DEP)

clean:
	rm -rf $(EXE) $(OBJ) $(DEP)
