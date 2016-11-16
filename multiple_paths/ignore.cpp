#include "ignore.h"
#include "globalVar.h"
#include "dummyExtras.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int reduce_number_of_LUT_inputs(int x, int y, int z, int excessInputs); // deletes path to free "excessInputs" in LUT i,j,k paths are deleted based on the importance of the port (the more critical the port the more important it is)
int reduce_number_of_LUT_inputs_maximize_tested_paths(int x, int y, int z, int excessInputs); // deletes path to free "excessInputs" in LUT i,j,k paths are deleted based ;
void remove_fanin_higher_than_three() // esnures that he number of inputs + the number of required control signals is <= LUTs inputs
{

	int total = 0;
	int totalIn = 0;
	int all = 0;
	int i, j, k;
	int port1 = -1;
	int port2 = -1;
	int avoidable = 0;
	int requiredControlSignals = 1;// by default we need at least one control signal to control the edge transition (edge)
	std::cout << "removing fanin to satisfy number of inputs constraint " << std::endl;
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				port1 = -1;
				port2 = -1;
				requiredControlSignals = 1;// by default we need at least one control signal to control the edge transition (edge)

				if (fpgaLogic[i][j][k].usedInputPorts < 2) // either a FF or a LUT with only one input so we dont have to do anything
					continue;
				if (check_control_signal_required(i, j, k)) // if fix signal is required then add one to the number of required control signals
				{
					requiredControlSignals++;
				}
				else // todo : should be considered further, possible better solution to handle cout
				{
					if (fpgaLogic[i][j][k].outputPorts[Cout - 5]) // this ensures that the LUT is i arith mode, we can only test paths using maximum 2 input ports, I think this could be relaxed a bit
					{
						requiredControlSignals++;
					}
				}

				int excessInputs = fpgaLogic[i][j][k].usedInputPorts + requiredControlSignals - LUTinputSize;

				if (excessInputs > 0)
					total++;

				totalIn +=  reduce_number_of_LUT_inputs(i, j, k, excessInputs); //reduce_number_of_LUT_inputs_maximize_tested_paths(i, j, k, excessInputs);//


			/*			if (fpgaLogic[i][j][k].usedInputPorts>2) // inputs are more than 2. we will delete some paths
				{
					total++;
					// check which inputs will remain. We decided to keep the most critical ones
					for (x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++)
					{
						if (!paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted && port1 < 0)
						{
							port1 = paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node].portIn;
						}
						else if (!paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted && port1 != paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node].portIn)
						{
							port2 = paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node].portIn;
							break;
						}
					}
					assert(port2 > -1 && port1>-1 && port1 != port2);

					
					bool controlRequired = true;
					// delete all paths not using port1 or port2

					controlRequired = check_control_signal_required(i, j, k);
					if (!controlRequired)
						std::cout << "LUT number " << i << " " << j << " " << k << std::endl;

					for (x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++)
					{
						if (paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node].portIn != port1 && paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node].portIn != port2)
						{
							if (delete_path(fpgaLogic[i][j][k].nodes[x].path))
							{
								totalIn++;
								if (!controlRequired)
									avoidable++;
							}
						}
					}
					
				}*/
			}
		}
	}
	std::cout << "total number of LE requirng to delete inputs to satusfy number of inputs constraint: " << total << std::endl;
	std::cout << "**********Number of deleted paths due to number of inputs contraint is : " << totalIn << " **********" << std::endl;
	std::cout << "Number of deleted paths that could be avoided considering that they dont require a control signal : " << avoidable <<  std::endl;
	IgnoredPathStats << totalIn << "\t";

}


int reduce_number_of_LUT_inputs(int x, int y, int z, int excessInputs) // deletes path to free "excessInputs" in LUT i,j,k paths are deleted based on the importance of the port (the more critical the port the more important it is)
{
	assert(fpgaLogic[x][y][z].usedInputPorts <= LUTinputSize);

	if (excessInputs < 1) // nothing to delete
		return 0;

	std::vector <bool> inputsSeen;
	inputsSeen.resize(LUTinputSize+1); // +1 for cin
	std::fill(inputsSeen.begin(), inputsSeen.end(), false);

	std::vector <std::pair<int, int>> inputImportance;
	inputImportance.resize(0);
	int pathsDeleted = 0;
	int tempPort;

	for (int i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++)
	{
		if (paths[fpgaLogic[x][y][z].nodes[i].path][0].deleted) // deleted path so ignore it
			continue;

		tempPort = paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].portIn;

		if (!inputsSeen[tempPort]) // have not seen this input port before
		{
			inputsSeen[tempPort] = true;
			inputImportance.push_back(std::make_pair(tempPort, fpgaLogic[x][y][z].nodes[i].path));
		}
	}

	for (int i = 0; i < excessInputs; i++)
	{
		tempPort = inputImportance[inputImportance.size() - 1 - i].first; // get the least important port
		for (int j = 0; j < (int)fpgaLogic[x][y][z].nodes.size(); j++)
		{
			if (paths[fpgaLogic[x][y][z].nodes[j].path][0].deleted)
				continue;

			if (paths[fpgaLogic[x][y][z].nodes[j].path][fpgaLogic[x][y][z].nodes[j].node].portIn == tempPort)
			{
				if (delete_path(fpgaLogic[x][y][z].nodes[j].path))
				{
					pathsDeleted++;
				}
			}
		}

	}

	return pathsDeleted;

}


bool compare_port_importance(const std::pair<int, int>&i, const std::pair<int, int>&j)
{
	return i.second > j.second;
}

int reduce_number_of_LUT_inputs_maximize_tested_paths(int x, int y, int z, int excessInputs) // deletes path to free "excessInputs" in LUT i,j,k paths are deleted based 
{
	assert(fpgaLogic[x][y][z].usedInputPorts <= LUTinputSize);

	if (excessInputs < 1) // nothing to delete
		return 0;

	std::vector <bool> inputsSeen;
	inputsSeen.resize(LUTinputSize + 1); // +1 for cin
	std::fill(inputsSeen.begin(), inputsSeen.end(), false);

	std::vector <std::pair<int, int>> inputImportance;
	inputImportance.resize(0);
	int pathsDeleted = 0;
	int tempPort;

	for (int i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++)
	{
		if (paths[fpgaLogic[x][y][z].nodes[i].path][0].deleted) // deleted path so ignore it
			continue;

		tempPort = paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].portIn;

		if (!inputsSeen[tempPort]) // have not seen this input port before
		{
			inputsSeen[tempPort] = true;
			inputImportance.push_back(std::make_pair(tempPort, 1)); // indicate that this port is used and till now it is used only by one path
		}
		else
		{
			for (int j = 0; j < (int)inputImportance.size(); j++)
			{
				if (tempPort == inputImportance[j].first)
				{
					inputImportance[j].second++;
					break;
				}
				assert(j < (int)inputImportance.size());// double check that this port was found in the inputImportance vector
			}
		}
	}

	std::sort(inputImportance.begin(), inputImportance.end(), compare_port_importance);


	for (int i = 0; i < excessInputs; i++)
	{
		tempPort = inputImportance[inputImportance.size() - 1 - i].first; // get the least important port
		for (int j = 0; j < (int)fpgaLogic[x][y][z].nodes.size(); j++)
		{
			if (paths[fpgaLogic[x][y][z].nodes[j].path][0].deleted)
				continue;

			if (paths[fpgaLogic[x][y][z].nodes[j].path][fpgaLogic[x][y][z].nodes[j].node].portIn == tempPort)
			{
				if (delete_path(fpgaLogic[x][y][z].nodes[j].path))
				{
					pathsDeleted++;
				}
			}
		}

	}

	return pathsDeleted;

}


void remove_arithLUT_with_two_inputs_and_no_cin()
{
	int total = 0;
	int i, j, k, kk;
	int total2 = 0;
	bool available = true;
	int port1 = -1;
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				if (fpgaLogic[i][j][k].outputPorts[Cout - 5])// cout is used 
				{
					if (fpgaLogic[i][j][k].usedInputPorts>1) // more than one fanin
					{
						assert(fpgaLogic[i][j][k].usedInputPorts == 2); // only 2 input ports can be used.
																		// check if we require a control signal (to fix the the output) for that cell
						if (!check_control_signal_required(i, j, k))
							continue;
						if (!fpgaLogic[i][j][k].inputPorts[Cin]) // no cin, cout used and more than one fanin is used. We should check ifwe can feed Cin of this cell. If not we will delete the least critical paths
						{
							//	check if the cell feeding cin of i,j,k is available
							available = true;
							if (k > 0) // feeding LUT is in the same LAB
							{
								if (fpgaLogic[i][j][k - 2].utilization != 0 || fpgaLogic[i][j][k - 1].utilization != 0) // if the node is utilized we can not use it, or if the register associated with the feeder node 
									available = false;
							}
							else
							{
								if (j < FPGAsizeY - 1) //not the top lab in the FPGA
								{
									if (fpgaLogic[i][j + 1][/*30*/FPGAsizeZ - 2].utilization != 0 || fpgaLogic[i][j + 1][/*30*/FPGAsizeZ - 1].utilization != 0) // if this node is utilized we can not use it
									{
										available = false;
									}
								}
								else // the top LUT in the fpga, so can not feed its cin (I think)
								{
									available = false;
								}
							}

							// if available is false then we must ensure that only one input port is used through this LUT
							if (!available)
							{
								assert(fpgaLogic[i][j][k].usedInputPorts == 2);

								port1 = -1;
								for (kk = 0; kk < (int)fpgaLogic[i][j][k].nodes.size(); kk++)
								{
									if (!paths[fpgaLogic[i][j][k].nodes[kk].path][0].deleted)
									{
										port1 = paths[fpgaLogic[i][j][k].nodes[kk].path][fpgaLogic[i][j][k].nodes[kk].node].portIn;
										break;
									}
								}
								assert(port1 > -1);

								for (kk = 0; kk <(int)fpgaLogic[i][j][k].nodes.size(); kk++)
								{
									// if a path is not deleted and uses a port other than port1 then delete is man.
									if (!paths[fpgaLogic[i][j][k].nodes[kk].path][0].deleted && paths[fpgaLogic[i][j][k].nodes[kk].path][fpgaLogic[i][j][k].nodes[kk].node].portIn != port1)
									{
										if (delete_path(fpgaLogic[i][j][k].nodes[kk].path))
											total++;

									}
								}

							}

						}
					}
				}
			}
		}
	}
	std::cout << "**********Number of deleted paths due to adder inputs contraint is : " << total << " **********" << std::endl;
	IgnoredPathStats << total << "\t";
}

void remove_feedback_paths()
{
	int total = 0;

	for (int i = 0; i < paths.size(); i++)
	{
		if (paths[i].size() < 1)
			continue;

		if (paths[i][0].x == paths[i][paths[i].size() - 1].x && paths[i][0].y == paths[i][paths[i].size() - 1].y && paths[i][0].z == paths[i][paths[i].size() - 1].z)
		{
			if (delete_path(i))
				total++;
		}
	}
	std::cout << "**********Number of deleted paths due to wrap around paths : " << total << " **********" << std::endl;

}

std::vector<Path_logic_component> number_of_distinct_inputs_to_lab(int x, int y, double & numberOfDistinctInputs)
{
	numberOfDistinctInputs = 0;
	int i, j;
	std::vector<Path_logic_component> nodesWithExternalInput;
	nodesWithExternalInput.resize(0);
	std::map<std::string, bool> inputSource;
	std::string tempKey;
	int currentPath, currentNode;
	bool flag = false;
//	if ((x == 9 && y == 60) || (x == 67 && y == 2))
//		std::cout << "hoppa" << std::endl;
	for (i = 0; i < FPGAsizeZ; i++) // loop across all cells in each lab
	{
		assert(fpgaLogic[x][y][i].usedInputPorts <= LUTinputSize); // extra check
		if (fpgaLogic[x][y][i].utilization == 0)
			continue;


		if (i % 2 == 1) // assumes that the input of the LUT (Xin) is not in the same LAB (conservetive assumption)
		{
			numberOfDistinctInputs += 0.5; // maybe 0.5
			continue;
		}
		//		if (check_control_signal_required_second(x, y, i)) // check if con signal is needed
		//			numberOfDistinctInputs += 2;
		//		else
		//			numberOfDistinctInputs++;

		// check if con signal is needed 
		if (check_control_signal_required_second(x, y, i))
			numberOfDistinctInputs++;

		// check if F signal is needed. if combout is not used, there is no use for control signal F
		if (fpgaLogic[x][y][i].outputPorts[Combout - 5])
			numberOfDistinctInputs++;

		///// in one lab luts with one input can share control signal F (not sure may be not true)
		//		if (fpgaLogic[x][y][i].usedInputPorts == 1) // only one input to the lut
		//			if (flag)
		//			{
		//				numberOfDistinctInputs--;
		//			}
		//			else
		//				flag = true;

		for (j = 0; j <(int)fpgaLogic[x][y][i].nodes.size(); j++) // loop across all nodes and check if the source is within the same lab or not
		{
			currentPath = fpgaLogic[x][y][i].nodes[j].path;
			currentNode = fpgaLogic[x][y][i].nodes[j].node;
			if (paths[currentPath][0].deleted)
				continue;
			if (paths[currentPath][currentNode - 1].x != x || paths[currentPath][currentNode - 1].y != y) // if the source signal is not coming from the same LAB
			{
				// check that we have not counted this source before
				// get Key of this source XYZP 
				tempKey = "X" + std::to_string(paths[currentPath][currentNode - 1].x) + "Y" + std::to_string(paths[currentPath][currentNode - 1].y) + "Z" + std::to_string(paths[currentPath][currentNode - 1].z) + "P" + std::to_string(paths[currentPath][currentNode - 1].portOut);
				auto iter = inputSource.find(tempKey);
				if (iter == inputSource.end()) // was not found, so add it to the map and increment number of distinct inputs to the lab
				{
					nodesWithExternalInput.push_back(Path_logic_component(currentPath, currentNode)); // push the node that uses external inputs
					inputSource.insert(std::pair<std::string, bool>(tempKey, true));
					numberOfDistinctInputs++;
				}
			}
		}
	}
	return nodesWithExternalInput;
}

void remove_to_match_routing_constraint()
{
	int i, j, k;
	double numberOfDistinctInputs;
	int leastImportantNodeIndex;
	int tempX, tempY, tempZ, tempPath, tempNode;
	//int totalDeleted;
	std::vector<Path_logic_component> nodesWithExternalInput;
	int total = 0;
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++) // loop across all LABs and check localinterconnect utilization
		{
			nodesWithExternalInput = number_of_distinct_inputs_to_lab(i, j, numberOfDistinctInputs);
			if (numberOfDistinctInputs < ceil(LocalInterconnectThreshold*LocalInterconnect))
				continue;
			assert(nodesWithExternalInput.size()>0);
			// get least important node to delete it and thus decrease the number of external inputs to this over used LAB
			leastImportantNodeIndex = 0;
			for (k = 0; k <(int)nodesWithExternalInput.size(); k++)
			{
				if (nodesWithExternalInput[k].path>nodesWithExternalInput[leastImportantNodeIndex].path)// then this node is less important
				{
					leastImportantNodeIndex = k;
				}
			}
			/////		// delete node with index K
			/////	total+=clear_le(paths[nodesWithExternalInput[leastImportantNodeIndex].path][nodesWithExternalInput[leastImportantNodeIndex].node].x, paths[nodesWithExternalInput[leastImportantNodeIndex].path][nodesWithExternalInput[leastImportantNodeIndex].node].y, paths[nodesWithExternalInput[leastImportantNodeIndex].path][nodesWithExternalInput[leastImportantNodeIndex].node].z);
			/////		//	clear_le(60, 70, 1); //[with this fix 5000 paths pass perfectly]
			// delete all paths using the same connection as k
			tempX = paths[nodesWithExternalInput[leastImportantNodeIndex].path][nodesWithExternalInput[leastImportantNodeIndex].node].x;
			tempY = paths[nodesWithExternalInput[leastImportantNodeIndex].path][nodesWithExternalInput[leastImportantNodeIndex].node].y;
			tempZ = paths[nodesWithExternalInput[leastImportantNodeIndex].path][nodesWithExternalInput[leastImportantNodeIndex].node].z;
			for (k = 0; k < (int)fpgaLogic[tempX][tempY][tempZ].nodes.size(); k++)
			{
				tempPath = fpgaLogic[tempX][tempY][tempZ].nodes[k].path;
				tempNode = fpgaLogic[tempX][tempY][tempZ].nodes[k].node;
				// check if this is useing the same port as the edge we want to delete
				if (paths[tempPath][tempNode].portIn == paths[nodesWithExternalInput[leastImportantNodeIndex].path][nodesWithExternalInput[leastImportantNodeIndex].node].portIn)
				{

					if (delete_path(tempPath))
					{
						assert(tempPath >= nodesWithExternalInput[leastImportantNodeIndex].path); // the path we are deleting must be less important than the original path we want to delete
						total++;
					}
				}

			}


			// reperat this iteration until LAB satisifes condition todo: may not give optimum result
			j--;
		}
	}

	std::cout << "**********Number of deleted paths due to routing contraint is : " << total << " **********" << std::endl;
	IgnoredPathStats << total << "\t";
}


void remove_to_fix_off_path_inputs() // off path inputs can be fixed using the fix signals. However, this is not enough sometimes you can have reconvergent fanout, this is dealt with in the reconvergent fanout function. This function handles the case when the off-path input is fed by a regiser. It checks if this register can have its output fixed while we test the LUT it is feeding.
{
	int total = 0;

	/*for (int i = 1; i < paths.size(); i++) // loop across all paths, it could be done by looping through fpgalogic instead
	{
		for (int j = 0; j < paths[i].size(); j++)
		{

		}
	}*/

	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ; k++)
			{
				if (k%LUTFreq != 0) // if it's a reg then skip
					continue;

				// LUTs only
				if (fpgaLogic[i][j][k].utilization < 2) // if one or less (zero) paths are using this LUT then skip we will never need to fix an off-path input coz there aint
					continue;
				if (fpgaLogic[i][j][k].usedInputPorts < 2) // no off path inputs since only one port is used
					continue;

				// LUT used with more than one input port used
				for (int z = 0; z < InputPortSize; z++)
				{
					if (fpgaLogic[i][j][k].inputPorts[z]) // check if this port is used
					{
						int pathFeeder, nodeFeeder;
						assert(get_feeder(i, j, k, z, pathFeeder, nodeFeeder)); // get the feeder for this LUT using the z port

						int feeederX = paths[pathFeeder][nodeFeeder].x;
						int feeederY = paths[pathFeeder][nodeFeeder].y;
						int feeederZ = paths[pathFeeder][nodeFeeder].z;

						if (feeederZ%LUTFreq == 0) // if the feeder is a lut then skip this port as we can fix this input port using the fix signal of the feeder LUT
							continue;

					// feeder is a reg
					///	if (!is_cascaded_reg(i, j, k)) // if its not a cascaded reg, then we can control it 
					//		continue;

						
						if (reg_free_input(feeederX, feeederY, feeederZ)) // if its input is free we can control this reg, so thats fine
							continue;

						// could be cascaded or a feed-back path

						int regFeederPath, regFeederNode;
						assert(get_feeder(feeederX, feeederY, feeederZ, regFeederPath, regFeederNode)); // get the feeder for this reg 

						int regFeederX = paths[regFeederPath][regFeederNode].x;
						int regFeederY = paths[regFeederPath][regFeederNode].y;
						int regFeederZ = paths[regFeederPath][regFeederNode].z;

						bool shouldDelete = false;

						std::vector<int> pathsToReg; // vector of paths using the lut feeding the reg and the reg
						pathsToReg.resize(0);
						std::vector<int> pathsIJKandRegFeeder; // paths using lut i,j,k and using an input other than z and going through the lut feeding the reg
						pathsIJKandRegFeeder.resize(0);

						for (int ii = 0; ii < fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes.size();ii++) // loop across all paths using the lut feeding the reg in question
						{
							Path_logic_component tempNode = fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[ii];
							if (paths[tempNode.path][0].deleted) // if this path is deleted, then continue
								continue;

							// check if this path feeds the reg
							if (paths[tempNode.path][tempNode.node + 1].x == feeederX && paths[tempNode.path][tempNode.node + 1].y == feeederY && paths[tempNode.path][tempNode.node + 1].z == feeederZ)
							{
								pathsToReg.push_back(tempNode.path);
							}


							// loop across nodes using LUT i,j,k to see all paths using lut i,k,k from an input other than port z
							for (int l = 0; l < fpgaLogic[i][j][k].nodes.size(); l++)
							{
								if (paths[fpgaLogic[i][j][k].nodes[l].path][0].deleted)
									continue;


								if (paths[fpgaLogic[i][j][k].nodes[l].path][fpgaLogic[i][j][k].nodes[l].node].portIn != z) // found a path using lut i,j,k from a node other than Z
								{
									if (fpgaLogic[i][j][k].nodes[l].path == tempNode.path) // one of the found paths also use the LUt feeding the reg
									{
										pathsIJKandRegFeeder.push_back(fpgaLogic[i][j][k].nodes[l].path);
										shouldDelete = true;
									}


								}

							}

						}

						if (shouldDelete) // we should delte some stuff coz we cant fix all off-path inputs
						{
							std::vector<int> deletedPaths;
							if (pathsIJKandRegFeeder.size() > pathsToReg.size())
								deletedPaths = pathsToReg;
							else
								deletedPaths = pathsIJKandRegFeeder;

							for (int pop = 0; pop < deletedPaths.size(); pop++)
							{
								if (delete_path(deletedPaths[pop]))
								{
									total++;
									std::cout << "fix off path " << deletedPaths[pop] << std::endl;
								}
							}

						}


					}
				}



			}
		}
	}

	std::cout << "*************************************** deleted paths to fix off path inputs = " << total << "**********************************" << std::endl;
	IgnoredPathStats << total << "\t";
}


void  remove_to_toggle_source()
{

	int total = 0;
	for (int i = 1; i < paths.size(); i++)
	{
		if (paths[i].size() < 1)
			continue;

		if (paths[i][0].deleted) // if its deleted
			continue;

		int sourceRegX = paths[i][0].x;
		int sourceRegY = paths[i][0].y;
		int sourceRegZ = paths[i][0].z;

		assert(sourceRegZ%LUTFreq != 0);// muts be reg

		if (paths[i][paths[i].size() - 1].x == sourceRegX && paths[i][paths[i].size() - 1].y == sourceRegY && paths[i][paths[i].size() - 1].z == sourceRegZ) // cascaded path so ignore it for now
			continue;

		if (reg_free_input(sourceRegX, sourceRegY, sourceRegZ)) // if its input is free we can control this reg, so thats fine
			continue;

		// could be cascaded or a feed-back path

		int regFeederPath, regFeederNode;
		assert(get_feeder(sourceRegX, sourceRegY, sourceRegZ, regFeederPath, regFeederNode)); // get the feeder for this reg 

		int regFeederX = paths[regFeederPath][regFeederNode].x;
		int regFeederY = paths[regFeederPath][regFeederNode].y;
		int regFeederZ = paths[regFeederPath][regFeederNode].z;

		bool shouldDelete = false;

		for (int j = 0; j < fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes.size(); j++) // loop across this reg feeder and see if it has the same path as path i
		{
			if (fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[j].path == i)
			{
				if (delete_path(i))
				{
					total++;
					shouldDelete = true;
					break;
				}
			}
		}

		if (!shouldDelete) // path i was not deleted as we can toggle the source reg. But now we will check can we toggle the source reg and at the same time keep all off path-inputs of path i fixed ?
		{

			int destinationX, destinationY, destinationZ;

			for (int j = 0; j < fpgaLogic[regFeederX][regFeederY][regFeederZ].connections.size(); j++) // loop over all connections from regFeeder to its fanout
			{
				// get destination j
				destinationX = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationX;
				destinationY = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationY;
				destinationZ = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationZ;

				if (destinationX == -1) // deleted connection
					continue;

				for (int k = 0; k < fpgaLogic[destinationX][destinationY][destinationZ].nodes.size(); k++)
				{
					if (fpgaLogic[destinationX][destinationY][destinationZ].nodes[k].path == i) // then this destination is a lut that is also used by path i, we can not toggle the source of path i while keeping the off-path inputs at this LUT fixed.
					{
						if (delete_path(i))
						{
							total++;
							std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
							std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
							std::cout << "Hit the extreme corner case" << std::endl;
							break;
						}
					}
				}

			}

		}

	}
	std::cout << "*************************************** deleted paths to control source reg = " << total << "**********************************" << std::endl;
	IgnoredPathStats << total << "\t";
}
void delete_especial_reconvergent_fanout()
{

	unsigned int i, j, k, kk;
	int tempComponentX, tempComponentY, tempComponentZ;
	int deletedPaths = 0;
	Path_logic_component tempNode;
	std::vector < Path_logic_component > rootNodes;
	//std::vector <bool> inputs(InputPortSize, false);
	//	Logic_element tempCell;
	std::cout << "deleteing paths to satisfy reconvergent fanout condition" << std::endl;
	for (i = 0; i < paths.size(); i++) // loop across all paths, another approach would be to loop across used logic elements
	{
		for (j = 0; j < paths[i].size(); j++) // loop across nodes in that path
		{
			if (i == 61 && j == 0)
				std::cout << "debug reconvergent fanout" << std::endl;
			if (paths[i][0].deleted) // this path is deleted then continue to the next path
				break;
			tempComponentX = paths[i][j].x;
			tempComponentY = paths[i][j].y;
			tempComponentZ = paths[i][j].z;

			for (k = 0; k < fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes.size(); k++) // loop across all nodes sharing the LE used by node j in path i
			{
				tempNode = fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes[k];
				if (i == tempNode.path) // same path
					continue;
				////// trial stuff
				if (paths[tempNode.path][0].deleted) // if this path is deleted then dont do anything
					continue;
				if (paths[i][j].portIn != paths[tempNode.path][tempNode.node].portIn) // diffenent paths and use different inputs
				{

					rootNodes.push_back(tempNode);


				}

			}
			////// if we can control the outpuf of an LE then just ensure that all paths using any node before rootNodes are no tested at the same time as path i

			for (k = 0; k < rootNodes.size(); k++)
			{
				tempNode = rootNodes[k];
				tempComponentX = paths[tempNode.path][tempNode.node - 1].x; // get the location of the LE feeding the node in rootNodes
				tempComponentY = paths[tempNode.path][tempNode.node - 1].y;
				tempComponentZ = paths[tempNode.path][tempNode.node - 1].z;
				if (j == 0)
					std::cout << i << " " << j << std::endl;
				//// trial studd
				for (kk = 0; kk < fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes.size(); kk++) // check the presence of the special reconvergent fanout, if it exists then delete the (less critical) path causing this
				{
					if (i == fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes[kk].path) // this means the fanout is present so we must delete all paths between tempNode.node and tempNode.node - 1 
					{
						//paths[tempNode.path][0].deleted = true;
#ifdef CycloneIV
						if (delete_path(tempNode.path))
							deletedPaths++;
#endif

#ifdef StratixV
						if (delete_path_stratix(tempNode.path))
							deletedPaths++;
#endif
						//	std::cout << "deleted : " << tempNode.path << std::endl;

					}

				}


			}
			rootNodes.clear();
			//	std::fill(inputs.begin(), inputs.end(), false);
		}
	}
	//// add edges between cascaded paths

	std::cout << "**********Number of deleted paths due to reconvergent fan-out : " << deletedPaths << " **********" << std::endl;
	IgnoredPathStats << deletedPaths << "\t";// << std::endl;
	//std::cout << "Number of Deleted Paths from delete function due to reconvergent fan-out :  " << deletedPaths << std::endl;

}

void assign_test_phases_ib()
{
	std::vector < std::vector <int> > pathRelationGraph;
	// create the graph representing the connections between paths
	//delete_especial_reconvergent_fanout();
	generate_pathRelationGraph(pathRelationGraph); //creates the PRG and add edges to ensure that all off path inputs of every tested path is fixed (cannot test 2 oaths with fan-in overlap)

	add_cascaded_edges_to_pathRelationGraph(pathRelationGraph);

	// start assigning phases ////////////////////////// graph coloring, coloring pathRelationGraph, could be improved significantly
	unsigned int i, j;
	int testingPhases = 1;
	std::vector <int> possPhases(testingPhases, 1);
	bool newColor;
	for (i = 1; i < pathRelationGraph.size(); i++) // traverse across the graph based on the node criitcality and assign test phases
	{
		/*	minPhase = -2;
		for (j = 0; j < pathRelationGraph[i].size(); j++)
		{
		if (paths[pathRelationGraph[i][j]][0].testPhase < minPhase || minPhase == -2)
		{
		minPhase = paths[pathRelationGraph[i][j]][0].testPhase;
		}
		}
		paths[i][0].testPhase = minPhase+*/
		if (paths[i][0].deleted)
			continue;

		for (j = 0; j < pathRelationGraph[i].size(); j++)
		{
			if (paths[pathRelationGraph[i][j]][0].testPhase>-1) // if color is defined
				possPhases[paths[pathRelationGraph[i][j]][0].testPhase] = 0; // set this as a not allowed color in the possible color list
		}
		newColor = true;
		for (j = 0; j < possPhases.size(); j++)
		{
			if (possPhases[j] == 1 && newColor)
			{
				newColor = false;
				paths[i][0].testPhase = j;
			}
			possPhases[j] = 1; // restore colors for next iteration
		}
		if (newColor) // must introduce a new color
		{
			paths[i][0].testPhase = possPhases.size();
			possPhases.push_back(1);

		}
	}

	numberOfTestPhases = possPhases.size();
}

void add_cascaded_edges_to_pathRelationGraph(std::vector < std::vector <int> > & pathRelationGraph)// add edges to the PRG to handle cascaded paths
{
	
	int i, j, k, l, m;
	for (i = 0; i < FPGAsizeX; i++)// loop through all LUTs of the FPGA fabric
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				if (k%LUTFreq == 0) // this is a LUT
				{
					////// adding edges between all paths using LUT i,j,k and all paths in the cascaded list of LUT i,j,k
					if (fpgaLogic[i][j][k].cascadedPaths.size()>0) // this LUTs feed a cascaded register, then we must add edges between all paths usgin this LUT and paths in the cascaded list
					{
						for (l = 0; l < (int)fpgaLogic[i][j][k].cascadedPaths.size(); l++) // loop across all paths using the cascaded register
						{
							int currentCascadedPath = fpgaLogic[i][j][k].cascadedPaths[l];
							if (paths[currentCascadedPath][0].deleted) // this cascaded path is deleted so go on to the next one
								continue;

							for (m = 0; m < (int)fpgaLogic[i][j][k].nodes.size(); m++) // now loop across all paths using LUT i,j,k
							{

								assert(currentCascadedPath != fpgaLogic[i][j][k].nodes[m].path); // this must be true any path using lut i,j,k can not be a cascaded path that starts in the cascaded FF
							
								if (paths[fpgaLogic[i][j][k].nodes[m].path][0].deleted) // if ythis path is deleted then continue
									continue;

								// add connection between currentCascadedPath and path m of LUT i,j,k
								add_connection(pathRelationGraph, currentCascadedPath, fpgaLogic[i][j][k].nodes[m].path);
							
							}
						}


						// add edges between 

					}

					// adding edges between all paths using LUT i,j,k and all paths in the cascaded list of LUTs that feed LUT i,j,k [already added this to the "generate_pathRelationGraph" function]

					// also adding edges between all paths using the 1st LUT (immediately after a cascaded regsiter) and all paths using the LUT feeding the cascaded register


				}
			}
		}
	}

}
void generate_pathRelationGraph(std::vector < std::vector <int> > & pathRelationGraph)
{
	pathRelationGraph.resize(paths.size());
	unsigned int i, j, k, kk, l;
	bool shouldDelete = false;
	int tempComponentX, tempComponentY, tempComponentZ;
	int deletedPaths = 0;
	Path_logic_component tempNode;
	std::vector < Path_logic_component > rootNodes;
	std::vector <bool> inputs(InputPortSize, false);
	//	Logic_element tempCell;
	for (i = 0; i < paths.size(); i++) // loop across all paths, another (better todo) approach would be to loop across used logic elements
	{
		for (j = 0; j < paths[i].size(); j++) // loop across nodes in that path
		{
			if (paths[i][0].deleted) // this path is deleted then continue to the next path
				break;
			tempComponentX = paths[i][j].x;
			tempComponentY = paths[i][j].y;
			tempComponentZ = paths[i][j].z;

			for (k = 0; k < fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes.size(); k++) // loop across all nodes sharing the LE used by node j in path i
			{
				tempNode = fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes[k];
				if (i == tempNode.path) // same path
					continue;
				////// trial stuff
				if (paths[tempNode.path][0].deleted) // if this path is deleted then dont do anything
					continue;
				if (paths[i][j].portIn != paths[tempNode.path][tempNode.node].portIn) // diffenent paths and use different inputs
				{
					if (!inputs[paths[tempNode.path][tempNode.node].portIn]) // first time to see a path using this input
					{
						inputs[paths[tempNode.path][tempNode.node].portIn] = true;
						rootNodes.push_back(tempNode);
					}

				}

			}
			assert(rootNodes.size() < InputPortSize - 1);
			////// if we can control the outpuf of an LE then just ensure that all paths using any node before rootNodes are no tested at the same time as path i
#ifdef ControlLE
			for (k = 0; k < rootNodes.size(); k++)
			{
				tempNode = rootNodes[k];
				tempComponentX = paths[tempNode.path][tempNode.node - 1].x; // get the location of the LE feeding the node in rootNodes
				tempComponentY = paths[tempNode.path][tempNode.node - 1].y;
				tempComponentZ = paths[tempNode.path][tempNode.node - 1].z;
				//// trial studd
				for (kk = 0; kk < fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes.size(); kk++) // check the presence of the special reconvergent fanout, if it exists then delete the (less critical) path causing this. shouldnt be any since we deleted them earlier
				{
					if (i == fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes[kk].path)
					{
						shouldDelete = true;
						paths[tempNode.path][0].deleted = true;
						std::cout << "deleted : in wrong place man : should have been deleted earlier :  " << tempNode.path << std::endl;
						assert(false);
						deletedPaths++;
					}

				}

				if (!shouldDelete)
				{
					//add edges between path i and all paths using LUT tempComponentX, tempComponentY, tempComponentZ
					for (kk = 0; kk < fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes.size(); kk++) // tod0: currently we fix cout and combout together, if we need to fix combout we also fix cout. This is not necessary we could change that.
					{
						//		if (paths[tempNode.path][tempNode.node - 1].portOut == paths[fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes[kk].path][fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes[kk].node].portOut)
						if (!paths[fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes[kk].path][0].deleted)
							add_connection(pathRelationGraph, i, fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes[kk].path);
						//		else
						//			std::cout << "7assal" << std::endl;

					}
					// the node feeding paths[i][j] is a register, we will check if this is a cascaded register then we must add edges between paths[i][j] and all paths using the LUT feeding this cascaded register
					if (tempComponentZ%LUTFreq != 0) 
					{
						if (is_cascaded_reg(tempComponentX, tempComponentY, tempComponentZ)) // if this is a a cascaded register, then must ensure that LUT
						{
							int cascadedLUTX, cascadedLUTY, cascadedLUTZ;
							int cascadedPathFeeder, cascadedNodeFeeder;
							// get the LUT that feeds this cascaded REG
							assert(get_feeder(tempComponentX, tempComponentY, tempComponentZ, cascadedPathFeeder, cascadedNodeFeeder));

							cascadedLUTX = paths[cascadedPathFeeder][cascadedNodeFeeder].x;
							cascadedLUTY = paths[cascadedPathFeeder][cascadedNodeFeeder].y;
							cascadedLUTZ = paths[cascadedPathFeeder][cascadedNodeFeeder].z;

							assert(fpgaLogic[cascadedLUTX][cascadedLUTY][cascadedLUTZ].cascadedPaths.size() > 0); // must have a cascaded list

							// now add edges between path i and all paths using LUT cascadedLUTX, cascadedLUTX, cascadedLUTX
							for (kk = 0; kk < fpgaLogic[cascadedLUTX][cascadedLUTY][cascadedLUTZ].nodes.size(); kk++) // tod0: currently we fix cout and combout together, if we need to fix combout we also fix cout. This is not necessary we could change that.
							{
								if (!paths[fpgaLogic[cascadedLUTX][cascadedLUTY][cascadedLUTZ].nodes[kk].path][0].deleted)
									add_connection(pathRelationGraph, i, fpgaLogic[cascadedLUTX][cascadedLUTY][cascadedLUTZ].nodes[kk].path);

							}

						}

					}

					// add edges between path i and all paths in the cascaded list of LUT tempComponentX, tempComponentY, tempComponentZ
					for (l = 0; l < fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].cascadedPaths.size(); l++)
					{
						int currentCascadedPath = fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].cascadedPaths[l];
						assert(i != currentCascadedPath); // this must be true any path using lut i,j,k can not be a cascaded path that starts in the cascaded FF

						if (!paths[currentCascadedPath][0].deleted)
							add_connection(pathRelationGraph, i, currentCascadedPath);

					}


				}
				shouldDelete = false;;

			}

#endif

#ifndef ControlLE
			///// if no ability to control output of LE then must recurse back to all sources that can affect each node in rootNodes and disable them
			for (k = 0; k <rootNodes.size(); k++)
			{
				//	tempCell = fpgaLogic[paths[rootNodes[k].path][rootNodes[k].node - 1].x][paths[rootNodes[k].path][rootNodes[k].node - 1].y][paths[rootNodes[k].path][rootNodes[k].node - 1].z];

				unkown(pathRelationGraph, i, paths[rootNodes[k].path][rootNodes[k].node - 1]);
			}
#endif
			rootNodes.clear();
			std::fill(inputs.begin(), inputs.end(), false);
		}
	}
	int wrapAroundPaths = 0;

	//// add edges between cascaded paths
	for (i = 1; i < paths.size(); i++) // loop over all paths
	{
		if (paths[i][0].deleted)
			continue;
		for (j = 0; j < fpgaLogic[paths[i][0].x][paths[i][0].y][paths[i][0].z].nodes.size(); j++) // loop across nodes using the source of the ith pass
		{
			if (fpgaLogic[paths[i][0].x][paths[i][0].y][paths[i][0].z].nodes[j].node != 0) // if a path uses this node and not at its first node then these two paths are cascaded
			{
	//			std::cout << "7assal CASCADE " << std::endl;
				if (i == fpgaLogic[paths[i][0].x][paths[i][0].y][paths[i][0].z].nodes[j].path) // this is a wraparound path. its source and target are the same FF. My current idea is to use syn clear or sync load to test these kind of paths. TO do so we must ensure that no other registers are used in the same LAB because of the global control signals
				{
				//	if (delete_path(i))
				//	{
						//assert(false);
				//		wrapAroundPaths++;
				//		continue; // dont add an edge
				//	}
//					std::cout << "feedback path " << std::endl;

				}
				else
				{
					add_connection(pathRelationGraph, i, fpgaLogic[paths[i][0].x][paths[i][0].y][paths[i][0].z].nodes[j].path);

				}

			}
		}
	}

	std::cout << "Number of Deleted Paths in generate pathRelation should be zero and it is : " << deletedPaths << std::endl;
	std::cout << "**********Number of deleted paths due to wrap around paths : " << wrapAroundPaths << " **********" << std::endl;
}


void add_connection(std::vector < std::vector <int> > & pathRelationGraph, int x, int y) // x and y paths that should not be tested together
{
	bool connExist = false;
	int i;
//	if (x == 0 || y == 0)
//		std::cout << "efef";
	for (i = 0; i < (int)pathRelationGraph[x].size(); i++) // loop across all neighbours of x
	{
		if (pathRelationGraph[x][i] == y) // if y is already connected to x then dont add it
		{
			connExist = true;
			break;
		}
	}
	assert(x != y);
	if (!connExist) // no connection exists, so add connection
	{
		if (x == y)
			std::cout << y << std::endl;
		pathRelationGraph[x].push_back(y);
		pathRelationGraph[y].push_back(x);
	}

}


void unkown(std::vector < std::vector <int> > & pathRelationGraph, int i, Path_node  tempCell)
{
	unsigned int   k, x, y;
	bool connExist = false;
	Path_logic_component tempNode;
	std::vector < Path_logic_component > rootNodes;
	std::vector <bool> inputs(InputPortSize, false);

	if (tempCell.z % 2 == 1) // this is a register, all paths starting at this register can not be tested with path i
	{
		for (y = 0; y < fpgaLogic[tempCell.x][tempCell.y][tempCell.z].nodes.size(); y++)
		{
			tempNode = fpgaLogic[tempCell.x][tempCell.y][tempCell.z].nodes[y];
			// connect path i and path tempNode.path in the pathRelationGraph
			// add connection to path i
			connExist = false;
			for (x = 0; x < pathRelationGraph[i].size(); x++)
			{
				if (pathRelationGraph[i][x] == tempNode.path) // edge already exists
				{
					connExist = true;
					break;
				}
			}
			if (!connExist)
			{
				pathRelationGraph[i].push_back(tempNode.path);
				if (i == tempNode.path)
					std::cout << i << std::endl;
			}
			connExist = false;
			// add connection to path rempNode.path
			for (x = 0; x < pathRelationGraph[tempNode.path].size(); x++)
			{
				if (pathRelationGraph[tempNode.path][x] == i) // edge already exists
				{
					connExist = true;
					break;
				}
			}
			if (!connExist)
				pathRelationGraph[tempNode.path].push_back(i);

			connExist = false;

		}
	}
	else
	{
		for (k = 0; k < fpgaLogic[tempCell.x][tempCell.y][tempCell.z].nodes.size(); k++)
		{
			tempNode = fpgaLogic[tempCell.x][tempCell.y][tempCell.z].nodes[k];
			if (i == tempNode.path) // same path
				continue;
			if (!inputs[paths[tempNode.path][tempNode.node].portIn]) // first time to see a path using this input
			{
				inputs[paths[tempNode.path][tempNode.node].portIn] = true;
				rootNodes.push_back(tempNode);
			}
		}
		assert(rootNodes.size() < InputPortSize - 1);
		for (k = 0; k <rootNodes.size(); k++)
		{
			unkown(pathRelationGraph, i, paths[rootNodes[k].path][rootNodes[k].node - 1]);
		}
		rootNodes.clear();
		std::fill(inputs.begin(), inputs.end(), false);
	}



}


#ifdef StratixV 

void remove_excess_fan_in()  // this function deletes paths to ensure that the remainig paths can be tested.
{ 
	int topZ, bottomZ, almInputs, topInputs, bottomInputs, sharedInputs, controlTop, controlBottom;
	int pathsDeletedFromALM = 0;
	int pathsDeletedFromALUT = 0;
	bool repeat = false;
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ / ALUTtoALM; k++) // loop across ALMs
			{
				repeat = false;
				if (alms[i][j][k].numOfInputs>3) // if its greater than 4, then may be we have to ignore some paths (not true change ())
				{
					topZ = k *ALUTtoALM;
					bottomZ = topZ + LUTFreq;
					almInputs = alms[i][j][k].numOfInputs;
					topInputs = fpgaLogic[i][j][topZ].usedInputPorts;
					bottomInputs = fpgaLogic[i][j][bottomZ].usedInputPorts;
					sharedInputs =  (topInputs + bottomInputs) - almInputs;
					assert((fpgaLogic[i][j][topZ].sharedInputPorts == fpgaLogic[i][j][bottomZ].sharedInputPorts));
					assert(sharedInputs == fpgaLogic[i][j][topZ].sharedInputPorts);

					assert(topInputs <= LUTinputSize);
					assert(bottomInputs <= LUTinputSize);

					//////////////////////////////////////////// check how many control signals needed, Here we assume all cells need an edge signal, but fix signal may not be required. We also assum that the control signals are not shared between ALUTs within an ALM.
					
					controlTop = 0;
					controlBottom = 0;
					if (topInputs > 0)
					{
						controlTop = 1;
						if (check_control_signal_required(i, j, topZ))
						{
							controlTop++;
						}
					}
					/////////// check bottom ALUT control signals
					if (bottomInputs > 0)
					{
						controlBottom = 1;
						if (check_control_signal_required(i, j, bottomZ))
						{
							controlBottom++;
						}
					}
					if (almInputs == 4 && sharedInputs == 0) // no need to delete (4,0),(3,1),(2,2) --> no share all possible to test
						continue;

					// check if alm can be tested currently
					if (almInputs + controlBottom + controlTop <= ALMinputSize) // make sure that we can input all inputs and control signals
					{
						if ((topInputs + controlTop) < LUTinputSize && (bottomInputs + controlBottom) < LUTinputSize) // Given that total inputs are less than eihght, if both luts are less than 6 input then no need to delete any thing 
							continue;
						else // total inputs are less than eight but one or both LUTs are 6 inputs, in this case we only have to delete an input to the ALUT and not cut the input from the ALM completely
						{
							if ((topInputs + controlTop) == LUTinputSize && (bottomInputs + controlBottom) == 0) // one LUT is 6 inputs the other is zero inputs then great
								continue;

							if ((topInputs + controlTop) == 0 && (bottomInputs + controlBottom) == LUTinputSize) // one LUT is 6 inputs the other is zero inputs then great
								continue;
							// at least one LUT is 6-inputs and the other is non-zero inputs, reduce number of inputs to 
						//	if (!repeat) // means new ALM
						//	{
						/*		std::cout << "reduce_ALUT" << std::endl;
								std::cout << "TOP ALUT X: " << i << " Y: " << j << " Z_top: " << topZ << " Z_bottom: " << bottomZ << std::endl;
								std::cout << "ALM inputs: " << almInputs << " topcontrol signals : " << controlTop << " bottom control signal " << controlBottom <<  std::endl;
								std::cout << "top inputs: " << topInputs << " Bottom inputs: " << bottomInputs << std::endl;
								std::cout << "shared Inputs : " << sharedInputs << std::endl;
								std::cout << "///////////////////////////////////////////////////////////////////////////////////////" << std::endl;
						//	}*/
							pathsDeletedFromALUT+=reduce_ALUT_inputs(i, j, k, topInputs, bottomInputs);
							repeat = true;
							k--;
						}
					}
					else // more inputs than available 8 inputs to ALM, so we must reduce ALM inputs
					{
					//	if (!repeat) // means new ALM
					//	{
					/*		std::cout << "reduce_ALM" << std::endl;
							std::cout << "TOP ALUT X: " << i << " Y: " << j << " Z_top: " << topZ << " Z_bottom: " << bottomZ << std::endl;
							std::cout << "ALM inputs: " << almInputs << " topcontrol signals : " << controlTop << " bottom control signal " << controlBottom << std::endl;
							std::cout << "top inputs: " << topInputs << " Bottom inputs: " << bottomInputs << std::endl;
							std::cout << "shared Inputs : " << sharedInputs << std::endl;
							std::cout << "///////////////////////////////////////////////////////////////////////////////////////" << std::endl;
					//	}*/
						repeat = true;
						pathsDeletedFromALM+=reduce_ALM_inputs(i, j, k);
						k--;
					}

				}
			}
		}
	}
	std::cout << "Paths deleted from ALM = " << pathsDeletedFromALM << std::endl;
	std::cout << "Paths deleted from ALUT = " << pathsDeletedFromALUT << std::endl;
	std::cout << "Total Paths Delted from Removing Excess Fan-in = " << pathsDeletedFromALM + pathsDeletedFromALUT << std::endl;
}



void handle_port_e() // avoids the case where port E is used , both LUTs in the ALM are used and not in 6-input mode
{
	// loop across alms, check for ALUTs using port E and both ALUTs in the same ALM are used
	int total_paths = 0;
	bool con_top, con_bott;
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ / 6; k++)
			{
				// check how many alms use port E and have two different outputs (both top and bottom LUTS are used)
				if (fpgaLogic[i][j][k*ALUTtoALM].utilization>0 && fpgaLogic[i][j][k*ALUTtoALM + LUTFreq].utilization>0) // check that both ALUTs in the ALM are used
				{
					if (fpgaLogic[i][j][k*ALUTtoALM].inputPorts[portE] || fpgaLogic[i][j][k*ALUTtoALM + LUTFreq].inputPorts[portE]) // check if port E is used, then something must go man
					{
						con_top = check_control_signal_required(i, j, k*ALUTtoALM);
						con_bott = check_control_signal_required(i, j, k*ALUTtoALM+LUTFreq);
						// either signals using port E must go or one ALUT must go
						total_paths+=reduce_due_to_port_e(i, j, k);
					}
				}
			}
		}
	}
	std::cout << "Total Paths Delted from port e constraint = " << total_paths << std::endl;
}

void handle_port_d_shared_with_e() // handles the case when the created LUT has 6 inputs and one of them is port D from the pin that is also connected to port E (coming from the original circuit). In that case somehting must go because we cannot connect D from this pin and still connect to port E
{
	int totalDeleted = 0;
	bool flagD;
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ; k++)
			{
				if (fpgaLogic[i][j][k].usedInputPorts == 4 && check_control_signal_required(i, j, k)) // this means we need to isntantiate a 6-inputs LUT, so we must ensure that port D is not used through the shitty PIN
				{
					// check if port D is used
					if (!fpgaLogic[i][j][k].inputPorts[portD]) // port D is not used
						continue;

					/////////////////////////////////////////////////
					// check if the block inpout mux has an odd index value, in this case dont add a dummy as this pin can not be shared with port E
					flagD = false;
				
					int feederPath, feederNode;
					// must find which LUT feeds port D of cell i,j, topZ
					assert(get_feeder(i, j, k, portD, feederPath, feederNode));
					int xFeeder = paths[feederPath][feederNode].x;
					int yFeeder = paths[feederPath][feederNode].y;
					int zFeeder = paths[feederPath][feederNode].z;

					// find routing connection from feeder to the current cell
					for (int x = 0; x < (int)fpgaLogic[xFeeder][yFeeder][zFeeder].connections.size(); x++)
					{
						if (fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].destinationX == i && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].destinationY == j && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].destinationZ == k && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].destinationPort == portD)
						{
							assert(fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].usedRoutingResources.size()>1);
							std::string blockInput = fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].usedRoutingResources[fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].usedRoutingResources.size() - 1]; // the last routing element is th e block input mux
							assert(blockInput[0] == 'B');
							if ((blockInput[blockInput.size() - 1] - '0') % 2 == 0) // if it is even
							{
								if (need_to_add_dummy(blockInput))
									flagD = true;
								else
									flagD = false;
							}
							else
								flagD = false;

							break;
						}
					}

					if (flagD) // port d is used through the shitty pin, then we must reduce one input from this LUT
					{
						// since port D is used through the shitty pin, port E can not be used, double check this
						assert(!fpgaLogic[i][j][k].inputPorts[portE]);
						int currentALM = k / ALUTtoALM;
						totalDeleted+=reduce_ALUT_inputs(i,j , currentALM, fpgaLogic[i][j][currentALM*ALUTtoALM].usedInputPorts, fpgaLogic[i][j][currentALM*ALUTtoALM + LUTFreq].usedInputPorts);

					}
					
					//////////////////////////////////
				}
			}
		}
	}


	std::cout << "Total Paths Delted from connecting port D to shitty pin and requiring 6 inputs-LUTt = " << totalDeleted << std::endl;
}

int reduce_ALUT_inputs(int x, int y, int z, int topInputs, int bottomInputs) // takes co-ordinates of the ALM , the number of inputs to the top and bottom luts. This functions is called when the number of inputs to the alm is < 8.
{
	assert(alms[x][y][z].numOfInputs < ALMinputSize); // however this does not include the contol signals. it should include that.
	/// initialize all port as not seen
	std::vector <bool> topInputsSeen;
	topInputsSeen.resize(LUTinputSize);
	std::fill(topInputsSeen.begin(), topInputsSeen.end(), false);

	std::vector <bool> bottomInputsSeen;
	bottomInputsSeen.resize(LUTinputSize);
	std::fill(bottomInputsSeen.begin(), bottomInputsSeen.end(), false);

	// empty vectors that will store the input ports by imprtance (criticality), the pairs used is (port,path)
	std::vector <std::pair<int, int>> topInputImportance;
	topInputImportance.resize(0);
	std::vector <std::pair<int, int>> bottomInputImportance;
	bottomInputImportance.resize(0);
	
	int topZ = z *ALUTtoALM;
	int bottomZ = topZ + LUTFreq;
	
	int tempPort;
	// order input ports of the top ALUT
	for (int i = 0; i < (int)fpgaLogic[x][y][topZ].nodes.size(); i++)
	{
		if (paths[fpgaLogic[x][y][topZ].nodes[i].path][0].deleted)
			continue;
		tempPort = paths[fpgaLogic[x][y][topZ].nodes[i].path][fpgaLogic[x][y][topZ].nodes[i].node].portIn;

		if (!topInputsSeen[tempPort]) // have not seen this input port before
		{
			topInputsSeen[tempPort] = true;
			topInputImportance.push_back(std::make_pair(tempPort, fpgaLogic[x][y][topZ].nodes[i].path));
		}
	}
	// order input ports of the bottom ALUT
	for (int i = 0; i < (int)fpgaLogic[x][y][bottomZ].nodes.size(); i++)
	{
		if (paths[fpgaLogic[x][y][bottomZ].nodes[i].path][0].deleted)
			continue;
		tempPort = paths[fpgaLogic[x][y][bottomZ].nodes[i].path][fpgaLogic[x][y][bottomZ].nodes[i].node].portIn;

		if (!bottomInputsSeen[tempPort]) // have not seen this input port before
		{
			bottomInputsSeen[tempPort] = true;
			bottomInputImportance.push_back(std::make_pair(tempPort, fpgaLogic[x][y][bottomZ].nodes[i].path));
		}
	}
	// top and bottom LUTs must be used, // this assertion is not necessarily true. MAybe a LUT has 7 inputs and the other has zero LUTs
	assert(bottomInputImportance.size() > 0 || topInputImportance.size() > 0);

	if (bottomInputImportance.size() == 0)
		goto delete_from_top;
	if (topInputImportance.size() == 0)
		goto delete_from_bottom;

	int worstLUT;
	int worstPort;
	if (bottomInputImportance[bottomInputImportance.size() - 1].second > topInputImportance[topInputImportance.size() - 1].second) // bottom LUT has a less critical port and it will be deleted
	{
	delete_from_bottom:
		worstLUT = bottomZ;
		worstPort = bottomInputImportance[bottomInputImportance.size() - 1].first;
	}
	else
	{
	delete_from_top:
		worstLUT = topZ;
		worstPort = topInputImportance[topInputImportance.size() - 1].first;
	}

	int pathsDeleted = 0;

	/// delete paths using worst port in worsLUT
	for (int i = 0; i < (int)fpgaLogic[x][y][worstLUT].nodes.size(); i++)
	{

		if (paths[fpgaLogic[x][y][worstLUT].nodes[i].path][fpgaLogic[x][y][worstLUT].nodes[i].node].portIn == worstPort)
		{
			if (delete_path_stratix(fpgaLogic[x][y][worstLUT].nodes[i].path))
			{
				pathsDeleted++;
			}
		}
	}
	return pathsDeleted;
}

int reduce_ALM_inputs(int x, int y, int z) // reduce an alm inputs
{
	assert(alms[x][y][z].numOfInputs < ALMinputSize); // however this does not include the contol signals. it should include that.
													  /// initialize all port as not seen
	std::vector <bool> topInputsSeen;
	topInputsSeen.resize(LUTinputSize);
	std::fill(topInputsSeen.begin(), topInputsSeen.end(), false);

	std::vector <bool> bottomInputsSeen;
	bottomInputsSeen.resize(LUTinputSize);
	std::fill(bottomInputsSeen.begin(), bottomInputsSeen.end(), false);

	// empty vectors that will store the input ports by imprtance (criticality), the pairs used is (port,path)
	std::vector <std::pair<int, int>> topInputImportance;
	topInputImportance.resize(0);
	std::vector <std::pair<int, int>> bottomInputImportance;
	bottomInputImportance.resize(0);

	int topZ = z *ALUTtoALM;
	int bottomZ = topZ + LUTFreq;

	int tempPort;
	// order input ports of the top ALUT
	for (int i = 0; i < (int)fpgaLogic[x][y][topZ].nodes.size(); i++)
	{
		if (paths[fpgaLogic[x][y][topZ].nodes[i].path][0].deleted)
			continue;
		tempPort = paths[fpgaLogic[x][y][topZ].nodes[i].path][fpgaLogic[x][y][topZ].nodes[i].node].portIn;

		if (!topInputsSeen[tempPort]) // have not seen this input port before
		{
			topInputsSeen[tempPort] = true;
			topInputImportance.push_back(std::make_pair(tempPort, fpgaLogic[x][y][topZ].nodes[i].path));
		}
	}
	// order input ports of the bottom ALUT
	for (int i = 0; i <(int)fpgaLogic[x][y][bottomZ].nodes.size(); i++)
	{
		if (paths[fpgaLogic[x][y][bottomZ].nodes[i].path][0].deleted)
			continue;
		tempPort = paths[fpgaLogic[x][y][bottomZ].nodes[i].path][fpgaLogic[x][y][bottomZ].nodes[i].node].portIn;

		if (!bottomInputsSeen[tempPort]) // have not seen this input port before
		{
			bottomInputsSeen[tempPort] = true;
			bottomInputImportance.push_back(std::make_pair(tempPort, fpgaLogic[x][y][bottomZ].nodes[i].path));
		}
	}

	//// now update the importance in case something is shared
	for (int i = 0; i < (int)topInputImportance.size(); i++)
	{
		if (fpgaLogic[x][y][topZ].isInputPortShared[topInputImportance[i].first]) // this port is shared, check if this port is used with higher criticality in the bottom ALUT
		{
			int sharedPort = fpgaLogic[x][y][topZ].sharedWith[topInputImportance[i].first];
			for (int j = 0; j < (int)bottomInputImportance.size(); j++)
			{
				if (sharedPort == bottomInputImportance[j].first) // found shared port in the bottom LUT
				{
					assert(fpgaLogic[x][y][topZ].isInputPortShared[topInputImportance[i].first] && fpgaLogic[x][y][bottomZ].isInputPortShared[bottomInputImportance[j].first]);
					assert(topInputImportance[i].second != bottomInputImportance[j].second);
					if (topInputImportance[i].second < bottomInputImportance[j].second) // top LUT use th input with a more critical path
					{
						bottomInputImportance[j].second = topInputImportance[i].second;
					}
					else
					{
						topInputImportance[i].second = bottomInputImportance[j].second;
					}
				}
			}
		}
	}

//// ge the worst top and bottom lut input
	//int worstLUT;
	int worstPort;
	
	int worstIndexTop, worstIndexBottom;

	if (topInputImportance.size() == 0)
	{
		worstIndexTop = -1;
	}
	else
	{
		worstIndexTop = 0;// topInputImportance[0].first;
	}

	if (bottomInputImportance.size() == 0)
	{
		worstIndexBottom = -1;
	}
	else
	{
		worstIndexBottom = 0;// bottomInputImportance[0].first;
	}

	for (int i = 0; i < (int)topInputImportance.size(); i++)
	{
		if (topInputImportance[i].second > topInputImportance[worstIndexTop].second)
		{
			worstIndexTop = i;
		}
	}

	for (int i = 0; i < (int)bottomInputImportance.size(); i++)
	{
		if (bottomInputImportance[i].second > bottomInputImportance[worstIndexBottom].second)
		{
			worstIndexBottom = i;
		}
	}

	assert((worstIndexBottom != -1) || (worstIndexBottom != -1));

	if (worstIndexBottom == -1)
		goto delete_from_top;

	if (worstIndexTop == -1)
		goto delete_from_bottom;


	int pathsDeleted = 0;
	if ((topInputImportance[worstIndexTop].second > bottomInputImportance[worstIndexBottom].second)) // bottom is more critical 
	{
	delete_from_top:	
		worstPort = topInputImportance[worstIndexTop].first;
		/// delete paths using worst port in worsLUT
		for (int i = 0; i < (int)fpgaLogic[x][y][topZ].nodes.size(); i++)
		{

			if (paths[fpgaLogic[x][y][topZ].nodes[i].path][fpgaLogic[x][y][topZ].nodes[i].node].portIn == worstPort)
			{
				if (delete_path_stratix(fpgaLogic[x][y][topZ].nodes[i].path))
				{
					pathsDeleted++;
				}
			}
		}
		return pathsDeleted;
	}
	else // top is more critical or equal criticality
	{
	delete_from_bottom:
		worstPort = bottomInputImportance[worstIndexBottom].first;
		/// delete paths using worst port in worsLUT
		for (int i = 0; i < (int)fpgaLogic[x][y][bottomZ].nodes.size(); i++)
		{

			if (paths[fpgaLogic[x][y][bottomZ].nodes[i].path][fpgaLogic[x][y][bottomZ].nodes[i].node].portIn == worstPort)
			{
				if (delete_path_stratix(fpgaLogic[x][y][bottomZ].nodes[i].path))
				{
					pathsDeleted++;
				}
			}
		}
	}

	if (worstIndexBottom == -1 || worstIndexTop == -1)
		return pathsDeleted;

	// if we reached this point, then we have deleted the paths connected to the worst input in the bottom LUT, now check if this is shared with top, if so then delete paths from top LUT too
	if (topInputImportance[worstIndexTop].second == bottomInputImportance[worstIndexBottom].second)
	{
		worstPort = topInputImportance[worstIndexTop].first;
		/// delete paths using worst port in worsLUT
		for (int i = 0; i < (int)fpgaLogic[x][y][topZ].nodes.size(); i++)
		{

			if (paths[fpgaLogic[x][y][topZ].nodes[i].path][fpgaLogic[x][y][topZ].nodes[i].node].portIn == worstPort)
			{
				if (delete_path_stratix(fpgaLogic[x][y][topZ].nodes[i].path))
				{
					pathsDeleted++;
				}
			}
		}
	}
	return pathsDeleted;
}

int reduce_due_to_port_e(int x, int y, int z) // currently delete such that we keep the most critical path intact
{
	int top_priority, bottom_priority, port_e_priority; // priorities of top, bottom ALUT and port e
	int topZ = z*ALUTtoALM;
	int bottomZ = z*ALUTtoALM + LUTFreq;
	port_e_priority = -1;
	bool top_use_e = false; // top alut uses port E
	bool bottom_use_e = false; // bottom alut uses port e

	int paths_deleted = 0;

	top_priority = fpgaLogic[x][y][topZ].owner.path;
	bottom_priority = fpgaLogic[x][y][bottomZ].owner.path;

	// check port e priority by checking the most critical path using port e from the top and bottom LUTs
	for (int i = 0; i < (int)fpgaLogic[x][y][topZ].nodes.size(); i++)
	{
		if (paths[fpgaLogic[x][y][topZ].nodes[i].path][0].deleted)
			continue;
		if (paths[fpgaLogic[x][y][topZ].nodes[i].path][fpgaLogic[x][y][topZ].nodes[i].node].portIn == portE)
		{
			port_e_priority = fpgaLogic[x][y][topZ].nodes[i].path;
			top_use_e = true;
			break;
		}
	}


	for (int i = 0; i <  (int)fpgaLogic[x][y][bottomZ].nodes.size(); i++)
	{
		if (paths[fpgaLogic[x][y][bottomZ].nodes[i].path][0].deleted)
			continue;
		if (paths[fpgaLogic[x][y][bottomZ].nodes[i].path][fpgaLogic[x][y][bottomZ].nodes[i].node].portIn == portE)
		{
			bottom_use_e = true;
			if (fpgaLogic[x][y][bottomZ].nodes[i].path < port_e_priority || port_e_priority < 0)
			{
				port_e_priority = fpgaLogic[x][y][bottomZ].nodes[i].path;
			}
			break;
		}
	}


	// we have two cases, 
	//first that both ALUTs are using port e
	if (bottom_use_e&&top_use_e)
	{
		assert(fpgaLogic[x][y][topZ].inputPorts[portE] && fpgaLogic[x][y][bottomZ].inputPorts[portE]);

		if (top_priority < bottom_priority) // top LUT is more importane, so it is not deleted
		{
			if (bottom_priority < port_e_priority) // the least important path uses port e so we will delete all paths using port e in the top and bottom ALUT, this will allow us to complete the testing procedure
			{
				paths_deleted+=delete_ALUT_port_stratix(x, y, topZ, portE);
				paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE);
			}
			else if (port_e_priority < bottom_priority)// delete the bottom LUT
			{
		//		if (fpgaLogic[x][y][bottomZ].usedInputPorts>1) // trial. if more than one input port is used then may be it is better to delete port e instead of all bottom AUT
		///		{
		//			paths_deleted += delete_ALUT_port_stratix(x, y, topZ, portE);
		//			paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE);
		///		}
		//		else
		//		{
					paths_deleted += delete_ALUT_stratix(x, y, bottomZ);

//				}
			}
			else // bottom=port_e and they are less important than top, delete e or b which ever has less effect on number of paths deleted
			{
				
				if (fpgaLogic[x][y][bottomZ].usedInputPorts>1) // check if the bottom ALUT has more than one port used, then it migh make sense to delete port e instead of the whole bottom ALUT
				{
					paths_deleted += delete_ALUT_port_stratix(x, y, topZ, portE);
					paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE);
				}
				else
				{
					// delete bottom ALUT

			//		if (fpgaLogic[x][y][bottomZ].usedInputPorts>1) // trial. if more than one input port is used then may be it is better to delete port e instead of all bottom AUT
			//		{
			//			paths_deleted += delete_ALUT_port_stratix(x, y, topZ, portE);
			///			paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE);
			//		}
			//		else
			//		{
						paths_deleted += delete_ALUT_stratix(x, y, bottomZ);
		//			}
					
				}
			}
		}
		else if (bottom_priority < top_priority)// bottom LUT is more important
		{
			if (top_priority < port_e_priority) // the least important path uses port e so we will delete all paths using port e in the top and bottom ALUT, this will allow us to complete the testing procedure
			{
				paths_deleted += delete_ALUT_port_stratix(x, y, topZ, portE);
				paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE);
			}
			else if (port_e_priority < top_priority) // delete top ALUT
			{

			//	if (fpgaLogic[x][y][topZ].usedInputPorts>1) // trial. if more than one input port is used then may be it is better to delete port e instead of all bottom AUT
			//	{
			//		paths_deleted += delete_ALUT_port_stratix(x, y, topZ, portE);
			//		paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE);
			//	}
			//	else
			//	{
					paths_deleted += delete_ALUT_stratix(x, y, topZ);

			//	}
			}
			else// top=port_e and they are less important than top, delete e or t which ever has less effect on number of paths deleted
			{
				if (fpgaLogic[x][y][bottomZ].usedInputPorts>1) // check if the bottom ALUT has more than one port used, then it migh make sense to delete port e instead of the whole bottom ALUT
				{
					paths_deleted += delete_ALUT_port_stratix(x, y, topZ, portE);
					paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE);
				}
				else
				{

				//	if (fpgaLogic[x][y][topZ].usedInputPorts>1) // trial. if more than one input port is used then may be it is better to delete port e instead of all bottom AUT
				//	{
				//		paths_deleted += delete_ALUT_port_stratix(x, y, topZ, portE);
				//		paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE);
				//	}
				//	else
				//	{
						paths_deleted += delete_ALUT_stratix(x, y, topZ);
				//	}
					// delete top ALUT
					
				}
			}
		}
		else // top and bottom are equal
		{
			if ((top_priority > port_e_priority)) //port e is more important, so delete top or bottom, whichever has less paths
			{
				if (fpgaLogic[x][y][topZ].utilization  > fpgaLogic[x][y][bottomZ].utilization)
				{
					paths_deleted += delete_ALUT_stratix(x, y, bottomZ);
				}
				else
				{
					paths_deleted += delete_ALUT_stratix(x, y, topZ);
				}

			}
			else // either top=bottom=edge or top=bottom (more important than port_e), then delete port e
			{
				if (top_priority == port_e_priority) // top=bottom = e
				{
					if (fpgaLogic[x][y][bottomZ].usedInputPorts==1) // if it only has ine used port (e), then just delete this
					{ 
						paths_deleted += delete_ALUT_stratix(x, y, bottomZ);
					}
					else if (fpgaLogic[x][y][topZ].usedInputPorts == 1)
					{ 
						paths_deleted += delete_ALUT_stratix(x, y, topZ);
					}
					else // delete port e
					{
						paths_deleted += delete_ALUT_port_stratix(x, y, topZ, portE);
						paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE);
					}


				}
				else // top = bottom and they are more important than e, then de;lete e
				{
					paths_deleted += delete_ALUT_port_stratix(x, y, topZ, portE);
					paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE);

				}
			}

		}
	}
	else // second, only one ALUT is using port E
	{
		assert(!(fpgaLogic[x][y][topZ].inputPorts[portE] && fpgaLogic[x][y][bottomZ].inputPorts[portE]));

		if (top_use_e) // top ALUT uses port e, so we have to either delete port e or delete bottom port
		{
			if (bottom_priority < port_e_priority) // delete port e from top LUT
			{
				paths_deleted += delete_ALUT_port_stratix(x, y, topZ, portE);
			}
			else
			{
				if (port_e_priority < bottom_priority) // port e is more important so delete bottom ALUT
				{
			//		if (fpgaLogic[x][y][bottomZ].usedInputPorts<2) // trial. if more than one input port is used then may be it is better to delete port e instead of all bottom AUT
						paths_deleted += delete_ALUT_stratix(x, y, bottomZ);
			//		else
			//		{
			//			paths_deleted += delete_ALUT_port_stratix(x, y, topZ, portE); // todo : check if this make sense
			//		}
				}
				else // bottom priority and port e have same priority, not sure what to do here
				{
					paths_deleted += delete_ALUT_port_stratix(x, y, topZ, portE); // todo : check if this make sense
				}
			}
		}
		else if (bottom_use_e) // bottom ALUT uses port e
		{
			if (top_priority < port_e_priority) // delete port e from bottom LUT
			{
				paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE);
			}
			else
			{
				if (port_e_priority < top_priority) // port e is more important so delete top ALUT
				{
		//			if (fpgaLogic[x][y][topZ].usedInputPorts<2) // trial. if more than one input port is used then may be it is better to delete port e instead of all bottom AUT
						paths_deleted += delete_ALUT_stratix(x, y, topZ);
			//		else
			//			paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE); // todo : check if this make sense

				}
				else // top priority and port e have same priority, not sure what to do here
				{
					paths_deleted += delete_ALUT_port_stratix(x, y, bottomZ, portE); // todo : check if this make sense
				}
			}
		}
		else
		{
			std::cout << std::endl << "SOme thing wrong at port e deletion tak care man" << std::endl;
		}
	}
	return paths_deleted;
}

#endif