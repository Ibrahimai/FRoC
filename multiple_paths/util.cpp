#include "util.h"
#include "globalVar.h"

bool get_feeder_special(int x, int y, int z, int portIn, int & feederPath, int & feederNode); // returns the path and node that feeds element x,y,z through portIn, it is special as it returns a value even if the feeder cell is deleted, but it checks that it did exist before.

bool check_control_signal_required(int x, int y, int z) // checks if cell x, y, requires a control signal or not
{
	int i;
	int currentPath, currentNode;
	assert(z % LUTFreq == 0); // must be a lut
						//return true;
	for (i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++)
	{
		currentPath = fpgaLogic[x][y][z].nodes[i].path;
		currentNode = fpgaLogic[x][y][z].nodes[i].node;
		if (paths[currentPath][0].deleted)
			continue;
		if (currentNode == paths[currentPath].size() - 2) // this node feeds a register, now just ignore it, for cascaded paths this cell must be controlled todo: if this is true return true; 
			continue;
		if (fpgaLogic[paths[currentPath][currentNode + 1].x][paths[currentPath][currentNode + 1].y][paths[currentPath][currentNode + 1].z].usedInputPorts>1) // more than one port used
		{
			assert(fpgaLogic[paths[currentPath][currentNode + 1].x][paths[currentPath][currentNode + 1].y][paths[currentPath][currentNode + 1].z].usedInputPorts < LUTinputSize - 1);
			return true;
		}
	}
	return false;
}
// todo :delete this function
bool check_control_signal_required_second(int x, int y, int z) // checks if cell x, y, requires a control signal or not
{
	int i;
	int currentPath, currentNode;
	assert(z % LUTFreq == 0); // must be a lut
						//	return true;
	for (i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++)
	{
		currentPath = fpgaLogic[x][y][z].nodes[i].path;
		currentNode = fpgaLogic[x][y][z].nodes[i].node;
		if (paths[currentPath][0].deleted)
			continue;
		if (currentNode == paths[currentPath].size() - 2) // this node feeds a register, now just ignore it, for cascaded paths this cell must be controlled todo: if this is true return true; 
			continue;
		if (fpgaLogic[paths[currentPath][currentNode + 1].x][paths[currentPath][currentNode + 1].y][paths[currentPath][currentNode + 1].z].usedInputPorts>1) // more than one port used
		{
			assert(fpgaLogic[paths[currentPath][currentNode + 1].x][paths[currentPath][currentNode + 1].y][paths[currentPath][currentNode + 1].z].usedInputPorts < LUTinputSize - 1);
			return true;
		}
	}
	return false;
}


bool delete_path(int path) 
{
	if (paths[path][0].deleted) // already deleted this path
		return false;
	// mark the path as deleted
	paths[path][0].deleted = true;
	//std::cout << "deleted : " << path << std::endl;
	int i, x, y, z, j;
	bool portInStillExists = false;
	bool portOutStillExists = false;
	/// loop across all nodes of this path and adjust them accordingly
	for (i = 0; i < (int)paths[path].size(); i++)
	{
		x = paths[path][i].x;
		y = paths[path][i].y;
		z = paths[path][i].z;
		// reduce this node utilization
		fpgaLogic[x][y][z].utilization--;
		// todo: remove this from the nodes of fpgalogic[x][y][z].nodes
		portInStillExists = false;
		portOutStillExists = false;
		for (j = 0; j < (int)fpgaLogic[x][y][z].nodes.size(); j++)
		{
			if (fpgaLogic[x][y][z].nodes[j].path == path) // same path, so continue to next path
				continue;
			if (paths[fpgaLogic[x][y][z].nodes[j].path][0].deleted) // if this path is deleted then continue to next path
				continue;

			if (paths[fpgaLogic[x][y][z].nodes[j].path][fpgaLogic[x][y][z].nodes[j].node].portIn == paths[path][i].portIn) // another path is using the same element using the same portIn
				portInStillExists = true;

			if (paths[fpgaLogic[x][y][z].nodes[j].path][fpgaLogic[x][y][z].nodes[j].node].portOut == paths[path][i].portOut) // another path is using the same element using the same portOut
				portOutStillExists = true;

		}

		// change owner ship
		for (j = 0; j < (int)fpgaLogic[x][y][z].nodes.size(); j++)
		{
			if (!paths[fpgaLogic[x][y][z].nodes[j].path][0].deleted) // if this path is deleted then continue to next path
			{
				fpgaLogic[x][y][z].owner.path = fpgaLogic[x][y][z].nodes[j].path;
				fpgaLogic[x][y][z].owner.node = fpgaLogic[x][y][z].nodes[j].node;
				break;
			}
		}

		if (!portInStillExists) // input port must be deleted
		{
			if (z%LUTFreq == 0) // LUT
			{
				// portIn is removed so I have to delete routing connection from the feeder node
				int nodeFeeder, pathFeeder;
				assert(get_feeder_special(x, y, z, paths[path][i].portIn, pathFeeder, nodeFeeder)); // this function returns th feeder even if it was deleted

				int xFeeder = paths[pathFeeder][nodeFeeder].x;
				int yFeeder = paths[pathFeeder][nodeFeeder].y;
				int zFeeder = paths[pathFeeder][nodeFeeder].z;

				// utilizing the fact that when I delete a path I keep the nodes array intact
				assert(fpgaLogic[xFeeder][yFeeder][zFeeder].nodes.size() > 0);

				bool extraCheck = false;

				// loop across all connections in the feeder node to delete the one that matches the deleted input port
				for (int counter = 0; counter < (int)fpgaLogic[xFeeder][yFeeder][zFeeder].connections.size(); counter++)
				{
					if (fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationX == x && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationY == y && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationZ == z && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationPort == paths[path][i].portIn) // connection matched
					{
						assert(!extraCheck);
						//.deletedConn							fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].deleted = true;
						// trial ibrahim 23/05/2016
						fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationX = -1;
						extraCheck = true;

					}
				}

			}

			fpgaLogic[x][y][z].inputPorts[paths[path][i].portIn] = false;
			fpgaLogic[x][y][z].usedInputPorts--;
		}
			
		

		if (!portOutStillExists) //output port must be deleted
		{
			if (z%LUTFreq == 0)
				fpgaLogic[x][y][z].outputPorts[paths[path][i].portOut - 5] = false;
			fpgaLogic[x][y][z].usedOutputPorts--;
		}

	}
	return true;
}

#ifdef StratixV
bool delete_path_stratix(int path) // adjust it for owner, shared inputs, etcc..
{
	if (paths[path][0].deleted) // already deleted this path
		return false;
	// mark the path as deleted
	paths[path][0].deleted = true;
	//std::cout << "deleted : " << path << std::endl;
	if (path == 362)
		std::cout << "deubug" << std::endl;

	int i, x, y, z, j;
	bool portInStillExists = false;
	bool portOutStillExists = false;
	/// loop across all nodes of this path and adjust them accordingly
	for (i = 0; i < (int)paths[path].size(); i++)
	{
		x = paths[path][i].x;
		y = paths[path][i].y;
		z = paths[path][i].z;
		// reduce this node utilization
		fpgaLogic[x][y][z].utilization--;
		// todo: remove this from the nodes of fpgalogic[x][y][z].nodes
		portInStillExists = false;
		portOutStillExists = false;
		for (j = 0; j < (int)fpgaLogic[x][y][z].nodes.size(); j++)
		{
	
			if (fpgaLogic[x][y][z].nodes[j].path == path) // same path, so continue to next path
				continue;
			if (paths[fpgaLogic[x][y][z].nodes[j].path][0].deleted) // if this path is deleted then continue to next path
				continue;

			if (paths[fpgaLogic[x][y][z].nodes[j].path][fpgaLogic[x][y][z].nodes[j].node].portIn == paths[path][i].portIn) // another path is using the same element using the same portIn
				portInStillExists = true;

			if (paths[fpgaLogic[x][y][z].nodes[j].path][fpgaLogic[x][y][z].nodes[j].node].portOut == paths[path][i].portOut) // another path is using the same element using the same portOut
				portOutStillExists = true;

		}
		// change owner ship
		for (j = 0; j < (int)fpgaLogic[x][y][z].nodes.size(); j++)
		{
			if (!paths[fpgaLogic[x][y][z].nodes[j].path][0].deleted) // if this path is deleted then continue to next path
			{
				fpgaLogic[x][y][z].owner.path = fpgaLogic[x][y][z].nodes[j].path;
				fpgaLogic[x][y][z].owner.node = fpgaLogic[x][y][z].nodes[j].node;
				break;
			}
		}

		if (!portInStillExists) // input port must be deleted
		{
			if (z%LUTFreq == 0) // 
			{

				// portIn is removed so I have to delete routing connection from the feeder node
				int nodeFeeder, pathFeeder;
				assert(get_feeder_special(x, y, z, paths[path][i].portIn, pathFeeder, nodeFeeder)); // this function returns th feeder even if it was deleted

				int xFeeder = paths[pathFeeder][nodeFeeder].x;
				int yFeeder = paths[pathFeeder][nodeFeeder].y;
				int zFeeder = paths[pathFeeder][nodeFeeder].z;

				// utilizing the fact that when I delete a path I keep the nodes array intact
				assert(fpgaLogic[xFeeder][yFeeder][zFeeder].nodes.size() > 0);

				bool extraCheck = false;

				// loop across all connections in the feeder node to delete the one that matches the deleted input port
				for (int counter = 0; counter < (int)fpgaLogic[xFeeder][yFeeder][zFeeder].connections.size(); counter++)
				{
					if (fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationX == x && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationY == y && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationZ == z && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationPort == paths[path][i].portIn) // connection matched
					{
						assert(!extraCheck);
						//.deletedConn							fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].deleted = true;
						// trial ibrahim 23/05/2016
						fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationX = -1;
						extraCheck = true;

					}
				}

			}

				fpgaLogic[x][y][z].inputPorts[paths[path][i].portIn] = false;
		}
			
			fpgaLogic[x][y][z].usedInputPorts--;
		if (z%LUTFreq == 0)
		{
			if (fpgaLogic[x][y][z].isInputPortShared[paths[path][i].portIn]) // if this is a shared input // mark this port as not shared in the other ALUT in this ALM
			{
				assert(z%LUTFreq == 0); // this is an ALUT
				// we must also mark the shared port in the other lut as not shared any more, so we first must identify the other LUT
				int otherLUTZ;
				if (((z / LUTFreq) % 2) == 1) // then this is a bottom ALUT
				{
					otherLUTZ = z - LUTFreq;
				}
				else // this is a top ALUT
				{
					otherLUTZ = z + LUTFreq;
				}
				int otherLUTSharedPort = fpgaLogic[x][y][z].sharedWith[paths[path][i].portIn];
				assert(fpgaLogic[x][y][otherLUTZ].isInputPortShared[otherLUTSharedPort]);
				assert(fpgaLogic[x][y][otherLUTZ].sharedWith[otherLUTSharedPort] == paths[path][i].portIn);

				// adjust the other LUT
				fpgaLogic[x][y][otherLUTZ].isInputPortShared[otherLUTSharedPort] = false;
				fpgaLogic[x][y][otherLUTZ].sharedInputPorts--;
				fpgaLogic[x][y][otherLUTZ].sharedWith[otherLUTSharedPort] = -1;
				// adjust the current LUT
				fpgaLogic[x][y][z].isInputPortShared[paths[path][i].portIn] = false;
				fpgaLogic[x][y][z].sharedInputPorts--;
				fpgaLogic[x][y][z].sharedWith[paths[path][i].portIn] = -1;

					


			}
			else
			{
				alms[x][y][z / ALUTtoALM].numOfInputs--;
			}
								
		}

		if (!portOutStillExists) //output port must be deleted
		{
			if (z%LUTFreq == 0)
				fpgaLogic[x][y][z].outputPorts[paths[path][i].portOut] = false;
			fpgaLogic[x][y][z].usedOutputPorts--;
		}

		// change owner ship


	}
	return true;
}


int delete_ALUT_stratix(int x, int y, int z) // deletes all paths using ALUT with co-ordinates x, y, z
{
	bool check = false;
	int deleted_paths = 0;
	for (int i = 0; i <  (int)fpgaLogic[x][y][z].nodes.size();i++)
	{
		if (paths[fpgaLogic[x][y][z].nodes[i].path][0].deleted)
			continue;

		check = delete_path_stratix(fpgaLogic[x][y][z].nodes[i].path);
		assert(check);
		if (check)
			deleted_paths++;


	}
	return deleted_paths;
}

int delete_ALUT_port_stratix(int x, int y, int z, int port) // deletes all paths using port port in the ALUT with co-ordinates x, y, z
{
	bool check = false;
	int deleted_paths = 0;
	for (int i = 0; i <  (int)fpgaLogic[x][y][z].nodes.size(); i++)
	{
		if (paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].portIn != port) // if this node is using another input port (other than oport) then skip it
			continue;
		if (paths[fpgaLogic[x][y][z].nodes[i].path][0].deleted)
			continue;

		check = delete_path_stratix(fpgaLogic[x][y][z].nodes[i].path);
		assert(check);
		if (check)
			deleted_paths++;


	}
	return deleted_paths;
}
#endif

bool get_feeder(int x, int y, int z, int & feederPath, int & feederNode) // return path and node that feeds the register at x, y and z
{
	if (z % LUTFreq == 0) // ensure that the given cell is a register
		return false;

	if (fpgaLogic[x][y][z].nodes.size() < 1) // make sure that this element is actually used
		return false;

	if (fpgaLogic[x][y][z].nodes[0].node < 1) // make sure that this element is not a source, i.e something feeds it
		return false;

	int feederX, feederY, feederZ;

	/// gets the element that feeds the given element through the first path using this node. Even if this path is deleted the feeder x, y and z would be the same, it is imposisble for two different cells to feed the same cell
	// these assumes the absence of cascaded paths. IT assumes that any node at location x, y, z will  have the same feeder, but what if this register is a source and a sink.{casc}
	feederX = paths[fpgaLogic[x][y][z].nodes[0].path][fpgaLogic[x][y][z].nodes[0].node - 1].x;
	feederY = paths[fpgaLogic[x][y][z].nodes[0].path][fpgaLogic[x][y][z].nodes[0].node - 1].y;
	feederZ = paths[fpgaLogic[x][y][z].nodes[0].path][fpgaLogic[x][y][z].nodes[0].node - 1].z;
	feederPath = -1;
	feederNode = -1;
	int i;

	for (i = 0; i < (int)fpgaLogic[feederX][feederY][feederZ].nodes.size(); i++) // loop through paths using this feeder the first nondeletred one is the name that should be used
	{
		if (!paths[fpgaLogic[feederX][feederY][feederZ].nodes[i].path][0].deleted)
		{
			feederPath = fpgaLogic[feederX][feederY][feederZ].nodes[i].path;
			feederNode = fpgaLogic[feederX][feederY][feederZ].nodes[i].node;
			return true;
		}
	}
	return false;
}



bool check_down_link_edge_transition(int i, int j, int k) // returns true if the cell feeded by fpgaLogic[i][j][k].cout is inverting from normal inputs to cout
{

	int x;
	int newI, newJ, newK;
	bool found = false;
	for (x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++)
	{
		if ((paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node].portOut == Cout) && !paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted) // this path and node use Cout
		{
			found = true;
			newI = paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node + 1].x;
			newJ = paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node + 1].y;
			newK = paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node + 1].z;
			break;
		}
	}
	// make sure that something was using Cout of the questioned cell
	assert(found);

	for (x = 0; x < (int)fpgaLogic[newI][newJ][newK].nodes.size(); x++) // lop accross paths using the node feeded by Cout of i,j,k
	{
		if ((paths[fpgaLogic[newI][newJ][newK].nodes[x].path][fpgaLogic[newI][newJ][newK].nodes[x].node].portIn != Cin) && (!paths[fpgaLogic[newI][newJ][newK].nodes[x].path][0].deleted) && ((paths[fpgaLogic[newI][newJ][newK].nodes[x].path][fpgaLogic[newI][newJ][newK].nodes[x].node].portOut == Cout))) // this path use input other than Cin and output as Cout
		{
			return paths[fpgaLogic[newI][newJ][newK].nodes[x].path][fpgaLogic[newI][newJ][newK].nodes[x].node].inverting;
		}
	}
	// if no path from normal to cout then
	return false; // default edge trnasissiton
}


bool get_feeder(int x, int y, int z, int portIn, int & feederPath, int & feederNode) // returns the path and node that feeds element x,y,z through portIn
{
	int i;
	if ((z%LUTFreq)!=0) // ensure that the given cell is a LUT
		return false;

	if (fpgaLogic[x][y][z].nodes.size() < 1) // make sure that this element is actually used
		return false;



	if (!fpgaLogic[x][y][z].inputPorts[portIn]) // ensures that this ports is actually used
		return false;
	int possibleFeedingPath = -1;
	int possibleFeedingNode = -1;
	for (i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++) // loop through paths using the questioned element, the first non-deleted path feeding the desired input port is used to get the feeder.
	{
		if (paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].portIn == portIn && !paths[fpgaLogic[x][y][z].nodes[i].path][0].deleted)
		{
			possibleFeedingPath = fpgaLogic[x][y][z].nodes[i].path;
			possibleFeedingNode = fpgaLogic[x][y][z].nodes[i].node;
			break;
		}
	}
	if (possibleFeedingNode == -1 || possibleFeedingPath == -1)
		return false;

	int feederX, feederY, feederZ;
	feederX = paths[possibleFeedingPath][possibleFeedingNode - 1].x;
	feederY = paths[possibleFeedingPath][possibleFeedingNode - 1].y;
	feederZ = paths[possibleFeedingPath][possibleFeedingNode - 1].z;

	for (i = 0; i < (int)fpgaLogic[feederX][feederY][feederZ].nodes.size(); i++) // loop through paths using this feeder the first nondeletred one is the name that should be used
	{
		if (!paths[fpgaLogic[feederX][feederY][feederZ].nodes[i].path][0].deleted)
		{
			feederPath = fpgaLogic[feederX][feederY][feederZ].nodes[i].path;
			feederNode = fpgaLogic[feederX][feederY][feederZ].nodes[i].node;
#ifdef StratixV
			assert(feederPath == fpgaLogic[feederX][feederY][feederZ].owner.path && feederNode == fpgaLogic[feederX][feederY][feederZ].owner.node); // check that the owner is correct
#endif
			return true;
		}
	}
	return false;
}





bool get_feeder_special(int x, int y, int z, int portIn, int & feederPath, int & feederNode) // returns the path and node that feeds element x,y,z through portIn, it is special as it returns a value even if the feeder cell is deleted, but it checks that it did exist before.
{
	int i;
	if ((z%LUTFreq) != 0) // ensure that the given cell is a LUT
		return false;

	if (fpgaLogic[x][y][z].nodes.size() < 1) // make sure that this element is actually used
		return false;



	if (!fpgaLogic[x][y][z].inputPorts[portIn]) // ensures that this ports is actually used
		return false;
	int possibleFeedingPath = -1;
	int possibleFeedingNode = -1;
	for (i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++) // loop through paths using the questioned element, the first non-deleted path feeding the desired input port is used to get the feeder.
	{
		if (paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].portIn == portIn)// && !paths[fpgaLogic[x][y][z].nodes[i].path][0].deleted)
		{
			possibleFeedingPath = fpgaLogic[x][y][z].nodes[i].path;
			possibleFeedingNode = fpgaLogic[x][y][z].nodes[i].node;
			break;
		}
	}
	if (possibleFeedingNode == -1 || possibleFeedingPath == -1)
		return false;

	int feederX, feederY, feederZ;
	feederX = paths[possibleFeedingPath][possibleFeedingNode - 1].x;
	feederY = paths[possibleFeedingPath][possibleFeedingNode - 1].y;
	feederZ = paths[possibleFeedingPath][possibleFeedingNode - 1].z;

	for (i = 0; i < (int)fpgaLogic[feederX][feederY][feederZ].nodes.size(); i++) // loop through paths using this feeder the first nondeletred one is the name that should be used
	{
//		if (!paths[fpgaLogic[feederX][feederY][feederZ].nodes[i].path][0].deleted)
//		{
			feederPath = fpgaLogic[feederX][feederY][feederZ].nodes[i].path;
			feederNode = fpgaLogic[feederX][feederY][feederZ].nodes[i].node;
//#ifdef StratixV
//			assert(feederPath == fpgaLogic[feederX][feederY][feederZ].owner.path && feederNode == fpgaLogic[feederX][feederY][feederZ].owner.node); // check that the owner is correct
//#endif
			return true;
//		}
	}
	return false; // this is what makes it special
}