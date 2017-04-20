
INC      = -I/home/ibrahim/Downloads/gurobi702/linux64/include/ -I../inc
CPPLIB   = -L/home/ibrahim/Downloads/gurobi702/linux64/lib/ -lgurobi_c++ -lgurobi70
CPP      = g++
CARGS    =  -std=c++11 -O2
SRCDIR	 = ../src
SOURCES  = $(wildcard $(SRCDIR)/*.cpp)
OBJDIR = ./
OBJECTS	 = $(SOURCES:.cpp=.o)
EXECUTABLE = main
HDRDIR = ../inc
HDR = $(wildcard $(HDRDIR)/*.h)
CFLAGS=-c -Wall -std=c++11 -O2


all:  $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CPP) $(CARGS) -o $(EXECUTABLE) $(OBJECTS) $(INC) $(CPPLIB) -lm


%.o: %.cpp $(HDR)
	$(CPP) $(CFLAGS) $(INC) $< -o $@


clean:
	rm $(EXECUTABLE) $(SRCDIR)/*.o

