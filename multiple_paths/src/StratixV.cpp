#include "StratixV.h"


ALUT::ALUT()
{
	connections.resize(0);
	actualConnections.resize(0);
	nodes.resize(0);
	utilization = 0;
	// port in initialization
	inputPorts[portA] = false;
	inputPorts[portB] = false;
	inputPorts[portC] = false;
	inputPorts[portD] = false;
	inputPorts[portE] = false;
	inputPorts[portF] = false;
	isInputPortShared[portA] = false;
	isInputPortShared[portB] = false;
	isInputPortShared[portC] = false;
	isInputPortShared[portD] = false;
	isInputPortShared[portE] = false;
	isInputPortShared[portF] = false;
	sharedWith[portA] = -1;
	sharedWith[portB] = -1;
	sharedWith[portC] = -1;
	sharedWith[portD] = -1;
	sharedWith[portE] = -1;
	sharedWith[portF] = -1;
	owner.path = -1;
	owner.node = -1;

	highSpeed = false;
	mLAB = false;

	LUTMask = "";

//	inputPorts[Cin] = false; // add when cin is considered
//	outputPorts[Cout - 5] = false;
	outputPorts[Combout] = false;
	usedInputPorts = 0;
	usedOutputPorts = 0;

	sharedInputPorts = 0;

	//CoutFixedDefaultValue = true;
}

ALUT::ALUT(int over)
{
	nodes.resize(0);
	utilization = over;
}
int ALUT::get_utilization()
{
	return utilization;
}
void ALUT::set_utilization(int x)
{
	utilization = x;
}

void ALUT::add_node(int p, int n, int in, int out, bool highS, bool MLABtype)
{

	int count = 0;
	int outPutCount = 0;
	int i;
	highSpeed = highS;
	mLAB = MLABtype;

	if (in != FFd) // check that it is not a FF
	{
		inputPorts[in] = true;
		outputPorts[out] = true;
		for (i = 0; i < InputPortSize; i++)
			if (inputPorts[i])
				count++;
		for (i = 0; i < OutputPortSize; i++)
			if (outputPorts[i])
				outPutCount++;
	
	//	if (outputPorts[0] && outputPorts[1])
	//		usedOutputPorts = 2;
	//	else
	//		usedOutputPorts = 1;
	}
	else
	{
		count = 1;
		outPutCount = 1;
	}
	usedInputPorts = count;
	usedOutputPorts = outPutCount;
	Path_logic_component temp(p, n);
	nodes.push_back(temp);
	// if this is the first node using this ALUT then it is its owner. make sure to update the delete functionality to reflect change of ownership. It could also be itenteresting to see how many times does an ALUT change ownership.
	if (utilization == 0)
		owner = temp;

	utilization++;
}

void ALUT::remove_overlap(int p, std::vector<int> & deletedPaths) // deletes all utilization except path p and returns all deleted paths into deleted_nodes
{
	int i, j;
	bool add;
	std::vector<int> indexToBeDeleted;
	for (i = 0; i < (int)nodes.size(); i++)
	{
		if (nodes[i].path != p)
		{

			add = true;
			for (j = 0; j < (int)deletedPaths.size(); j++)
			{
				if (deletedPaths[j] == nodes[i].path)
				{
					add = false;
					break;
				}

			}
			if (add)
				deletedPaths.push_back(nodes[i].path);

			if (nodes[i].path < p && add)
				std::cout << "smthng wrong" << std::endl;


			//	utilization--;
			//	indexToBeDeleted.push_back(i);
			//	nodes.erase(nodes.begin() + i);
		}
	}

	//	for (i = 0; i < indexToBeDeleted.size();i++)
	//		nodes.erase(nodes.begin() + (indexToBeDeleted[i]-i));

}

ALM::ALM()
{
	numOfInputs = 0;
//	mLAB = false;
//	highSpeed = false;
}

ALM::ALM(int x)//,  bool mLABType, bool VthType)
{
	numOfInputs = x;
//	highSpeed = VthType;
//	mLAB = mLABType;
}