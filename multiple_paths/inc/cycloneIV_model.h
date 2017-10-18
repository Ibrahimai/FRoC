#include <vector>
#include <map>
#include <unordered_map>
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
#define Cin (4)
#define Cout (5)
#define Combout (6)
#define FFd (7)
#define FFq (8)

#define singlePort (0)
#define simpleDualPort (1)
#define trueDualPort (2)




//#define controlSignalIdleStateReduction
#define sinksLoad
//#define DUMMYREG
#define shiftRegOrTree 

#define ControlLE

#define minSlack	-3
#define maxSlack	3
#define binCount	10

#define LUTFreq 2 // means that every other z point a lut exists 0,2,4,6,8,...
#define LUTinputSize 4
#define LocalInterconnect 38
#define LocalInterconnectThreshold 0.95

#define InputPortSize (5)
#define OutputPortSize (2)

#define FPGAsizeX (115) // start from 1 to 114 
#define FPGAsizeY (73) // starts from 1 to 72
#define FPGAsizeZ	(32) // start from 0 to 31


#define addedLatencyToBufferSinks (5)
#define addedLatencyTorelaxTimingConstraints (6)
#define orResetShift (8)


// FF modes
#define dInput 0
#define sData 1
// this class is used to store the corresponding path, node,delay for all paths using a certain RE
class RE_logic_component
{
public:
	int path;
	int node;
	double delay;
	RE_logic_component() {};
	RE_logic_component(int p, int n, double d)
	{
		path = p;
		node = n;
		delay = d;
	};
};

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

class Edge_Delay
{
public:
	int type; // 0 = FF, 1 = FR, 2 = RF, 3 = RR. For Tco of first FF add 4 to the previous values.
	double delay;
	Edge_Delay()
	{
		type = -1;
		delay = -1.0;
	}

	Edge_Delay(int ed, double del)
	{
		type = ed;
		delay = del;
	}

};

class Routing_connection
{
public:
	int destinationX; // x loc of destination
	int destinationY; // y loc of destination
	int destinationZ; // z loc of destination
	int destinationPort; // destination port
	int sourcePort; // source port
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
	}

	void clear()
	{
		usedRoutingResources.resize(0);
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
	int edgeType; // determines edge type at this node of this path where (ff->0, fr->1,rf->2,rr->3)
	int eqPath; // if redun is true, eqPath stores the most critical path that uses the same node with the same input and output
	int eqNode; // if redun is true, ewNode stores the node of the eqPath that uses this node equivalently.
	bool tested; // true when this path si tested, this is only used in the complete netlist
	Path_node() {};
	Path_node(int xx, int yy, int zz, int in, int out, bool invert, int edge) 
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
		tested = false;
		testPhase = -1; // -1
		inverting = invert;
		edgeType = edge;
	};



};


class Logic_element
{
public:
	int utilization;
	int usedInputPorts;
	int usedOutputPorts;
	int FFMode; // -1 unused FF or source only, 0 means used from D input, 1 means used from sdata (with lsload connected to 1)  input
	bool inputPorts[InputPortSize];
	bool outputPorts[OutputPortSize];
	Path_logic_component owner; // the path,node that owns this ALUT (the ALUT name usedin the output file)
	std::string LUTMask; // with this we coul elimnate the need for CoutFixedDefaultValue
	bool CoutFixedDefaultValue; // Cout deafult fixed value is 0, this variable is true when this is the case. For some cases we have to change that to allow the desired behaviour in the subsequent element. We must mark the cell accordingly.
	std::vector<Path_logic_component> nodes; // list of nodes representing which path and node use this le
	std::vector<Routing_connection> connections; // vector of the connections of all fanouts for each logic element
	std::vector<int> cascadedPaths; // each element in this vector represent a path that starts at FF x. Where the input of x is connected to the output of THIS logic element.


	Logic_element();
	Logic_element(int over);
	int get_utilization();
	void set_utilization(int x);
	void add_node(int p, int n, int in, int out);
	void remove_overlap(int p, std::vector<int> & deleted_nodes);
//	void remove_overlap_with_fanin(int p, int pIn, std::vector<int> & deleted_nodes);
};



class BRAM
{
public:
	int x; // x location 
	int y; // y location
	std::string name; // mem name, will be used to track it down in the VQM file
	int operationMode; // mode of the BRAM {dual_port, single port, true dual}
	int portADataWidth;
	int portBDataWidth;
	int portBUsedDataWidth;
	int portAAddressWidth;
	int portAUsedDataWidth;
	int portBAddressWidth;
	bool portAWE;
	bool portARE;
	bool portBWE;
	bool portBRE;
	bool clk0;
	bool ena0;
	bool clr0;
	bool portARegistered;
	bool portBRegistered;
	//std::string portAClock;
	//std::string portBClock;
	//std::string portAReadDuringWrite;
	//std::string portBReadDuringWrite;

	// the following are part of the BRAM WYSIWYG but will not be considered initially
	bool clk1;
	bool ena1;
	bool ena2;
	bool ena3;
	bool clr1;

	BRAM();
	BRAM(int xLoc, int yLoc);
	



};







//void assign_test_phases_ib();
//void unkown(std::vector < std::vector <int> > & pathRelationGraph, int i, Path_node  tempCell);
//void generate_pathRelationGraph(std::vector < std::vector <int> > & pathRelationGraph);
//bool delete_path(int path); // delete a given path by setting the first node of the path as deleted and reducing utilization of all its nodes, and adjusting portIn and poirtOut of fpgalogic according to the deleted action. The only thing left the same is fpgalogic.nodes array.
//void create_or_tree(int inputs, int LUTinputs, int number, std::ofstream& controllerFile);