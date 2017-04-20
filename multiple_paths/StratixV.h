#pragma once
#include <vector>
#include <map>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>
#undef NDEBUG
#include <assert.h>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <math.h>  

#define portA (0)
#define portB (1)
#define portC  (2)
#define portD (3)
#define portE (4)
#define portF (5)
#define Cin (6)
#define Sharein (7)

#define Combout (0)
#define Sumout (1)
#define Cout (2)
#define Shareout (3)


#define FFd (8)
#define FFq (9)

#define locationConsraintFile "LocationFile.txt"


//#define controlSignalIdleStateReduction
#define sinksLoad
//#define DUMMYREG
#define shiftRegOrTree 

#define ControlLE

#define minSlack	-3
#define maxSlack	3
#define binCount	10

#define ALUTtoALM 6

#define LUTFreq 3 // means that every 3 z points a lut exists 0,3,6,9,12,15,...
#define LUTinputSize 6
#define ALMinputSize 8
#define LocalInterconnect 38 // not correct
#define LocalInterconnectThreshold 0.95

#define InputPortSize (6) // todo: add cin when you add arithm operations
#define OutputPortSize (1) // todo: add cout when you add arithm operations

#define FPGAsizeX (210) // start from 1 to 209 
#define FPGAsizeY (129) // starts from 1 to 128
#define FPGAsizeZ	(60) // start from 0 to 59
#define ALMsinLAB	(10) // start from 0 to 59


#define addedLatencyToBufferSinks (5)
#define addedLatencyTorelaxTimingConstraints (6)
#define orResetShift (8)

class Path_logic_component
{
public:
	int path;
	int node;
	Path_logic_component() {};
	Path_logic_component(int p, int n)
	{
		path = p;
		node = n;
	};
};

class Routing_connection
{
public:
//	int flagDelete;
	int destinationX; // x loc of destination, if this is -1 then this connection is deleted
	int destinationY; // y loc of destination
	int destinationZ; // z loc of destination
	int destinationPort; // destination port
	int sourcePort; // source port
//	bool deleted; // for the cases where we have to delete paths after reading the routing file, specifically for the port D and port E thing when 6_input LUts are required
	std::vector<std::string> usedRoutingResources; // vector of strings, each string corresponds to a routing resource used (ex: R4:x*y*...)
	Routing_connection() {};
	Routing_connection(int xx, int yy, int zz, int destPort, int sourPort)
	{
		usedRoutingResources.resize(0);
		destinationX = xx;
		destinationY = yy;
		destinationZ = zz;
		destinationPort = destPort;
		sourcePort = sourPort;
//		flagDelete = 0;
//		deleted = false;
	}

	void Routing_connection::clear()
	{
		usedRoutingResources.resize(0);
	//	deleted = false;
	//	flagDelete = 0;
		destinationX = 0;
		destinationY = 0;
		destinationZ = 0;
		destinationPort = 0;
		sourcePort = 0;
	}

};
class Path_node // which part of the FPGA does a path node use
{

public:
	int x;
	int y;
	int z;
	int portIn;
	int portOut;
	int testPhase; // which test wave is this path in 
	bool redun; // is true if there is at least one path that use the same node with same input and output
	bool deleted;
	bool inverting; // determine the behaviour of the edge across this node (ff,rr->noninverting, or fr,rf->inverting)
	int eqPath; // if redun is true, eqPath stores the most critical path that uses the same node with the same input and output
	int eqNode; // if redun is true, ewNode stores the node of the eqPath that uses this node equivalently.
	Path_node() {};
	Path_node(int xx, int yy, int zz, int in, int out, bool invert)
	{
		x = xx;
		y = yy;
		z = zz;
		portIn = in;
		portOut = out;
		redun = false;
		eqPath = 0;
		eqNode = 0;
		deleted = false;
		testPhase = -1; // -1
		inverting = invert;
	};



};

class ALUT
{
public:
	int utilization;
	int usedInputPorts;
	int usedOutputPorts;
	bool inputPorts[InputPortSize];
	bool outputPorts[OutputPortSize];
	bool highSpeed;
	bool mLAB;
	Path_logic_component owner; // the path,node that owns this ALUT (the ALUT name usedin the output file)
	int sharedInputPorts; // number of shared inputs with the other ALUT in the ALM
	bool isInputPortShared[InputPortSize];
	int sharedWith[InputPortSize];
	std::string LUTMask; // with this we coul elimnate the need for CoutFixedDefaultValue
	//bool CoutFixedDefaultValue; // Cout deafult fixed value is 0, this variable is true when this is the case. For some cases we have to change that to allow the desired behaviour in the subsequent element. We must mark the cell accordingly.
	std::vector<Path_logic_component> nodes; // list of nodes representing which path and node use this le
	std::vector<Routing_connection> connections; // vector of the connections of all fanouts for each logic element
	std::vector<Routing_connection> actualConnections; // vector of the connections of all fanouts for each logic element after quartus placed and routed the calibration file using our constraints, this is used to verify that quartus obeyed all our routing constraints
	ALUT();
	ALUT(int over);
	int get_utilization();
	void set_utilization(int x);
	void ALUT::add_node(int p, int n, int in, int out, bool highS, bool MLABtype);
	void remove_overlap(int p, std::vector<int> & deleted_nodes);
	//	void remove_overlap_with_fanin(int p, int pIn, std::vector<int> & deleted_nodes);
};

class ALM
{
public:
	int numOfInputs;

	ALM();
	ALM(int x);
};

//Logic_element fpgaLogic[FPGAsizeX][FPGAsizeY][FPGAsizeZ]; // size of cyclone IV on DE2 board, got it from chip planner, model the logic elements of the chip
														  //std::vector < std::vector <bool>> testingPhases;
//int numberOfTestPhases;
//std::vector <double> pathSlack;
//std::vector< std::vector<Path_node> > paths; // model the paths







