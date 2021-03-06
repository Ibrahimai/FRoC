#include "cycloneIV_model.h"

Logic_element::Logic_element()
{
	connections.resize(0);
	nodes.resize(0);
	cascadedPaths.resize(0);
	utilization = 0;
	FFMode = -1;
	inputPorts[portA] = false;
	inputPorts[portB] = false;
	inputPorts[portC] = false;
	inputPorts[portD] = false;
	inputPorts[Cin] = false;
	outputPorts[Cout - 5] = false;
	outputPorts[Combout - 5] = false;
	owner.path = -1;
	owner.node = -1;
	usedInputPorts = 0;
	usedOutputPorts = 0;
	CoutFixedDefaultValue = true;

	isBRAM = false;
	indexMemories.resize(0);
	BRAMinputPorts.resize(0);
	BRAMoutputPorts.resize(0);
	portAInputCount = 0;
	portBInputCount = 0;
	BRAMInputPortsUsed = 0;
	BRAMOutputPortsUsed = 0;
	countNumofMem = 0;
}

Logic_element::Logic_element(int over)
{
	nodes.resize(0);
	utilization = over;
}
int Logic_element::get_utilization()
{
	return utilization;
}
void Logic_element::set_utilization(int x)
{
	utilization = x;
}

void Logic_element::add_node(int p, int n, int in, int out)
{

	int count = 0;
	int i;

	if (in < 5) // if input higher than 4 then this is a FF
	{
		inputPorts[in] = true;
		outputPorts[out - 5] = true;
		for (i = 0; i < 5; i++)
			if (inputPorts[i])
				count++;

		if (outputPorts[0] && outputPorts[1])
			usedOutputPorts = 2;
		else
			usedOutputPorts = 1;
	}
	else
	{
		count = 1;
		usedOutputPorts = 1;
	}
	usedInputPorts = count;
	Path_logic_component temp(p, n);
	nodes.push_back(temp);

	// if this is the first node using this ALUT then it is its owner. make sure to update the delete functionality to reflect change of ownership. It could also be itenteresting to see how many times does an ALUT change ownership.
	if (utilization == 0)
		owner = temp;
	utilization++;
}



void Logic_element::add_node(int p, int n, std::pair <int, int> BRAMportInputInfo, std::pair <int, int> BRAMportOuputInfo)
{

	int count = 0;
	int i,j;

	int inputPortCount = 0;
	int outputPortCount = 0;
	int portAcount = 0;
	int portBcount = 0;
	if (BRAMportInputInfo.first >= 0)
	{
		BRAMinputPorts[BRAMportInputInfo.first][BRAMportInputInfo.second] = true;
		for (i = 0; i < BRAMinputPorts.size(); i++)
		{
			int oldCount = count;
			for (j = 0; j < BRAMinputPorts[i].size(); j++)
			{
				if (BRAMinputPorts[i][j])
				{
					count++;
					if (i <= BRAMportAWE) // then it's port A related
					{
						//portAInputCount++;
						portAcount++;
					}
					else
					{
						//portBInputCount++;
						portBcount++;
					}
				}
			}

			if (oldCount != count)
			{
				// port i is used so we will increment the number of used input ports
				assert(count > oldCount);
				inputPortCount++;
			}
		}

		usedInputPorts = count;
		BRAMInputPortsUsed = inputPortCount;
		portAInputCount = portAcount;
		portBInputCount = portBcount;
	}

	if (BRAMportOuputInfo.first >= 0)
	{
		BRAMoutputPorts[BRAMportOuputInfo.first][BRAMportOuputInfo.second] = true;
		for (i = 0; i < BRAMoutputPorts.size(); i++)
		{
			int oldCount = count;

			for (j = 0; j < BRAMoutputPorts[i].size(); j++)
				if (BRAMoutputPorts[i][j])
					count++;

			if (oldCount != count)
			{
				// port i is used so we will increment the number of used input ports
				assert(count > oldCount);
				outputPortCount++;
			}

		}
		usedOutputPorts = count;
		BRAMOutputPortsUsed = outputPortCount;
	}

	Path_logic_component temp(p, n);
	nodes.push_back(temp);

	// if this is the first node using this ALUT then it is its owner. make sure to update the delete functionality to reflect change of ownership. It could also be itenteresting to see how many times does an ALUT change ownership.
	if (utilization == 0)
		owner = temp;
	utilization++;
}

void Logic_element::remove_overlap(int p, std::vector<int> & deletedPaths) // deletes all utilization except path p and returns all deleted paths into deleted_nodes
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


BRAM::BRAM()
{

	x = -1; // x location 
	y = -1; // y location
	operationMode = -1; // mode of the BRAM {dual_port, single port, true dual}
	portADataWidth = -1;
	portAUsedDataWidth = -1; // the number of used bits from the width of A
	portBUsedDataWidth = -1; // the number of used bits from the width of A
	portBDataWidth = -1;
	portAAddressWidth = -1;
	portBAddressWidth = -1;
	portAWE = false;
	portARE = false;
	portBWE = false;
	portBRE = false;
	clk0 = false;
	ena0 = false;
	clr0 = false;
	portBRegistered = false;
	portARegistered = false;
	//portAClock = "";
	//portBClock = "";
	//portAReadDuringWrite = "";
	//portBReadDuringWrite = "";
	// the following are part of the BRAM WYSIWYG but will not be considered initially
	clk1 = false;
	ena1 = false;
	ena2 = false;
	ena3 = false;
	clr1 = false;

}


BRAM::BRAM(int xLoc, int yLoc)
{

	x = xLoc; // x location 
	y = yLoc; // y location
	operationMode = -1; // mode of the BRAM {dual_port, single port, true dual}
	portADataWidth = -1;
	portBDataWidth = -1;
	portAAddressWidth = -1;
	portBAddressWidth = -1;
	portAWE = false;
	portARE = false;
	portBWE = false;
	portBRE = false;
	clk0 = false;
	ena0 = false;
	clr0 = false;
	// the following are part of the BRAM WYSIWYG but will not be considered initially
	clk1 = false;
	ena1 = false;
	ena2 = false;
	ena3 = false;
	clr1 = false;

}

BRAMController::BRAMController()
{
	BRAMinputPorts.resize(0);
	BRAMoutputPorts.resize(0);
}