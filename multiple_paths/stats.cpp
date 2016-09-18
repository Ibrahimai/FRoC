#include "globalVar.h"

void count_cascaded_paths()
{

	int total = 0;
	int i, j, k, l;
	for (i = 1; i < (int)paths.size(); i++) // loop over all paths
	{
		for (j = 0; j < (int)fpgaLogic[paths[i][0].x][paths[i][0].y][paths[i][0].z].nodes.size(); j++)
		{
			if (fpgaLogic[paths[i][0].x][paths[i][0].y][paths[i][0].z].nodes[j].node != 0)
			{
				//	std::cout << "7assal " << std::endl;
				total++;
			}
		}
	}

	int totalCascadedFF = 0;
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				if (k%LUTFreq == 0) // lut, then continue
					continue;

				bool source = false;
				bool sink = false;
				for (l = 0; l < fpgaLogic[i][j][k].nodes.size(); l++)
				{
					if (paths[fpgaLogic[i][j][k].nodes[l].path][0].deleted) // if this path is deleted, then dont bother accounting for it
						continue;

					if (fpgaLogic[i][j][k].nodes[l].node == 0)
					{
						source = true;
					}
					else
					{
						sink = true;
						assert(fpgaLogic[i][j][k].nodes[l].node == paths[fpgaLogic[i][j][k].nodes[l].path].size() - 1); // make sure that this is really the sink of the corresponding path.
					}
					if (source && sink)
					{
						totalCascadedFF++;
						break;
					}

				}


			}
		}
	}



	std::cout << "total numer of cascaded paths: " << total << std::endl;
	std::cout << "total numer of cascaded FFs: " << totalCascadedFF << std::endl;
}


void check_LE_outputs()
{

	int total = 0;
	int totalIn = 0;
	int all = 0;
	int i, j, k;
	int totalSource = 0;
	int totalSink = 0;
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				if (k%LUTFreq != 0) // source or sink registers
				{
					if (fpgaLogic[i][j][k].utilization == 0) // not used
						continue;
					if (fpgaLogic[i][j][k].nodes[0].node == 0) // source
					{
						assert(fpgaLogic[i][j][k].nodes.size()>0);
						totalSource++;
					}
					else // sink
					{
						assert(fpgaLogic[i][j][k].nodes.size()>0);
						// this must be the last node in the path so check that
						assert(fpgaLogic[i][j][k].nodes[0].node == paths[fpgaLogic[i][j][k].nodes[0].path].size() - 1);
						totalSink++;
					}
				}
				if (fpgaLogic[i][j][k].usedOutputPorts == 2)
				{
					total++;
					all++;
				}
				else if (fpgaLogic[i][j][k].usedOutputPorts == 1)
				{
					all++;
				}
				if (fpgaLogic[i][j][k].usedInputPorts>2)
					totalIn++;
			}
		}
	}
	std::cout << "total LUTs used " << all << std::endl;
	std::cout << "total sources used " << totalSource << std::endl;
	std::cout << "total sinks used " << totalSink << std::endl;

}


void check_critical_path_from_Input_toCout() // checks how many LUTS have a critical connection between an input different to Cin and Cout
{
	int total = 0;
	int i, j, k;
	for (i = 1; i <(int)paths.size(); i++) // loop over all paths
	{
		for (j = 0; j < (int)paths[i].size(); j++)
		{
			if (paths[i][j].portIn != Cin&&paths[i][j].portOut == Cout)
			{
				if (fpgaLogic[paths[i][j].x][paths[i][j].y][paths[i][j].z].usedInputPorts>1)
				{

					//			std::cout << i << " " << j << std::endl;
					total++;
				}
			}
		}
	}

	total = 0;
	int total2 = 0;
	int kk;
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
						if (!fpgaLogic[i][j][k].inputPorts[Cin]) // no cin
						{
							//			std::cout << "sava3" << std::endl;
							total++;
							for (kk = 0; kk < (int)fpgaLogic[i][j][k].nodes.size(); kk++)
							{
								if (fpgaLogic[paths[fpgaLogic[i][j][k].nodes[kk].path][fpgaLogic[i][j][k].nodes[kk].node + 1].x][paths[fpgaLogic[i][j][k].nodes[kk].path][fpgaLogic[i][j][k].nodes[kk].node + 1].y][paths[fpgaLogic[i][j][k].nodes[kk].path][fpgaLogic[i][j][k].nodes[kk].node + 1].z].usedInputPorts>1)
								{
									total2++;
								}
							}
						}
					}
				}
			}
		}
	}
	//	std::cout << total << std::endl;
}


// check timing edges counting ic and cell delay different timing edges
int check_number_of_timing_edges_more()
{
	int i, j, k;
	int sX, sY, sZ, sP, dX, dY, dZ, dP; // source port is the output port of a node, destination port is the output port of the destination
	int pIn;
	std::string tempKey;
	std::map<std::string, double> timingEdgeSlack;
	int bestPath, tempPath, tempNode;
	for (i = 1; i < (int)paths.size(); i++) // loop across all paths
	{
		for (j = 0; j < (int)paths[i].size() - 1; j++) // loop across all nodes in each path
		{
			// check if deleted or not
			if (paths[i][0].deleted)
				continue;
			// get source and destination
			// fist source
			sX = paths[i][j].x;
			sY = paths[i][j].y;
			sZ = paths[i][j].z;
			sP = paths[i][j].portOut;
			// then destination
			dX = paths[i][j + 1].x;
			dY = paths[i][j + 1].y;
			dZ = paths[i][j + 1].z;
			dP = paths[i][j + 1].portIn;

			// check if this edge is already considered 
			tempKey = "ICsX" + std::to_string(sX) + "sY" + std::to_string(sY) + "sZ" + std::to_string(sZ) + "sP" + std::to_string(sP) + "dX" + std::to_string(dX) + "dY" + std::to_string(dY) + "dZ" + std::to_string(dZ) + "dP" + std::to_string(dP);
			auto iter = timingEdgeSlack.find(tempKey);
			if (iter == timingEdgeSlack.end()) // was not found, so add it to the map with the right slack
			{
				// to get the righ slack we will loop across all nodes using the destination cell and check the worst slack that uses the same port in and same port out
				pIn = paths[i][j + 1].portIn;
				bestPath = paths.size();
				for (k = 0; k < (int)fpgaLogic[dX][dY][dZ].nodes.size(); k++)
				{
					tempPath = fpgaLogic[dX][dY][dZ].nodes[k].path;
					tempNode = fpgaLogic[dX][dY][dZ].nodes[k].node;
					// if path is deleted ignore it man
					if (paths[tempPath][0].deleted)
						continue;
					if (paths[tempPath][tempNode].portIn == pIn) // uses the fact that the first path found is the one with lowest slack
					{
						bestPath = tempPath;
						break;
					}
				}

				timingEdgeSlack.insert(std::pair<std::string, double>(tempKey, pathSlack[bestPath]));
			}
		}
	}


	for (i = 1; i < (int)paths.size(); i++) // loop across all paths
	{
		for (j = 0; j <(int)paths[i].size(); j++) // loop across all nodes in each path
		{
			// check if deleted or not
			if (paths[i][0].deleted)
				continue;
			// get source and destination
			// fist source
			sX = paths[i][j].x;
			sY = paths[i][j].y;
			sZ = paths[i][j].z;
			sP = paths[i][j].portIn;
			// then destination
			dP = paths[i][j].portOut;

			// check if this edge is already considered 
			tempKey = "CELLsX" + std::to_string(sX) + "sY" + std::to_string(sY) + "sZ" + std::to_string(sZ) + "sP" + std::to_string(sP) + "dP" + std::to_string(dP);
			auto iter = timingEdgeSlack.find(tempKey);
			if (iter == timingEdgeSlack.end()) // was not found, so add it to the map with the right slack
			{
				// to get the righ slack we will loop across all nodes using the destination cell and check the worst slack that uses the same port in and same port out
				bestPath = paths.size();
				for (k = 0; k < (int)fpgaLogic[sX][sY][sZ].nodes.size(); k++)
				{
					tempPath = fpgaLogic[sX][sY][sZ].nodes[k].path;
					tempNode = fpgaLogic[sX][sY][sZ].nodes[k].node;
					// if path is deleted ignore it man
					if (paths[tempPath][0].deleted)
						continue;
					if (paths[tempPath][tempNode].portIn == sP && paths[tempPath][tempNode].portOut == dP) // uses the fact that the first path found is the one with lowest slack
					{
						bestPath = tempPath;
						break;
					}
				}

				timingEdgeSlack.insert(std::pair<std::string, double>(tempKey, pathSlack[bestPath]));
			}
		}
	}
	// print slack coverage
	std::ofstream edgeCoverageFile;
	edgeCoverageFile.open("edgeCoverage.txt");

	for (auto iter = timingEdgeSlack.begin(); iter != timingEdgeSlack.end(); iter++)
		edgeCoverageFile << iter->second << std::endl;

	edgeCoverageFile.close();

	return timingEdgeSlack.size();
}

void calc_stats()
{
	int totalTimingEdges;
	///////////////////////////// stat functions
	count_cascaded_paths();
	check_LE_outputs();
	check_critical_path_from_Input_toCout();
	totalTimingEdges = check_number_of_timing_edges_more();
	std::cout << "total number of edges " << totalTimingEdges << std::endl;

}


void print_path_coverage_to_file()
{
	std::ofstream pathCoverageFile;
	pathCoverageFile.open("pathCoverage.txt");
	int i;
	for (i = 1; i < (int)paths.size(); i++)
	{
		if (paths[i][0].deleted)
			continue;
		pathCoverageFile << pathSlack[i] << std::endl;
	}
	pathCoverageFile.close();
}

#ifdef StratixV
void LUT_inputs_stat()
{

	std::vector < std::vector<Path_logic_component> > LUTsInputs;
	std::vector<Path_logic_component>  FFsInputs;

	LUTsInputs.resize(LUTinputSize);

	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ; k++)
			{

				if (fpgaLogic[i][j][k].usedInputPorts>0)
				{
					if (k%LUTFreq != 0)
					{
						FFsInputs.push_back(Path_logic_component(fpgaLogic[i][j][k].owner.path, fpgaLogic[i][j][k].owner.node));
						continue;
					}
					assert(k%LUTFreq == 0);
					LUTsInputs[fpgaLogic[i][j][k].usedInputPorts - 1].push_back(Path_logic_component(fpgaLogic[i][j][k].owner.path, fpgaLogic[i][j][k].owner.node));
				}
			}
		}
	}
	int totalLarger = 0;
	int fullALMs = 0;
	int fullALMwithOneE = 0;
	int fullALMwithTwoE = 0;
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ/6; k++)
			{

				if (alms[i][j][k].numOfInputs>4)
					totalLarger++;

				// check how many alms use port E and have two different outputs (both top and bottom LUTS are used)
				if (fpgaLogic[i][j][k*ALUTtoALM].utilization>0 && fpgaLogic[i][j][k*ALUTtoALM + LUTFreq].utilization>0)
				{
					fullALMs++;
					if (fpgaLogic[i][j][k*ALUTtoALM].inputPorts[portE] || fpgaLogic[i][j][k*ALUTtoALM + LUTFreq].inputPorts[portE])
					{
						if (fpgaLogic[i][j][k*ALUTtoALM].inputPorts[portE] && fpgaLogic[i][j][k*ALUTtoALM + LUTFreq].inputPorts[portE])
							fullALMwithTwoE++;
						else
							fullALMwithOneE++;
					}
				}
			}
		}
	}

	return;
}

#endif 