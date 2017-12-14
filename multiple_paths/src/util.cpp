#include "util.h"
#include "globalVar.h"

bool get_feeder_special(int x, int y, int z, int portIn, int & feederPath, int & feederNode); // returns the path and node that feeds element x,y,z through portIn, it is special as it returns a value even if the feeder cell is deleted, but it checks that it did exist before.

// we check for wraparound apths[old] currently the function assumes there is no wraparound paths. To make it formally correct, we need to check that the source and sink found are from different paths. Otherwise, we could falsely identify a wraparound path as cascaded paths.
bool is_cascaded_reg(int x, int y, int z) // returns true if the register in loc (x,y,z) is a cascaded register. Meaning that it is a sink and a source at the same time
{
	assert(z % LUTFreq != 0); // this is a register

	int l = 0;
	bool source = false;
	bool sink = false;
	std::vector<int> sources;
	std::vector<int> sinks;
	sources.resize(0);
	sinks.resize(0);
	for (l = 0; l < (int)fpgaLogic[x][y][z].nodes.size(); l++)
	{
		if (paths[fpgaLogic[x][y][z].nodes[l].path][0].deleted) // if this path is deleted, then we shouldnt be accounting for it
			continue;
		if (fpgaLogic[x][y][z].nodes[l].node == 0)
		{
			source = true;
			sources.push_back(fpgaLogic[x][y][z].nodes[l].path);
		}
		else
		{
			sink = true;

			sinks.push_back(fpgaLogic[x][y][z].nodes[l].path);
			assert(fpgaLogic[x][y][z].nodes[l].node == (int)paths[fpgaLogic[x][y][z].nodes[l].path].size() - 1); // make sure that this is really the sink of the corresponding path.
		}
	//	if (source && sink)
	//	{
		//	return true;
//		}

	}

	if (sink&&source) // this register is a sink and source, just gonna check if they are all cascaded or not (some may be wraparound/feedback)
	{
		for (int i = 0; i < (int)sinks.size(); i++)
		{
			for (int j = 0; j < (int)sources.size(); j++)
			{
				if (sinks[i] == sources[j]) // then this is a cascaded path so remove it from both
				{
					sinks.erase(sinks.begin() + i); // delete the ith element, if i is zero element @ index 0 is deleted
					sources.erase(sources.begin() + j); // delete the jth element
					i--;
					break;
				}
			}
		}

		if (sinks.size() > 0 && sources.size()>0) // if after deleting allc ascaded paths we still have items in the sources and sinks list then baaaam thats cascaded register
		{
			return true;
		}
		else
		{
			/*	if (x == 65 && y == 21 && z == 29)
			{
				std::cout << "sinks size " << sinks.size() << std::endl;
				std::cout << "sources size " << sources.size() << std::endl;
			}*/
			return false;
		}
	}
	else
	{
		/* if (x == 65 && y == 21 && z == 29)
		{
			std::cout << "sinks size " << sinks.size() << std::endl;
			std::cout << "sources size " << sources.size() << std::endl;
		} */
		return false;
	}
	
}

bool reg_free_input(int x, int y, int z) // returns true if the register in loc (x,y,z) has its input free, only a source
{
	assert(z % LUTFreq != 0); // this is a register

	int l = 0;
	bool source = false;
	bool sink = false;
	//std::vector<int> sources;
	//std::vector<int> sinks;
	//sources.resize(0);
	//sinks.resize(0);
	for (l = 0; l < (int)fpgaLogic[x][y][z].nodes.size(); l++)
	{
		if (paths[fpgaLogic[x][y][z].nodes[l].path][0].deleted) // if this path is deleted, then we shouldnt be accounting for it
			continue;
		if (fpgaLogic[x][y][z].nodes[l].node == 0)
		{
			source = true;
		//	sources.push_back(fpgaLogic[x][y][z].nodes[l].path);
		}
		else
		{
			sink = true;

		//	sinks.push_back(fpgaLogic[x][y][z].nodes[l].path);
			assert(fpgaLogic[x][y][z].nodes[l].node == (int)paths[fpgaLogic[x][y][z].nodes[l].path].size() - 1); // make sure that this is really the sink of the corresponding path.
		}
		//	if (source && sink)
		//	{
		//	return true;
		//		}

	}

	if (sink&&source) // this register is a sink and source, so cannot control its input. Thus input is not free
	{
		return false;
	}
	else
	{
		return true;
	}

}

bool check_control_signal_required(int x, int y, int z) // checks if cell x, y, z  requires a control signal or not
{
	int i;
	int currentPath, currentNode, nextNode;
	assert(z % LUTFreq == 0); // must be a lut
						//return true;
	for (i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++)
	{
		currentPath = fpgaLogic[x][y][z].nodes[i].path;
		currentNode = fpgaLogic[x][y][z].nodes[i].node;
		nextNode = currentNode + 1;
		if (paths[currentPath][0].deleted)
			continue;
		if (currentNode == (int)paths[currentPath].size() - 2) // this node is the b4 last cell so it feeds a register, now just ignore it, for cascaded paths this cell must be controlled todo: if this is true return true; 
		{
			// this means that this node is the cell before the last, so it feeds a aregister, we must check if this register is a cascaded register then we must control it.
			
			// check if this node feeds a cascaded register
			if (is_cascaded_reg(paths[currentPath][nextNode].x, paths[currentPath][nextNode].y, paths[currentPath][nextNode].z)) // if it feeds a cascaded register, then return true as we do need the control signal.
			{
				assert(fpgaLogic[x][y][z].cascadedPaths.size()>0);
				return true;
			}
		}
		if (fpgaLogic[paths[currentPath][currentNode + 1].x][paths[currentPath][currentNode + 1].y][paths[currentPath][currentNode + 1].z].usedInputPorts>1) // more than one port used
		{
			assert(fpgaLogic[paths[currentPath][currentNode + 1].x][paths[currentPath][currentNode + 1].y][paths[currentPath][currentNode + 1].z].usedInputPorts <= LUTinputSize ); /*I now call it before deleting any paths so it may just hapen that a LUT is using all of its inputs, added equal sign to allow Luts to use only one control signal "fix"*/
			return true;
		}
	}
	return false;
}
// todo :delete this function
bool check_control_signal_required_second(int x, int y, int z) // checks if cell x, y, requires a control signal or not
{
	/*int i;
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
			assert(fpgaLogic[paths[currentPath][currentNode + 1].x][paths[currentPath][currentNode + 1].y][paths[currentPath][currentNode + 1].z].usedInputPorts <= LUTinputSize - 1); //dded equal sign to allow Luts to use only one control signal "fix"/

			return true;
		}
	}
	return false; */
	return check_control_signal_required(x, y, z); // ib 12/14/2016


	//return false;
}


bool delete_path(int path) 
{
	if (paths[path][0].deleted) // already deleted this path
		return false;

//	if (paths[path][0].x == 58 && paths[path][0].y == 21 && paths[path][0].z == 17)
//		std::cout << "debuging 21-10-2016" << std::endl;

	// see if this is a cascaded path (it source reg is actually a sink register for something else)
	bool isCascadedReg = is_cascaded_reg(paths[path][0].x, paths[path][0].y, paths[path][0].z);

	// used to handle updating the cascaded list when the deleted path is a cascaded path
	int cascadedFeederPath, cascadedFeederNode;
	int cascadedFeederX = -1;
	int cascadedFeederY = -1; 
	int cascadedFeederZ = -1;


	if (isCascadedReg)
	{
		assert(get_feeder(paths[path][0].x, paths[path][0].y, paths[path][0].z, cascadedFeederPath, cascadedFeederNode));
		cascadedFeederX = paths[cascadedFeederPath][cascadedFeederNode].x;
		cascadedFeederY = paths[cascadedFeederPath][cascadedFeederNode].y;
		cascadedFeederZ = paths[cascadedFeederPath][cascadedFeederNode].z;

		assert(fpgaLogic[cascadedFeederX][cascadedFeederY][cascadedFeederZ].cascadedPaths.size() > 0); // since it is cascaded it must have a cascaded list
	}

	// mark the path as deleted
	paths[path][0].deleted = true;
	//std::cout << "deleted : " << path << std::endl;
	int i, x, y, z, j;
	bool portInStillExists = false;
	bool portOutStillExists = false;
	bool isStillSink = false;
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
		isStillSink = false;

		for (j = 0; j < (int)fpgaLogic[x][y][z].nodes.size(); j++)
		{
			if (fpgaLogic[x][y][z].nodes[j].path == path) // same path, so continue to next path
				continue;
			if (paths[fpgaLogic[x][y][z].nodes[j].path][0].deleted) // if this path is deleted then continue to next path
				continue;

			if (i == (int)paths[path].size() - 1) // last element in the path which is the sink reg
			{
				assert(z%LUTFreq != 0); // make sure its a reg
				if (fpgaLogic[x][y][z].nodes[j].node>0) // check if node j at reg x, y, z is a sink register. If thats the case then this register is still a sink
					isStillSink = true;
			}

			if (paths[fpgaLogic[x][y][z].nodes[j].path][fpgaLogic[x][y][z].nodes[j].node].portIn == paths[path][i].portIn) // another path is using the same element using the same portIn
				portInStillExists = true;

			if (paths[fpgaLogic[x][y][z].nodes[j].path][fpgaLogic[x][y][z].nodes[j].node].portOut == paths[path][i].portOut) // another path is using the same element using the same portOut
				portOutStillExists = true;

		}

		// change owner ship
		bool isUsed = false;
		for (j = 0; j < (int)fpgaLogic[x][y][z].nodes.size(); j++)
		{
			if (!paths[fpgaLogic[x][y][z].nodes[j].path][0].deleted) // if this path is deleted then continue to next path
			{
				fpgaLogic[x][y][z].owner.path = fpgaLogic[x][y][z].nodes[j].path;
				fpgaLogic[x][y][z].owner.node = fpgaLogic[x][y][z].nodes[j].node;
				isUsed = true;
				break;
			}
		}
		if (!isUsed) // this is not used
		{
			fpgaLogic[x][y][z].owner.path = -1;
			fpgaLogic[x][y][z].owner.node = -1;

		}
		if (!portInStillExists || (!isStillSink&&(z%LUTFreq!=0))) // input port must be deleted or the sink register is no longer there
		{
			if (z%LUTFreq == 0) // LUT
			{
				assert(!portInStillExists); // ensure that his part is only executed for LUTs when !portInstillexists is true
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
				fpgaLogic[x][y][z].inputPorts[paths[path][i].portIn] = false;
				fpgaLogic[x][y][z].usedInputPorts--;

			}
			else if (i>0) // FF and sink of path path
			{
				int nodeFeeder, pathFeeder;
				assert(get_feeder_special(x, y, z, pathFeeder, nodeFeeder)); // this function returns th feeder even if it was deleted
				//get_feeder_special(int x, int y, int z, int & feederPath, int & feederNode)

				int xFeeder = paths[pathFeeder][nodeFeeder].x;
				int yFeeder = paths[pathFeeder][nodeFeeder].y;
				int zFeeder = paths[pathFeeder][nodeFeeder].z;

				// utilizing the fact that when I delete a path I keep the nodes array intact
				assert(fpgaLogic[xFeeder][yFeeder][zFeeder].nodes.size() > 0);

				bool extraCheck = false;

				// loop across all connections in the feeder node to delete the one that matches the deleted input port
				for (int counter = 0; counter < (int)fpgaLogic[xFeeder][yFeeder][zFeeder].connections.size(); counter++)
				{
					if (fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationX == x && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationY == y && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationZ == z) // connection matched
					{
						assert(!extraCheck);
						//.deletedConn							fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].deleted = true;
						// trial ibrahim 23/05/2016
						fpgaLogic[xFeeder][yFeeder][zFeeder].connections[counter].destinationX = -1;
						extraCheck = true;

						// since we have deleted a connection to a register we must update the cascaded list of xfeeder,yfeeder zfeeder by removing all paths starting at reg x, y, z
						for (int pop = 0; pop < (int)fpgaLogic[xFeeder][yFeeder][zFeeder].cascadedPaths.size(); pop++)
						{
							// check if this cascaded path start at reg x, y,z. If so, then delete it from the cascaded list
							if (paths[fpgaLogic[xFeeder][yFeeder][zFeeder].cascadedPaths[pop]][0].x == x && paths[fpgaLogic[xFeeder][yFeeder][zFeeder].cascadedPaths[pop]][0].y == y && paths[fpgaLogic[xFeeder][yFeeder][zFeeder].cascadedPaths[pop]][0].z == z)
							{
								fpgaLogic[xFeeder][yFeeder][zFeeder].cascadedPaths.erase(fpgaLogic[xFeeder][yFeeder][zFeeder].cascadedPaths.begin() + pop);
								pop--;
							}
						}
					}
				}
				fpgaLogic[x][y][z].inputPorts[paths[path][i].portIn] = false;
				fpgaLogic[x][y][z].usedInputPorts--;

			}

		//	fpgaLogic[x][y][z].inputPorts[paths[path][i].portIn] = false;
		//	fpgaLogic[x][y][z].usedInputPorts--;
		}
			
		

		if (!portOutStillExists) //output port must be deleted
		{
			if (z%LUTFreq == 0)
				fpgaLogic[x][y][z].outputPorts[paths[path][i].portOut - 5] = false;
			fpgaLogic[x][y][z].usedOutputPorts--;
		}

	}

	// delete this path from cascaded list if it is a cascaded path
	if (isCascadedReg)
	{
		//sert(get_feeder(paths[path][0].x, paths[path][0].y, paths[path][0].z, cascadedFeederPath, cascadedFeederNode));


		// get the cascaded LUT that feeds the csacaded regsiter
	//	int cascadedFeederX = paths[cascadedFeederPath][cascadedFeederNode].x;
	//	int cascadedFeederY = paths[cascadedFeederPath][cascadedFeederNode].y;
	//	int cascadedFeederZ = paths[cascadedFeederPath][cascadedFeederNode].z;

	//	assert(fpgaLogic[cascadedFeederX][cascadedFeederY][cascadedFeederZ].cascadedPaths.size() > 0); // since it is cascaded

		for (i = 0; i < (int)fpgaLogic[cascadedFeederX][cascadedFeederY][cascadedFeederZ].cascadedPaths.size(); i++)
		{

			if (fpgaLogic[cascadedFeederX][cascadedFeederY][cascadedFeederZ].cascadedPaths[i] == path)
			{
				fpgaLogic[cascadedFeederX][cascadedFeederY][cascadedFeederZ].cascadedPaths.erase(fpgaLogic[cascadedFeederX][cascadedFeederY][cascadedFeederZ].cascadedPaths.begin() + i);
				i--;
			}
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

// return path and node that feeds the register at x, y and z
bool get_feeder(int x, int y, int z, int & feederPath, int & feederNode) 
{
	// ensure that the given cell is a register
	if (z % LUTFreq == 0 && !fpgaLogic[x][y][z].isBRAM) 
		return false;

	if (fpgaLogic[x][y][z].nodes.size() < 1) // make sure that this element is actually used
		return false;

//	if (fpgaLogic[x][y][z].nodes[0].node < 1) // make sure that this element is not a source, i.e something feeds it
//		return false;

	int i, nodeIndex;
	nodeIndex = -1;
	for (i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++)
	{
		if (fpgaLogic[x][y][z].nodes[i].node>0) // this is not a source node
		{
			nodeIndex = i;
			break;
		}
	}

	if (nodeIndex == -1) // this FF is not a sink, so it has no feeder. return false
		return false;

	int feederX, feederY, feederZ;

	// gets the element that feeds the given element through the first path using this node.
	// Even if this path is deleted the feeder x, y and z would be the same, 
	// it is imposisble for two different cells to feed the same register
	// [corected now it works with no assumptions]  (old note) --> these assumes the absence of cascaded paths. IT assumes that any node at location x, y, z will  have the same feeder, but what if this register is a source and a sink.{casc}
	feederX = paths[fpgaLogic[x][y][z].nodes[nodeIndex].path][fpgaLogic[x][y][z].nodes[nodeIndex].node - 1].x;
	feederY = paths[fpgaLogic[x][y][z].nodes[nodeIndex].path][fpgaLogic[x][y][z].nodes[nodeIndex].node - 1].y;
	feederZ = paths[fpgaLogic[x][y][z].nodes[nodeIndex].path][fpgaLogic[x][y][z].nodes[nodeIndex].node - 1].z;
	feederPath = -1;
	feederNode = -1;
//	int i;

// loop through paths using this feeder the first nondeletred one is the name that should be used
	for (i = 0; i < (int)fpgaLogic[feederX][feederY][feederZ].nodes.size(); i++) 
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


// return path and node that feeds the register at x, y and z, 
//difference with the non-special is that it does not check for deleted path.
//it returns the path even if it is deleted.
bool get_feeder_special(int x, int y, int z, int & feederPath, int & feederNode) 
{
	if (z % LUTFreq == 0) // ensure that the given cell is a register
		return false;

	if (fpgaLogic[x][y][z].nodes.size() < 1) // make sure that this element is actually used
		return false;

	//	if (fpgaLogic[x][y][z].nodes[0].node < 1) // make sure that this element is not a source, i.e something feeds it
	//		return false;

	int i, nodeIndex;
	nodeIndex = -1;
	for (i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++)
	{
		if (fpgaLogic[x][y][z].nodes[i].node>0) // this is not a source node
		{
			nodeIndex = i;
			break;
		}
	}

	if (nodeIndex == -1) // this FF is not a sink, so it has no feeder. return false
		return false;

	int feederX, feederY, feederZ;

	/// gets the element that feeds the given element through the first path using this node. Even if this path is deleted the feeder x, y and z would be the same, it is imposisble for two different cells to feed the same register
	// [corected now it works with no assumptions]  (old note) --> these assumes the absence of cascaded paths. IT assumes that any node at location x, y, z will  have the same feeder, but what if this register is a source and a sink.{casc}
	feederX = paths[fpgaLogic[x][y][z].nodes[nodeIndex].path][fpgaLogic[x][y][z].nodes[nodeIndex].node - 1].x;
	feederY = paths[fpgaLogic[x][y][z].nodes[nodeIndex].path][fpgaLogic[x][y][z].nodes[nodeIndex].node - 1].y;
	feederZ = paths[fpgaLogic[x][y][z].nodes[nodeIndex].path][fpgaLogic[x][y][z].nodes[nodeIndex].node - 1].z;
	feederPath = -1;
	feederNode = -1;
	//	int i;

	for (i = 0; i < (int)fpgaLogic[feederX][feederY][feederZ].nodes.size(); i++) // loop through paths using this feeder the first nondeletred one is the name that should be used
	{
	//	if (!paths[fpgaLogic[feederX][feederY][feederZ].nodes[i].path][0].deleted)
	//	{
			feederPath = fpgaLogic[feederX][feederY][feederZ].nodes[i].path;
			feederNode = fpgaLogic[feederX][feederY][feederZ].nodes[i].node;
			return true;
	//	}
	}
	return false;
}

bool check_down_link_edge_transition(int i, int j, int k) // returns true if the cell feeded by fpgaLogic[i][j][k].cout is inverting from normal inputs to cout
{

	int x;
	int newI = -1;
	int newJ = -1;
	int newK = -1;
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

// returns the path and node that feeds element x,y,z through portIn
// currPath and currNode will store the values of the path and node in the feeder that feeds x,y, z in portIn
// they are added to determine which BRAM port is feeding
bool get_feeder(int x, int y, int z, int portIn, int & feederPath, int & feederNode) 
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
	// loop through paths using the questioned element, 
	// the first non-deleted path feeding the desired input port is used to get the feeder.
	for (i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++) 
	{
		if (paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].portIn == portIn 
			&& !paths[fpgaLogic[x][y][z].nodes[i].path][0].deleted)
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

// find which port of the BRAM is used to feed x, y, z from portIn
//BRAMoutputport is either port a or b
bool get_feederPort_from_BRAM(int x, int y, int z, int portIn, std::string & BRAMoutputPort, int & BRAMoutputPortIndex)
{

	int feederPath;
	int feederNode;
	assert(get_feeder(x, y, z, portIn, feederPath, feederNode));
	

	int feederX = paths[feederPath][feederNode].x;
	int feederY = paths[feederPath][feederNode].y;
	int feederZ = paths[feederPath][feederNode].z;
	// check that the feeder is a BRAM
	assert(fpgaLogic[feederX][feederY][feederZ].isBRAM);

	int possibleFeedingPath = -1;
	int possibleFeedingNode = -1;

	// loop through paths using the questioned element, 
	// the first non-deleted path feeding the desired input port is used to get the feeder port.
	for (int i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++)
	{
		if (paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].portIn == portIn
			&& !paths[fpgaLogic[x][y][z].nodes[i].path][0].deleted)
		{
			possibleFeedingPath = fpgaLogic[x][y][z].nodes[i].path;
			possibleFeedingNode = fpgaLogic[x][y][z].nodes[i].node;
			break;
		}
	}

	assert(possibleFeedingNode > 0);

	BRAMoutputPortIndex = paths[possibleFeedingPath][possibleFeedingNode - 1].BRAMPortOutIndex;
	if (paths[possibleFeedingPath][possibleFeedingNode - 1].BRAMPortOut == BRAMportAout)
	{
		BRAMoutputPort = "_a_dataout";
	}
	else
	{
		assert(paths[possibleFeedingPath][possibleFeedingNode - 1].BRAMPortOut == BRAMportBout);
		BRAMoutputPort = "_b_dataout";
	}
	return true;
}

// finds the path and node that feeds the BRAM from a specific index in a specific port
// returns true if a feeder was found and false otherwise
bool get_BRAM_feeder(
	int x, // x loc
	int y, // y loc
	int z, // z loc, should always be zero as it's a BRAM
	std::pair <int, int> BRAMportInputInfo,  // .first represnts the port, .second is the index
	int & feederPath,  // the path feeder for the specific pin
	int & feederNode) // the node feeder for the specific pin
{
	// checking that z is zero and that this is a BRAM
	assert(z == 0);
	assert(fpgaLogic[x][y][z].isBRAM);

	int i;


	if (fpgaLogic[x][y][z].nodes.size() < 1) // make sure that this element is actually used
		return false;


	// ensures that this ports is actually used
	if (!fpgaLogic[x][y][z].BRAMinputPorts[BRAMportInputInfo.first][BRAMportInputInfo.second]) 
		return false;


	int possibleFeedingPath = -1;
	int possibleFeedingNode = -1;

	// loop through paths using the questioned element, 
	//the first non-deleted path feeding the desired input port and pin is used to get the feeder.
	for (i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++) 
	{
		if (paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].BRAMPortIn == BRAMportInputInfo.first
			&& paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].BRAMPortInIndex == BRAMportInputInfo.second
			&& !paths[fpgaLogic[x][y][z].nodes[i].path][0].deleted)
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

	// loop through paths using this feeder the first nondeletred one is the name that should be used
	for (i = 0; i < (int)fpgaLogic[feederX][feederY][feederZ].nodes.size(); i++) 
	{
		if (!paths[fpgaLogic[feederX][feederY][feederZ].nodes[i].path][0].deleted)
		{
			feederPath = fpgaLogic[feederX][feederY][feederZ].nodes[i].path;
			feederNode = fpgaLogic[feederX][feederY][feederZ].nodes[i].node;

			assert(feederPath == fpgaLogic[feederX][feederY][feederZ].owner.path);
			assert(feederNode == fpgaLogic[feederX][feederY][feederZ].owner.node);
			return true;
		}
	}
	return false;

}

// returns the path and node that feeds element x,y,z through portIn,
//it is special as it returns a value even if the feeder cell is deleted, but it checks that it did exist before.

bool get_feeder_special(int x, int y, int z, int portIn, int & feederPath, int & feederNode) 
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
	for (i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++) // loop through paths using the questioned element, the first path feeding the desired input port is used to get the feeder.
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



int reverseNumber(int n)
{
	int reversedNumber = 0, remainder;


	while (n != 0)
	{
		remainder = n % 10;
		reversedNumber = reversedNumber * 10 + remainder;
		n /= 10;
	}

	//cout << "Reversed Number = " << reversedNumber;

	return reversedNumber;

}


std::string portNumbertoName(int BRAMportInfo)
{
	if (BRAMportInfo == BRAMportAAddress)
		return "a_address";

	if (BRAMportInfo == BRAMportAData)
		return "a_data";

	if (BRAMportInfo == BRAMportAWE)
		return "a_we";


	if (BRAMportInfo == BRAMportBAddress)
		return "b_address";

	if (BRAMportInfo == BRAMportBData)
		return "b_data";

	if (BRAMportInfo == BRAMportBWE)
		return "b_we";
}