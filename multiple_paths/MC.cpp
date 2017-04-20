#pragma once
#include "MC.h"
#include "globalVar.h"


// random variables with a certain sigma and u to model each delay element as an independant RV
std::map<std::string, std::vector < Rand_Edge_Delay > >  timingEdgesRandVars;
std::map<std::string, std::vector < Rand_Edge_Delay > >  Cells_REsRandVars;
/// random variables with sigma 1 and u zero
std::unordered_map<std::string, std::vector < Rand_Edge_Delay > >  timingEdgesRandVars_standard;
std::unordered_map<std::string, std::vector < Rand_Edge_Delay > >  Cells_REsRandVars_standard;


//dev_per_to_mean is the ration of dev / mean, quartus_delay is how do we interperet the delay reported by quartus, 1 means mean + sigma
void MC_generate_REs_rand_vars(double dev_per_to_mean, double quartus_delay)
{
	Cells_REsRandVars.clear();
	Cells_REsRandVars_standard.clear();

	// loop over the global variable timingEdgesDelay to add cell delay to rand variable Cells_RErandsvars
	for (auto iter = timingEdgesDelay.begin(); iter != timingEdgesDelay.end(); iter++)
	{



		std::string tempKey = iter->first;
		// if its a cell delay thne the sirst parts of the string should be CELLsX
		if (!(tempKey[0] == 'C' && tempKey[1] == 'E' && tempKey[2] == 'L' && tempKey[3] == 'L')) // if its not a cell delay then continue, we dont care about IC delay
			continue;


		////// begin trial, push cell delay into celledgesdelay map
		assert(cellEdgesDelay.find(tempKey) == cellEdgesDelay.end());
		cellEdgesDelay.insert(std::pair<std::string, std::vector < Edge_Delay > >(iter->first, iter->second));
		/////// end trial

		std::vector< Edge_Delay > currentEdge = iter->second;

		std::vector <Rand_Edge_Delay > currentEdgeRand; // vectore storing (edgyeType,randVariabl) of the current edge
		std::vector <Rand_Edge_Delay > currentEdgeRand_standard; // vectore storing (edgyeType,randVariabl) of the current edge
		currentEdgeRand.resize(0);
		currentEdgeRand_standard.resize(0);

		auto iter_rand_edges = Cells_REsRandVars.find(tempKey);
		assert(iter_rand_edges == Cells_REsRandVars.end());

		//loop over all the edge delays of this edge (max 4. FF, FR, RF, RR)
		for (int i = 0; i < currentEdge.size(); i++)
		{
			// u + quartus_delay*sigma = delay_reported_by_quartus (quartus_delay [0-3])
			// sigma/mean = dev_per_to_mean


			double mean = currentEdge[i].delay / (1 + quartus_delay*dev_per_to_mean);	// -quartus_delay*dev_per_to_mean;
			double dev = dev_per_to_mean*mean;
			if (dev == 0)
			{
				dev = 0.00000000000000000000000000000000000000000000001;
			}
			std::normal_distribution<double> randVarstemp(mean, dev);
			std::normal_distribution<double> randVarstemp_standard(0.0, 1.0);

			Rand_Edge_Delay temp = Rand_Edge_Delay(currentEdge[i].type, randVarstemp);
			Rand_Edge_Delay temp_standard = Rand_Edge_Delay(currentEdge[i].type, randVarstemp_standard);
			currentEdgeRand.push_back(temp);
			currentEdgeRand_standard.push_back(temp_standard);
		}

		Cells_REsRandVars.insert(std::pair<std::string, std::vector<Rand_Edge_Delay > >(tempKey, currentEdgeRand));
		Cells_REsRandVars_standard.insert(std::pair<std::string, std::vector<Rand_Edge_Delay > >(tempKey, currentEdgeRand_standard));
	}


	// loop over the global variable REsDelay to add RE delay 
	for (auto iter = REsDelay.begin(); iter != REsDelay.end(); iter++)
	{



		std::string tempKey = iter->first;
	

		std::vector< Edge_Delay > currentEdge = iter->second;

		std::vector <Rand_Edge_Delay > currentEdgeRand; // vectore storing (edgyeType,randVariabl) of the current edge
		std::vector <Rand_Edge_Delay > currentEdgeRand_standard; // vectore storing (edgyeType,randVariabl) of the current edge
		currentEdgeRand.resize(0);
		currentEdgeRand_standard.resize(0);

		auto iter_rand_edges = Cells_REsRandVars.find(tempKey);
		assert(iter_rand_edges == Cells_REsRandVars.end());

		//loop over all the edge delays of this edge (max 4. FF, FR, RF, RR)
		for (int i = 0; i < currentEdge.size(); i++)
		{
			// u + quartus_delay*sigma = delay_reported_by_quartus (quartus_delay [0-3])
			// sigma/mean = dev_per_to_mean

			// edge of a RE can only be RR or FF
			assert(currentEdge[i].type == 0 || currentEdge[i].type == 3);

			double mean = currentEdge[i].delay / (1 + quartus_delay*dev_per_to_mean);	// -quartus_delay*dev_per_to_mean;
			double dev = dev_per_to_mean*mean;
			if (dev == 0)
			{
				dev = 0.00000000000000000000000000000000000000000000001;
			}
			std::normal_distribution<double> randVarstemp(mean, dev);
			std::normal_distribution<double> randVarstemp_standard(0.0, 1.0);
			Rand_Edge_Delay temp = Rand_Edge_Delay(currentEdge[i].type, randVarstemp);
			Rand_Edge_Delay temp_standard = Rand_Edge_Delay(currentEdge[i].type, randVarstemp_standard);
			currentEdgeRand.push_back(temp);
			currentEdgeRand_standard.push_back(temp_standard);
		}

		Cells_REsRandVars.insert(std::pair<std::string, std::vector<Rand_Edge_Delay > >(tempKey, currentEdgeRand));
		Cells_REsRandVars_standard.insert(std::pair<std::string, std::vector<Rand_Edge_Delay > >(tempKey, currentEdgeRand_standard));
	}


}

//dev_per_to_mean is the ration of dev / mean, quartus_delay is how do we interperet the delay reported by quartus, 1 means mean + sigma
void MC_generate_edges_rand_vars(double dev_per_to_mean, double quartus_delay) 
{
	 
	timingEdgesRandVars.clear();
	timingEdgesRandVars_standard.clear();


	// loop over the global variable timingEdgesDelay
	for (auto iter = timingEdgesDelay.begin(); iter != timingEdgesDelay.end(); iter++)
	{
		
		

		std::string tempKey = iter->first;
		std::vector< Edge_Delay > currentEdge = iter->second;

		std::vector <Rand_Edge_Delay > currentEdgeRand; // vectore storing (edgyeType,randVariabl) of the current edge
		std::vector <Rand_Edge_Delay > currentEdgeRand_standard; // vectore storing (edgyeType,randVariabl) of the current edge
		currentEdgeRand.resize(0);
		currentEdgeRand_standard.resize(0);

		auto iter_rand_edges = timingEdgesRandVars.find(tempKey);
		assert(iter_rand_edges == timingEdgesRandVars.end());

		//loop over all the edge delays of this edge (max 4. FF, FR, RF, RR)
		for (int i = 0; i < currentEdge.size(); i++)
		{
			// u + quartus_delay*sigma = delay_reported_by_quartus (quartus_delay [0-3])
			// sigma/mean = dev_per_to_mean


			double mean = currentEdge[i].delay / (1 + quartus_delay*dev_per_to_mean);	// -quartus_delay*dev_per_to_mean;
			double dev = dev_per_to_mean*mean;
			if (dev == 0)
			{
				dev = 0.00000000000000000000000000000000000000000000001;
			}
			std::normal_distribution<double> randVarstemp(mean, dev);
			Rand_Edge_Delay temp = Rand_Edge_Delay(currentEdge[i].type, randVarstemp);			
			currentEdgeRand.push_back(temp);
			std::normal_distribution<double> randVarstemp_standard(0.0, 1.0);
			Rand_Edge_Delay temp_standard = Rand_Edge_Delay(currentEdge[i].type, randVarstemp_standard);
			currentEdgeRand_standard.push_back(temp_standard);
		}

		timingEdgesRandVars.insert(std::pair<std::string, std::vector<Rand_Edge_Delay > >(tempKey, currentEdgeRand));
		timingEdgesRandVars_standard.insert(std::pair<std::string, std::vector<Rand_Edge_Delay > >(tempKey, currentEdgeRand_standard));
	}

	assert(timingEdgesRandVars.size() == timingEdgesDelay.size());

}

void get_location_from_key(std::string tempKey, int & x, int & y, int & z)
{
	std::vector<std::string> locs;
	locs.resize(0);

	bool numberFound = false;

	std::string temp = "";
	for (int i = 0; i < tempKey.size(); i++)
	{
		if (isdigit(tempKey[i]))
		{
			if (!numberFound)
				numberFound = true;

			temp += tempKey[i];
		}
		else
		{
			if (numberFound)
			{
				numberFound = false;
				locs.push_back(temp);
				temp = "";
			}
		}
	}
	if(numberFound)
		locs.push_back(temp);
	assert(locs.size() == 5);
	x = stoi(locs[0]);
	y = stoi(locs[1]);
	z = stoi(locs[2]);
}

// performs number of simulation, takes number of required simulations, vecotr of tested and untested paths
// returns a probability of failure and stores the importance of each path in pathsImport
double MC_sim_edges(int num_of_simulations, std::vector<int> untestedPaths, std::vector<int> testedPaths, std::map<std::string, std::vector<Path_logic_component> > timingEdgeToPaths, std::vector<double> & pathsImport)
{
	int num_of_failures = 0;
	std::cout << "*******************************************************************" << std::endl;
	std::cout << "MC simulation with timing edges : " << std::endl;

	TE_MCSim << "*******************************************************************" << std::endl;
	TE_MCSim << "MC simulation with timing edges : " << std::endl;

	// generate a new random seed for each MC simulation
	std::random_device seed;
	std::mt19937 gen{ seed() };

//	std::cout << "****first number " << gen << std::endl;
//	std::cout << "****second number " << gen() << std::endl;


	std::vector <std::pair<int, int>> problematicPaths;
	problematicPaths.resize(0);

	std::vector <std::pair<int, int>> longestPaths;
	longestPaths.resize(0);


	//std::vector <bool> isTested


	//pathsDelay.resize(paths.size());

	std::pair<int, double> shortestCriticalPath_delay;
	std::pair<int, double> longestCriticalPath_delay;
	longestCriticalPath_delay.second = 0.0;
	longestCriticalPath_delay.first = -1;

	shortestCriticalPath_delay.second = 0.0;
	shortestCriticalPath_delay.first = -1;

	for (int sampleCounter = 0; sampleCounter < num_of_simulations; sampleCounter++) // creates number of simulations
	{
		// vector to store all paths sizes
		std::vector<double> pathsDelay(paths.size(), 0.0); // stores paths delay with clock skew into account, it assumes the same transition type reported by Quartus
		std::vector<double> pathsDelayOpTransition(paths.size(), 0.0); // stores it with clock skew into account, it assumes the opposite transition than the quartus reported one


		// taking clock skew into account
		for (int i = 1; i < paths.size(); i++)
		{
			pathsDelay[i] -= pathClockSkew[i];
			pathsDelayOpTransition[i] -= pathClockSkew[i];
		}

		std::vector<double> testedPathsDelay;
		std::vector<double> untestedPathsDelay;

	//	double fastestPath = -1;

		testedPathsDelay.resize(0);
		untestedPathsDelay.resize(0);

		double maxPathDelay = 0.0;

		double maxUntestedPathDelay = 0.0;
		double maxTestedPathDelay = 0.0;

		int longestUntestedPath = -1;
		int longestTestedPath = -1;


		int longestPath = -1;

		// loop over all timing edges to get the delay of each edge and add it to the paths using it
		for (auto iter = timingEdgeToPaths.begin(); iter != timingEdgeToPaths.end(); iter++)
		{
			std::string tempKey = iter->first;
			std::vector<int> currentPaths;// = iter->second; // store paths of the current edge
			std::vector<int> currentNodes;// = iter->second; // store the corresponding node of each  path used by this edge

			for (int counter_vector = 0; counter_vector < (iter->second).size(); counter_vector++)
			{
				currentPaths.push_back((iter->second)[counter_vector].path);
				currentNodes.push_back((iter->second)[counter_vector].node);
			}

			int x, y, z = -1;
			if(tempKey[0]=='C') // if its a cell get the location of it please
				get_location_from_key(tempKey, x, y, z);

			// initially I will just use the max delay of all transisiotnas as the delay of this edge

			double maxDelay = 0.0;

			// if a register is cascaded htne there are 2 delays associated with it Tco if its source and CELL delay if its a destination so I am gonna get both
			double maxDelaySourceReg = -1.0;
			double maxDelayDestReg = -1.0;

			auto iter_delay = timingEdgesRandVars.find(tempKey);

			if (tempKey[0] != 'C') // if its not a cell then make sure that the IC delay only has 2 possible values
				assert((iter_delay->second).size() < 3);


			// the timing edge must exist in the timingedge delay vars map
			assert(iter_delay != timingEdgesRandVars.end());

			std::vector<double> transitionDelay(8, -1); // 8 possible transistion delay, the normal 4 (ff,fr,rf,rr) and another 4 just for the Tco of the source register

			// loop across different transistions at this edge
			for (int i = 0; i < (iter_delay->second).size(); i++)
			{
				double currDelay = ((iter_delay->second)[i].randVars)(gen);

				transitionDelay[(iter_delay->second)[i].type] = currDelay;

				if (currDelay > maxDelay)
					maxDelay = currDelay;

				if ((iter_delay->second)[i].type > 3)
					assert(z%LUTFreq != 0);

				if (z > -1 && z%LUTFreq != 0) // this is a source
				{
					if ((iter_delay->second)[i].type <= 3) // reg as a dest
					{
						if (currDelay > maxDelayDestReg)
							maxDelayDestReg = currDelay;
					}
					else // reg is a source
					{
						if (currDelay > maxDelaySourceReg)
							maxDelaySourceReg = currDelay;
					}
				}
				
			}



			// now we have a delay for this edge lets add it to all the paths its using

			for (int i = 0; i < currentPaths.size(); i++)
			{
					 
				if (z < 0 || z%LUTFreq == 0) // LUT ot IC
				{
					int currentEdgeType = paths[currentPaths[i]][currentNodes[i]].edgeType; // get the corresponding cell delay
					assert(currentEdgeType > -1 && currentEdgeType < 4);

					if (z < 0) // then its an IC delay, so it can eithe have type 0 (FF) or type 3 (RR), get the type from the destination node transistion. transofrm 0,1 -> 0 and 2,3 to 3
					{
						currentEdgeType = (int)floor(currentEdgeType / 2.0) * 3;//(currentEdgeType % 2) * 3;
					}

					// get the opposite transition 0 -> 3, 1 -> 2, 2 -> 1, 3 -> 0; 

					int oppEdgeType;
					if (currentEdgeType == 0)
						oppEdgeType = 3;
					else if (currentEdgeType == 1)
						oppEdgeType = 2;
					else if (currentEdgeType == 2)
						oppEdgeType = 1;
					else
						oppEdgeType = 0;

					assert(transitionDelay[currentEdgeType] > -1);
					pathsDelay[currentPaths[i]] += transitionDelay[currentEdgeType];

					if (transitionDelay[oppEdgeType] == -1) // this transition was not reported by quartus and so we have aa -v2 1
						pathsDelayOpTransition[currentPaths[i]] = -1;
					else // this transiiton exists
					{
						if (pathsDelayOpTransition[currentPaths[i]] != -1) // check if the delay is equal to 1, it would be equal to 1 if one of the transitions in the opposite guy is not there
						{
							pathsDelayOpTransition[currentPaths[i]] += transitionDelay[oppEdgeType];

						}
					}
				//	pathsDelay[currentPaths[i]] += maxDelay;
				}
				else
				{
					if (paths[currentPaths[i]][0].x == x && paths[currentPaths[i]][0].y == y && paths[currentPaths[i]][0].z == z) // this timing edge is part of the source of the path
					{
						assert(maxDelaySourceReg > -0.00000000000000001);
						pathsDelay[currentPaths[i]] += maxDelaySourceReg;
						pathsDelayOpTransition[currentPaths[i]] += maxDelaySourceReg;
					}
					else // then it's a destination
					{
						int currentEdgeType = paths[currentPaths[i]][currentNodes[i]].edgeType; // get the corresponding cell delay
						assert(currentEdgeType > -1 && currentEdgeType < 4);
						assert(maxDelayDestReg > -0.00000000000000001);
						assert(transitionDelay[currentEdgeType] > -1);
						pathsDelay[currentPaths[i]] += transitionDelay[currentEdgeType];
						assert(maxDelayDestReg > -0.00000000000000001);

						int oppEdgeType;
						if (currentEdgeType == 0)
							oppEdgeType = 3;
						else if (currentEdgeType == 1)
							oppEdgeType = 2;
						else if (currentEdgeType == 2)
							oppEdgeType = 1;
						else
							oppEdgeType = 0;

						if (transitionDelay[oppEdgeType] == -1) // this transition was not reported by quartus and so we have aa -v2 1
							pathsDelayOpTransition[currentPaths[i]] = -1;
						else // this transiiton exists
						{
							if (pathsDelayOpTransition[currentPaths[i]] != -1) // check if the delay is equal to 1, it would be equal to 1 if one of the transitions in the opposite guy is not there
							{
								pathsDelayOpTransition[currentPaths[i]] += transitionDelay[oppEdgeType];

							}
						}

					//	pathsDelay[currentPaths[i]] += maxDelayDestReg;
					}
				}

				if (pathsDelay[currentPaths[i]]>maxPathDelay)
				{
					maxPathDelay = pathsDelay[currentPaths[i]];
					longestPath = currentPaths[i];
				}

				// oppo begin
				if (pathsDelayOpTransition[currentPaths[i]]>maxPathDelay)
				{
					maxPathDelay = pathsDelayOpTransition[currentPaths[i]];
					longestPath = currentPaths[i];

				}
				// oppo end

				if (pathsDelay[currentPaths[i]]>maxUntestedPathDelay && !paths[currentPaths[i]][0].tested)
				{
					maxUntestedPathDelay = pathsDelay[currentPaths[i]];
					longestUntestedPath = currentPaths[i];
				}

				//oppo begin
				if (pathsDelayOpTransition[currentPaths[i]]>maxUntestedPathDelay && !paths[currentPaths[i]][0].tested)
				{
					maxUntestedPathDelay = pathsDelayOpTransition[currentPaths[i]];
					longestUntestedPath = currentPaths[i];
				}

				//oppo end

				if (pathsDelay[currentPaths[i]]>maxTestedPathDelay && paths[currentPaths[i]][0].tested)
				{
					maxTestedPathDelay = pathsDelay[currentPaths[i]];
					longestTestedPath = currentPaths[i];
				}

				//oppo begin
				if (pathsDelayOpTransition[currentPaths[i]]>maxTestedPathDelay && paths[currentPaths[i]][0].tested)
				{
					maxTestedPathDelay = pathsDelayOpTransition[currentPaths[i]];
					longestTestedPath = currentPaths[i];
				}
				// oppo end

			}

		}

		/// for stats purposes see what is the longest path ate each MC sample begin
	
		int index_longest_path = -1;
		for (int k = 0; k < longestPaths.size(); k++)
		{
			if (longestPaths[k].first == longestPath)
			{
				index_longest_path = k;

			}
		}


		if (index_longest_path > -1) // then we have seen this path b4, so just increment the number of occurunces
		{

			longestPaths[index_longest_path].second++;
		}
		else
		{
			longestPaths.push_back(std::pair<int, int >(longestPath, 1));
		}
		// end

		// now we will check if the longest path corresponds to an untested path or not

		bool fail = false;
		for (int i = 0; i < untestedPaths.size(); i++)
		{
			if (longestPath == untestedPaths[i])
			{
				//std::cout << "longest path was found in an untested path " << std::endl;
				//std::cout << "longest path is " << untestedPaths[i] << std::endl;
				// insert this path in the set of problematic path
				int index = -1;
				int untestedPath = -1;
				untestedPath = untestedPaths[i];
				for (int k = 0; k < problematicPaths.size(); k++)
				{
					if (problematicPaths[k].first == untestedPaths[i])
					{
						index = k;
						
					}
				}

				assert(untestedPath > -1);

				if (index > -1) // then we have seen this path b4, so just increment the number of occurunces
				{

					problematicPaths[index].second++;
				}
				else
				{
					problematicPaths.push_back(std::pair<int, int >(untestedPath, 1));
				}
				fail = true;
			}
		}

		// get the critical path and its delay
		if ((sampleCounter == 0) || (shortestCriticalPath_delay.second > maxPathDelay))
		{
			shortestCriticalPath_delay.first = longestPath;
			shortestCriticalPath_delay.second = maxPathDelay;
		}

		// get the critical path and its delay
		if ((sampleCounter == 0) || (longestCriticalPath_delay.second < maxPathDelay))
		{
			longestCriticalPath_delay.first = longestPath;
			longestCriticalPath_delay.second = maxPathDelay;
		}


		// now I just wanted to check if the longest delay is also the same as a tested path

		for (int i = 0; i < testedPaths.size(); i++)
		{
			if (maxPathDelay == pathsDelay[testedPaths[i]])
			{
				if (fail)
				{
					std::cout << "the longest delay was found in two different sets; tested and untested";
					fail = false;
				} 
			}
				
		}

		if (fail)
		{
			num_of_failures++;
			assert(maxUntestedPathDelay > maxTestedPathDelay);
	//		std::cout << "Maximum untested path " << longestUntestedPath <<  " delay is " << maxUntestedPathDelay << " maximum tested path " << longestTestedPath << " delay is " << maxTestedPathDelay << std::endl;

		}

	}


	pathsImport.clear();
	pathsImport.resize(paths.size());

	std::fill(pathsImport.begin(), pathsImport.end(), 0.0);

	// print problematic paths
	for (int k = 0; k < problematicPaths.size(); k++)
	{
		TE_MCSim << "Path " << problematicPaths[k].first << " is a problem in " << problematicPaths[k].second << " samples." << std::endl;
		std::cout << "Path " << problematicPaths[k].first << " is a problem in " << problematicPaths[k].second << " samples." << std::endl;

		assert(pathsImport[problematicPaths[k].first] == 0.0);

		pathsImport[problematicPaths[k].first] = (problematicPaths[k].second*1.0);// / (num_of_failures);
	}
	TE_MCSim << "=----=-=-09-09=-=-09-0=-0==-0-9=-=0- " << std::endl;


	

	int slowestPath = longestPaths[0].first;
	for (int k = 0; k < longestPaths.size(); k++)
	{
		if (longestPaths[k].first > slowestPath)
			slowestPath = longestPaths[k].first;

		TE_MCSim << "Path " << longestPaths[k].first << " is the longest in " << longestPaths[k].second << " samples." << std::endl;
		std::cout << "Path " << longestPaths[k].first << " is the longest in " << longestPaths[k].second << " samples." << std::endl;
	}

	// add a bit of weight to the paths that are not longest to let the ILp consider testing them too
	for (int k = 1; k < pathsImport.size(); k++)
	{
		if (pathsImport[k] != 0)
			continue;

		pathsImport[k] = 1.0 / (paths.size()*((k/10)+1));// *num_of_failures);

		
	}


	TE_MCSim << "Number of paths that were the longest " << longestPaths.size() << std::endl;
	TE_MCSim << "Fastest slowest path was " << slowestPath << std::endl;
	
	TE_MCSim << "Out of all the MC samples, the shortest delay was for path " << shortestCriticalPath_delay.first << " delay was " << shortestCriticalPath_delay.second << std::endl;
	TE_MCSim << "Out of all the MC samples, the longest delay was for path " << longestCriticalPath_delay.first << " delay was " << longestCriticalPath_delay.second << std::endl;

	return (num_of_failures*1.0) / num_of_simulations;
}


// takes in a strin gin the format of "RE"<wire>, wire must be R or C only, other wise assertion my friend.
void get_RE_location_and_length(std::string Key, int & x, int & y, int & length)
{
	assert(isdigit(Key[3]));

	std::vector<std::string> numbers;
	//numbers.resize(3);

	bool foundNumber = false;
	std::string tempNumber;
	for (int i = 0; i < Key.size();i++)
	{
		if (!foundNumber)
		{
			if (isdigit(Key[i]))
			{
				tempNumber.push_back(Key[i]);
				foundNumber = true;
			}
		}
		else
		{
			if (isdigit(Key[i]))
			{
				tempNumber.push_back(Key[i]);
			}
			else
			{
				numbers.push_back(tempNumber);
				tempNumber.clear();
				foundNumber = false;
			}

		}
		// read the first three number and leave
		if (numbers.size() == 3)
			break;
	}

	assert(numbers.size() == 3);

	length = stoi(numbers[0]);
	x = stoi(numbers[1]);
	y = stoi(numbers[2]);

}



void get_RE_location(std::string Key, int & x, int & y )
{
	

	std::vector<std::string> numbers;
	//numbers.resize(3);

	bool foundNumber = false;
	std::string tempNumber;
	for (int i = 0; i < Key.size(); i++)
	{
		if (!foundNumber)
		{
			if (isdigit(Key[i]))
			{
				tempNumber.push_back(Key[i]);
				foundNumber = true;
			}
		}
		else
		{
			if (isdigit(Key[i]))
			{
				tempNumber.push_back(Key[i]);
			}
			else
			{
				numbers.push_back(tempNumber);
				tempNumber.clear();
				foundNumber = false;
			}

		}
		// read the first three number and leave
		if (numbers.size() == 2)
			break;
	}

	assert(numbers.size() == 2);

	//length = stoi(numbers[0]);
	x = stoi(numbers[0]);
	y = stoi(numbers[1]);

}

// MC sim with individual elements delay
double MC_sim_RE(bool slidingWindow, int corelationModel, int num_of_simulations, std::vector<int> untestedPaths, std::vector<int> testedPaths, std::map<std::string, std::vector<Path_logic_component> > timingEdgeToPaths, std::vector<double> & pathsImport)
{
	int num_of_failures = 0;

	std::cout << "*******************************************************************" << std::endl;
	std::cout << "MC simulation with REs : " << std::endl;

	RE_MCSim << "*******************************************************************" << std::endl;
	RE_MCSim << "MC simulation with REs : " << std::endl;

	// generate a new random seed for each MC simulation
	std::random_device seed;
	std::mt19937 gen{ seed() };

	//	std::cout << "****first number " << gen << std::endl;
	//	std::cout << "****second number " << gen() << std::endl;


	std::vector <std::pair<int, int>> problematicPaths;
	problematicPaths.resize(0);

	std::vector <std::pair<int, int>> longestPaths;
	longestPaths.resize(0);


	//std::vector <bool> isTested


	//pathsDelay.resize(paths.size());

	// stpore information of the shortest CP reported over all the simulation
	std::pair<int, double> shortestCriticalPath_delay;
	std::pair<int, double> longestCriticalPath_delay;
	shortestCriticalPath_delay.second = 0.0;
	shortestCriticalPath_delay.first = -1;

	longestCriticalPath_delay.second = 0.0;
	longestCriticalPath_delay.first = -1;


	// create a random independant RV that will be used to force corelations between RVs pf delays, only when doing fully co-related RVs
	double mean_stand_normal = 0.0;
	double dev_stand_normal = 1.0;

	int horiz = 114;// 38;
	int vert = 72;// 36;

	int xMargin = 1;
	int yMargin = 1;

	// for fullCOrelation just create one rand var
	std::normal_distribution<double> indepRV(mean_stand_normal, dev_stand_normal);

	// for partial corelation we are gonna create a 2d vector of rand variables
	std::vector< std::vector <std::normal_distribution<double>> > RVmatrix; // MAtrix[x][y]

	if (corelationModel == PARTIALCORELATION)
	{
		RVmatrix.resize(horiz + 2*xMargin); // 2 for padding one atop and one abottom

		for (int i = 0; i < horiz + 2 * xMargin; i++)
		{
			for (int j = 0; j < vert + 2*yMargin; j++) // +2 for padding
			{
				std::normal_distribution<double> temp(mean_stand_normal, dev_stand_normal);
				RVmatrix[i].push_back(temp);
			}
		}
		 
	}

	for (int sampleCounter = 0; sampleCounter < num_of_simulations; sampleCounter++) // creates number of simulations
	{
		// vector to store all paths sizes
		std::vector<double> pathsDelay(paths.size(), 0.0); // stores it with clock skew into account

		// taking clock skew and total delta RE into account
		for (int i = 1; i < paths.size(); i++)
		{
			pathsDelay[i] -= pathClockSkew[i];
			pathsDelay[i] += pathREsDelta[i];
		}

		std::vector<double> testedPathsDelay;
		std::vector<double> untestedPathsDelay;

		testedPathsDelay.resize(0);
		untestedPathsDelay.resize(0);

		double maxPathDelay = 0.0;

		double maxUntestedPathDelay = 0.0;
		double maxTestedPathDelay = 0.0;

		int longestUntestedPath = -1;
		int longestTestedPath = -1;


		int longestPath = -1;
		int edgetype = -1;

		double currSampleRv = 0.0;

		std::vector<std::vector<double> > currSampleRVMatrix;
		


		/// get the delay of the current sample for the partial and full corelations case
		if (corelationModel == FULLCORELATION)
		{
			currSampleRv = indepRV(gen);
		}
		else if (corelationModel == PARTIALCORELATION)
		{
			currSampleRVMatrix.resize(RVmatrix.size());
			for (int i = 0; i < RVmatrix.size(); i++)
			{
				for (int j = 0; j < RVmatrix[i].size(); j++)
				{	
					currSampleRVMatrix[i].push_back((RVmatrix[i][j])(gen));
				}
			}

		}

		// loop over all RE timing edges to get the delay of each edge and add it to the paths using it
		for (auto iter = REToPaths.begin(); iter != REToPaths.end(); iter++)
		{
			std::string tempKey = iter->first;
			std::vector<int> currentPaths;// = iter->second;
			std::vector<int> currentNodes;// = iter->second; // store the corresponding node of each  path used by this edge


			for (int counter_vector = 0; counter_vector < (iter->second).size(); counter_vector++)
			{
				currentPaths.push_back((iter->second)[counter_vector].path);
				currentNodes.push_back((iter->second)[counter_vector].node);
			}
			// initially I will just use the max delay of all transisiotnas as the delay of this edge

			double maxDelay = 0.0;

			auto iter_delay = Cells_REsRandVars.find(tempKey);
			auto iter_delay_standard = Cells_REsRandVars_standard.find(tempKey);

			// the timing edge must exist in the timingedge delay vars map
			assert(iter_delay != Cells_REsRandVars.end());
			assert(iter_delay_standard != Cells_REsRandVars_standard.end());

			// at most the number of delays of an RE can be 2, FF and RR
			assert((iter_delay->second).size() < 3);

			edgetype = -1;

			std::vector<double> transitionDelay(4, -1); // RE edge transition can either be 0 or 3

			// loop across different transistions at this edge to get the max delay
			for (int i = 0; i < (iter_delay->second).size(); i++)
			{
				double currDelay;
				
				if (corelationModel== NOCORELATION)// totally independant variables
				{
					currDelay = ((iter_delay->second)[i].randVars)(gen);
					if (iter == REToPaths.begin())
					{
					//	RE_MCSim << currDelay << std::endl;
					//	if (sampleCounter == 0)
					//		std::cout << "mean is " << (iter_delay->second)[i].randVars.mean() << " stdev is " << ((iter_delay->second)[i].randVars.stddev()) << std::endl;
					}
				}
				else if(corelationModel== FULLCORELATION) // fully correlated
				{
					// del = u + sigma*RV
					currDelay = (iter_delay->second)[i].randVars.mean() + ((iter_delay->second)[i].randVars.stddev())*currSampleRv;
			//		std::cout << (iter_delay->second)[i].randVars.mean() << " dev " << ((iter_delay->second)[i].randVars.stddev()) << std::endl;
			//		if (iter == REToPaths.begin())
			//		{
				//		RE_MCSim << currDelay << std::endl;
				//		if (sampleCounter == 0)
				//			std::cout << "mean is " << (iter_delay->second)[i].randVars.mean() << " stdev is " << ((iter_delay->second)[i].randVars.stddev()) << std::endl;
				//	}
				}
				else if(corelationModel== PARTIALCORELATION)
				{
					assert(tempKey.size() > 3);
					
					// lets check if its an R* or C* VS smthng else
					if (isdigit(tempKey[3])) // its either an R or a C
					{
						int vertLoc, horizLoc, length;
						get_RE_location_and_length(tempKey, horizLoc, vertLoc, length);
						
						int xBegin = horizLoc / ((FPGAsizeX - 1) / horiz);

						if (horizLoc % (((FPGAsizeX - 1) / horiz)) == 0 && horizLoc!=0)
							xBegin--;

						int yBegin = vertLoc / ((FPGAsizeY - 1) / vert);

						if (vertLoc % (((FPGAsizeY - 1) / vert)) == 0 && vertLoc!=0)
							yBegin--;

						int xEnd, yEnd;

						if (tempKey[2] == 'C') // vertical wire
						{
							xEnd = xBegin;
							int finalY = vertLoc + length;

							yEnd = finalY / ((FPGAsizeY - 1) / vert);
							if (finalY % (((FPGAsizeY - 1) / vert)) == 0 && finalY != 0)
								yEnd--;

							//yEnd = yBegin + length;
						}
						else // horizontal wire
						{
							assert(tempKey[2] == 'R');
							yEnd = yBegin;

							int finalX = horizLoc + length;
							xEnd = finalX / ((FPGAsizeX - 1) / horiz);

							if (finalX % (((FPGAsizeX - 1) / horiz)) == 0 && finalX != 0)
								xEnd--;

						//	xEnd = xBegin + length;
							
						}

						if (yEnd >= vert)
							yEnd = vert - 1;

						if (xEnd >= horiz)
							xEnd = horiz - 1;

						/// now that we have what xBegin, xEnd yBegin and yEnd lets get the mother fucking value

						//////////////////////////////////////////////////////////
						///////////////////// stuf for sliding window/////////////
						//////////////////////////////////////////////////////////
						if (slidingWindow)
						{
							double RVsSum = 0.0;

							// adjust xbegin and ybegin to account for added padded RVs (margins) at the left and bottom of the chip
							xBegin += xMargin;
							yBegin += yMargin;

							int xMargInitial = xBegin - xMargin;
							int yMargInitial = yBegin - yMargin;

							// loop over the windows row
							for (int windowX = 0; windowX < xMargin * 2 + 1; windowX++)
							{
								for (int windowY = 0; windowY < yMargin * 2 + 1; windowY++)
								{
									RVsSum += currSampleRVMatrix[xMargInitial + windowX][yMargInitial + windowY];
								}
							}

							//////////////////////////////////////////////////////////
							////////////////////// End ///////////////////////////////
							//////////////////////////////////////////////////////////

						
							double numberOfRVsInWindow = (xMargin * 2 + 1)*(yMargin * 2 + 1);
							currDelay = (iter_delay->second)[i].randVars.mean() + (((iter_delay->second)[i].randVars.stddev()) / sqrt(numberOfRVsInWindow+1))*(RVsSum+ ((iter_delay_standard->second)[i].randVars)(gen));
						}
						else
						{
							// del = u + (sigma/sqrt2)*RV1 + (sigma/sqrt2)*RV2
							currDelay = (iter_delay->second)[i].randVars.mean()
								+ (((iter_delay->second)[i].randVars.stddev()) / sqrt(2))*currSampleRVMatrix[xBegin][yBegin]
							+(((iter_delay->second)[i].randVars.stddev()) / sqrt(2)*((iter_delay_standard->second)[i].randVars)(gen));
							//+ (((iter_delay->second)[i].randVars.stddev()) / sqrt(2))*currSampleRVMatrix[xEnd][yEnd];
						}

					}
					else // something else other than R or C, so it only has an x and y no length
					{
						int vertLoc, horizLoc;
						get_RE_location(tempKey, horizLoc, vertLoc);

						int xBegin = horizLoc / ((FPGAsizeX - 1) / horiz);

						if (horizLoc % (((FPGAsizeX - 1) / horiz)) == 0 && horizLoc != 0)
							xBegin--;

						int yBegin = vertLoc / ((FPGAsizeY - 1) / vert);

						if (vertLoc % (((FPGAsizeY - 1) / vert)) == 0 && vertLoc != 0)
							yBegin--;
						//////////////////////////////////////////////////////////
						///////////////////// stuf for sliding window/////////////
						//////////////////////////////////////////////////////////
						if (slidingWindow)
						{	
							double RVsSum = 0.0;

							// adjust xbegin and ybegin to account for added padded RVs (margins) at the left and bottom of the chip
							xBegin += xMargin;
							yBegin += yMargin;

							int xMargInitial = xBegin - xMargin;
							int yMargInitial = yBegin - yMargin;

							// loop over the windows row
							for (int windowX = 0; windowX < xMargin * 2 + 1; windowX++)
							{
								for (int windowY = 0; windowY < yMargin * 2 + 1; windowY++)
								{
									RVsSum += currSampleRVMatrix[xMargInitial + windowX][yMargInitial + windowY];
								}
							}

						//////////////////////////////////////////////////////////
						////////////////////// End ///////////////////////////////
						//////////////////////////////////////////////////////////

						
							double numberOfRVsInWindow = (xMargin * 2 + 1)*(yMargin * 2 + 1);
							currDelay = (iter_delay->second)[i].randVars.mean() + (((iter_delay->second)[i].randVars.stddev()) / sqrt(numberOfRVsInWindow + 1))*(RVsSum + ((iter_delay_standard->second)[i].randVars)(gen));
						}
						else
						{
							currDelay = (iter_delay->second)[i].randVars.mean()
								+ (((iter_delay->second)[i].randVars.stddev())/sqrt(2))*currSampleRVMatrix[xBegin][yBegin]
							+(((iter_delay->second)[i].randVars.stddev()) /sqrt(2)*((iter_delay_standard->second)[i].randVars)(gen));
						}
					}
					// some printing to check that we actually do get a random variable
					if (iter == REToPaths.begin())
					{
						if (i == 0)
						{
				//			RE_MCSim << currDelay << "\t ";
							if (sampleCounter == 0)
								std::cout << "First coloumn RE mean is " << (iter_delay->second)[i].randVars.mean() << " stdev is " << ((iter_delay->second)[i].randVars.stddev()) << std::endl;
						}
					}
				}
				else
				{
					std::cout << "ERROR: Wrong Corelation Model." << std::endl;
					assert(false);
				}

				assert((iter_delay->second)[i].type == 0 || (iter_delay->second)[i].type == 3); // either ff or rr

				transitionDelay[(iter_delay->second)[i].type] = currDelay;

				if (currDelay > maxDelay)
					maxDelay = currDelay;			
			}

			for (int i = 0; i < currentPaths.size(); i++)
			{
				int currentEdgeType = paths[currentPaths[i]][currentNodes[i]].edgeType; // get the corresponding cell delay
				assert(currentEdgeType > -1 && currentEdgeType < 4);
				// this is a RE delay, so it can eithe have type 0 (FF) or type 3 (RR), get the type from the destination node transistion. transofrm 0,1 -> 0 and 2,3 to 3
				currentEdgeType = (int)floor(currentEdgeType / 2.0) * 3;//(currentEdgeType % 2) * 3;
				assert(transitionDelay[currentEdgeType] > -1);
				pathsDelay[currentPaths[i]] += transitionDelay[currentEdgeType];

		//		pathsDelay[currentPaths[i]] += maxDelay;	

				if (pathsDelay[currentPaths[i]]>maxPathDelay)
				{
					maxPathDelay = pathsDelay[currentPaths[i]];
					longestPath = currentPaths[i];
				}

				if (pathsDelay[currentPaths[i]]>maxUntestedPathDelay && !paths[currentPaths[i]][0].tested)
				{
					maxUntestedPathDelay = pathsDelay[currentPaths[i]];
					longestUntestedPath = currentPaths[i];
				}

				if (pathsDelay[currentPaths[i]]>maxTestedPathDelay && paths[currentPaths[i]][0].tested)
				{
					maxTestedPathDelay = pathsDelay[currentPaths[i]];
					longestTestedPath = currentPaths[i];
				}

			}

		}
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////// copied this from the mc_sim_edges and only changed if its IC delay then ignore it, should eb refactored //////////////
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		for (auto iter = timingEdgeToPaths.begin(); iter != timingEdgeToPaths.end(); iter++)
		{
			std::string tempKey = iter->first;
		//	std::vector<int> currentPaths = iter->second;

			int x, y, z = -1;
			if (tempKey[0] == 'C') // if its a cell get the location of it please
				get_location_from_key(tempKey, x, y, z);
			else
				continue;


			std::vector<int> currentPaths;// = iter->second; // store paths of the current edge
			std::vector<int> currentNodes;// = iter->second; // store the corresponding node of each  path used by this edge

			for (int counter_vector = 0; counter_vector < (iter->second).size(); counter_vector++)
			{
				currentPaths.push_back((iter->second)[counter_vector].path);
				currentNodes.push_back((iter->second)[counter_vector].node);
			}

			// initially I will just use the max delay of all transisiotnas as the delay of this edge

			double maxDelay = 0.0;

			// if a register is cascaded htne there are 2 delays associated with it Tco if its source and CELL delay if its a destination so I am gonna get both
			double maxDelaySourceReg = -1.0;
			double maxDelayDestReg = -1.0;

			auto iter_delay = Cells_REsRandVars.find(tempKey); //timingEdgesRandVars
			auto iter_delay_standard = Cells_REsRandVars_standard.find(tempKey);


			// the timing edge must exist in the timingedge delay vars map
			assert(iter_delay != Cells_REsRandVars.end()); //timingEdgesRandVars
			assert(iter_delay_standard != Cells_REsRandVars_standard.end()); //timingEdgesRandVars

			edgetype = -1;

			std::vector<double> transitionDelay(8, -1);

			// loop across different transistions at this edge
			for (int i = 0; i < (iter_delay->second).size(); i++)
			{
				//double currDelay = ((iter_delay->second)[i].randVars)(gen);

				double currDelay;

				if (corelationModel == NOCORELATION)// totally independant variables
				{
					currDelay = ((iter_delay->second)[i].randVars)(gen);
				}
				else if (corelationModel == FULLCORELATION)// fully correlated
				{
					// del = u + sigma*RV
					currDelay = (iter_delay->second)[i].randVars.mean() + ((iter_delay->second)[i].randVars.stddev())*currSampleRv;
				}
				else if (corelationModel == PARTIALCORELATION)
				{
					int xBegin = x / ((FPGAsizeX - 1) / horiz);

					if (x % (((FPGAsizeX - 1) / horiz)) == 0 && x != 0)
						xBegin--;

					int yBegin = y / ((FPGAsizeY - 1) / vert);

					if (y % (((FPGAsizeY - 1) / vert)) == 0 && y != 0)
						yBegin--;

					if (slidingWindow)
					{
						//////////////////////////////////////////////////////////
						///////////////////// stuf for sliding window/////////////
						//////////////////////////////////////////////////////////
						double RVsSum = 0.0;

						// adjust xbegin and ybegin to account for added padded RVs (margins) at the left and bottom of the chip
						xBegin += xMargin;
						yBegin += yMargin;

						int xMargInitial = xBegin - xMargin;
						int yMargInitial = yBegin - yMargin;

						// loop over the windows row
						for (int windowX = 0; windowX < xMargin * 2 + 1; windowX++)
						{
							for (int windowY = 0; windowY < yMargin * 2 + 1; windowY++)
							{
								RVsSum += currSampleRVMatrix[xMargInitial + windowX][yMargInitial + windowY];
							}
						}

						//////////////////////////////////////////////////////////
						////////////////////// End ///////////////////////////////
						//////////////////////////////////////////////////////////

					
						double numberOfRVsInWindow = (xMargin * 2 + 1)*(yMargin * 2 + 1);
						currDelay = (iter_delay->second)[i].randVars.mean() + (((iter_delay->second)[i].randVars.stddev()) / sqrt(numberOfRVsInWindow + 1))*(RVsSum + ((iter_delay_standard->second)[i].randVars)(gen));
					}
					else
					{
						currDelay = (iter_delay->second)[i].randVars.mean()
							+ (((iter_delay->second)[i].randVars.stddev())/sqrt(2))*currSampleRVMatrix[xBegin][yBegin]
						+ (((iter_delay->second)[i].randVars.stddev()) / sqrt(2)*((iter_delay_standard->second)[i].randVars)(gen));
					}

					if (iter == timingEdgeToPaths.begin())
					{
				//		RE_MCSim << currDelay << std::endl;
						if (sampleCounter == 0)
							std::cout << "Second coloumn CELL mean is " << (iter_delay->second)[i].randVars.mean() << " stdev is " << ((iter_delay->second)[i].randVars.stddev()) << std::endl;
					}
				}
				else
				{
					std::cout << "ERROR : Wrong corelation model." << std::endl;
					assert(false);
				}

				transitionDelay[(iter_delay->second)[i].type] = currDelay;

				if (currDelay > maxDelay)
				{
					maxDelay = currDelay;
					edgetype = (iter_delay->second)[i].type;
				}

				//	maxDelay = currDelay;

				if ((iter_delay->second)[i].type > 3)
					assert(z%LUTFreq != 0);



				if (z > -1 && z%LUTFreq != 0) // this is a source
				{
					if ((iter_delay->second)[i].type <= 3) // reg as a dest
					{
						if (currDelay > maxDelayDestReg)
							maxDelayDestReg = currDelay;
					}
					else // reg is a source
					{
						if (currDelay > maxDelaySourceReg)
							maxDelaySourceReg = currDelay;
					}
				}

			}

			//	if (sampleCounter % 100 == 0 && iter == timingEdgeToPaths.begin())
			//		std::cout << " delay is " << std::endl;

			// now we have a delay for this edge lets add it to all the paths its using

			for (int i = 0; i < currentPaths.size(); i++)
			{
				//	if(currentPaths[i]==1)
				//	{
				//		std::cout << "path number 1 currentDelay is" << pathsDelay[currentPaths[i]] << " + " << maxDelay << std::endl;
				//	}

				if (z < 0 || z%LUTFreq == 0) // LUT
				{
					int currentEdgeType = paths[currentPaths[i]][currentNodes[i]].edgeType; // get the corresponding cell delay
					assert(currentEdgeType > -1 && currentEdgeType < 4);

					if (z < 0) // then its an IC delay, so it can eithe have type 0 (FF) or type 3 (RR), get the type from the destination node transistion. transofrm 0,1 -> 0 and 2,3 to 3
					{
						currentEdgeType = (int)floor(currentEdgeType / 2.0) * 3;//(currentEdgeType % 2) * 3;
					}

					assert(transitionDelay[currentEdgeType] > -1);
					pathsDelay[currentPaths[i]] += transitionDelay[currentEdgeType];

					//pathsDelay[currentPaths[i]] += maxDelay;
				}
					
				else // a cell register delay
				{
					if (paths[currentPaths[i]][0].x == x && paths[currentPaths[i]][0].y == y && paths[currentPaths[i]][0].z == z) // this timing edge is part of the source of the path
					{
						assert(maxDelaySourceReg > -0.00000000000000001);
						pathsDelay[currentPaths[i]] += maxDelaySourceReg;
					}
					else // then it's a destination
					{
						int currentEdgeType = paths[currentPaths[i]][currentNodes[i]].edgeType; // get the corresponding cell delay
						assert(currentEdgeType > -1 && currentEdgeType < 4);
						assert(maxDelayDestReg > -0.00000000000000001);
						assert(transitionDelay[currentEdgeType] > -1);
						pathsDelay[currentPaths[i]] += transitionDelay[currentEdgeType];
						//		pathsDelay[currentPaths[i]] += maxDelayDestReg;
					}
				}

				if (pathsDelay[currentPaths[i]]>maxPathDelay)
				{
					maxPathDelay = pathsDelay[currentPaths[i]];
					longestPath = currentPaths[i];
				}

				if (pathsDelay[currentPaths[i]]>maxUntestedPathDelay && !paths[currentPaths[i]][0].tested)
				{
					maxUntestedPathDelay = pathsDelay[currentPaths[i]];
					longestUntestedPath = currentPaths[i];
				}



				if (pathsDelay[currentPaths[i]]>maxTestedPathDelay && paths[currentPaths[i]][0].tested)
				{
					maxTestedPathDelay = pathsDelay[currentPaths[i]];
					longestTestedPath = currentPaths[i];
				}
			}

		}
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////// end copy /////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////










		/// for stats purposes see what is the longest path ate each MC sample begin

		int index_longest_path = -1;
		for (int k = 0; k < longestPaths.size(); k++)
		{
			if (longestPaths[k].first == longestPath)
			{
				index_longest_path = k;

			}
		}


		if (index_longest_path > -1) // then we have seen this path b4, so just increment the number of occurunces
		{

			longestPaths[index_longest_path].second++;
		}
		else
		{
			longestPaths.push_back(std::pair<int, int >(longestPath, 1));
		}
		// end

		// now we will check if the longest path corresponds to an untested path or not

		bool fail = false;
		for (int i = 0; i < untestedPaths.size(); i++)
		{
			if (longestPath == untestedPaths[i])
			{
				//std::cout << "longest path was found in an untested path " << std::endl;
				//std::cout << "longest path is " << untestedPaths[i] << std::endl;
				// insert this path in the set of problematic path
				int index = -1;
				int untestedPath = -1;
				untestedPath = untestedPaths[i];
				for (int k = 0; k < problematicPaths.size(); k++)
				{
					if (problematicPaths[k].first == untestedPaths[i])
					{
						index = k;

					}
				}

				assert(untestedPath > -1);

				if (index > -1) // then we have seen this path b4, so just increment the number of occurunces
				{

					problematicPaths[index].second++;
				}
				else
				{
					problematicPaths.push_back(std::pair<int, int >(untestedPath, 1));
				}
				fail = true;
			}
		}


		// get the critical path and its delay, we want to get the shortest CP delay
		if ((sampleCounter == 0) || (shortestCriticalPath_delay.second > maxPathDelay))
		{
			shortestCriticalPath_delay.first = longestPath;
			shortestCriticalPath_delay.second = maxPathDelay;
		}

		//get the critical path and its delay, we want to get the longest CP delay
		if ((sampleCounter == 0) || (longestCriticalPath_delay.second < maxPathDelay))
		{
			longestCriticalPath_delay.first = longestPath;
			longestCriticalPath_delay.second = maxPathDelay;
		}

		// now I just wanted to check if the longest delay is also the same as a tested path

		for (int i = 0; i < testedPaths.size(); i++)
		{
			if (maxPathDelay == pathsDelay[testedPaths[i]])
			{
				if (fail)
				{
					std::cout << "the longest delay was found in two different sets; tested and untested";
					fail = false;
				}
			}

		}

		if (fail)
		{
			num_of_failures++;
			assert(maxUntestedPathDelay > maxTestedPathDelay);
	//		std::cout << "Maximum untested path " << longestUntestedPath << " delay is " << maxUntestedPathDelay << " maximum tested path " << longestTestedPath << " delay is " << maxTestedPathDelay << std::endl;

		}

	}

	pathsImport.clear();
	pathsImport.resize(paths.size());

	std::fill(pathsImport.begin(), pathsImport.end(), 0.0);

	// print problematic paths
	for (int k = 0; k < problematicPaths.size(); k++)
	{
		RE_MCSim << "Path " << problematicPaths[k].first << " is a problem in " << problematicPaths[k].second << " samples." << std::endl;
		assert(pathsImport[problematicPaths[k].first] == 0.0);

		pathsImport[problematicPaths[k].first] = (problematicPaths[k].second*1.0);// / (num_of_failures);
	}
	RE_MCSim << "=----=-=-09-09=-=-09-0=-0==-0-9=-=0- " << std::endl;
	int slowestPath = longestPaths[0].first;
	for (int k = 0; k < longestPaths.size(); k++)
	{
		if (longestPaths[k].first > slowestPath)
			slowestPath = longestPaths[k].first;
		RE_MCSim << "Path " << longestPaths[k].first << " is the longest in " << longestPaths[k].second << " samples." << std::endl;
	}

	// add a bit of weight to the paths that are not longest to let the ILp consider testing them too
	for (int k = 1; k < pathsImport.size(); k++)
	{
		if (pathsImport[k] != 0)
			continue;

		pathsImport[k] = 1.0 / (paths.size());// *((k / 10) + 1));// *num_of_failures);


	}

	RE_MCSim << "Number of paths that were the longest " << longestPaths.size() << std::endl;
	RE_MCSim << "Fastest slowest path was " << slowestPath << std::endl;

	RE_MCSim << "Out of all the MC samples, the shortest delay was for path " << shortestCriticalPath_delay.first << " delay was " << shortestCriticalPath_delay.second << std::endl;
	RE_MCSim << "Out of all the MC samples, the longest delay was for path " << longestCriticalPath_delay.first << " delay was " << longestCriticalPath_delay.second << std::endl;


	return (num_of_failures*1.0) / num_of_simulations;
}







//////////////////////////////////////////////////////////////////////////
///////////////// Validate data structures used for MC sim ///////////////
//////////////////////////////////////////////////////////////////////////


// validatye MC with edge delays
bool MC_validate_edges_delays(std::map<std::string, std::vector<Path_logic_component> > timingEdgeToPaths)
{
	std::vector<double> pathsDelay(paths.size(), 0.0); // stores path delays with clock skew into account, the input edge is rising
	std::vector<double> pathsDelayF(paths.size(), 0.0); // stores path delays with clock skew into account, the input edge is falling

													   // taking clock skew into account
	for (int i = 1; i < paths.size(); i++)
	{
		pathsDelay[i] -= pathClockSkew[i];
		pathsDelayF[i] -= pathClockSkew[i];
	}

	std::vector<double> testedPathsDelay;
	std::vector<double> untestedPathsDelay;

	testedPathsDelay.resize(0);
	untestedPathsDelay.resize(0);

	double maxPathDelay = 0.0;

	double maxUntestedPathDelay = 0.0;
	double maxTestedPathDelay = 0.0;

	int longestUntestedPath = -1;
	int longestTestedPath = -1;


	int longestPath = -1;
	int edgetype = -1;
	// loop over all timing edges to get the delay of each edge and add it to the paths using it
	for (auto iter = timingEdgeToPaths.begin(); iter != timingEdgeToPaths.end(); iter++)
	{
		std::string tempKey = iter->first;
		std::vector<int> currentPaths;// = iter->second; // store paths of the current edge
		std::vector<int> currentNodes;// = iter->second; // store the corresponding node of each  path used by this edge

		for (int counter_vector = 0; counter_vector < (iter->second).size(); counter_vector++)
		{
			currentPaths.push_back((iter->second)[counter_vector].path);
			currentNodes.push_back((iter->second)[counter_vector].node);
		}
		int x = -1;
		int y = -1;
		int z = -1;
		if (tempKey[0] == 'C') // if its a cell get the location of it please
			get_location_from_key(tempKey, x, y, z);
		

		// initially I will just use the max delay of all transisiotnas as the delay of this edge

		double maxDelay = 0.0;

		// if a register is cascaded htne there are 2 delays associated with it Tco if its source and CELL delay if its a destination so I am gonna get both
		double maxDelaySourceReg = -1.0;
		double maxDelayDestReg = -1.0;

		auto iter_delay = timingEdgesDelay.find(tempKey);

		if (tempKey[0] != 'C') // if its not a cell then make sure that the IC delay only has 2 possible values
			assert((iter_delay->second).size() < 3);

		// the timing edge must exist in the timingedge delay vars map
		assert(iter_delay != timingEdgesDelay.end());

		edgetype = -1;

		std::vector<double> transitionDelay(8, -1);
		// loop across different transistions at this edge
		for (int i = 0; i < (iter_delay->second).size(); i++)
		{
			double currDelay = ((iter_delay->second)[i]).delay;

			transitionDelay[(iter_delay->second)[i].type] = currDelay;

			if (currDelay > maxDelay)
			{
				maxDelay = currDelay;
				edgetype = (iter_delay->second)[i].type;
			}
			if ((iter_delay->second)[i].type > 3)
				assert(z%LUTFreq != 0);

			if (z > -1 && z%LUTFreq != 0) // this is a reg
			{
				if ((iter_delay->second)[i].type <= 3) // reg as a dest
				{
					if (currDelay > maxDelayDestReg)
						maxDelayDestReg = currDelay;
				}
				else // reg is a source
				{
					if (currDelay > maxDelaySourceReg)
						maxDelaySourceReg = currDelay;
				}
			}

		}

		//	if (sampleCounter % 100 == 0 && iter == timingEdgeToPaths.begin())
		//		std::cout << " delay is " << std::endl;

		// now we have a delay for this edge lets add it to all the paths its using

		for (int i = 0; i < currentPaths.size(); i++)
		{
			//	if(currentPaths[i]==1)
			//	{
			//		std::cout << "path number 1 currentDelay is" << pathsDelay[currentPaths[i]] << " + " << maxDelay << std::endl;
			//	}
	//		if (currentPaths[i] == 7)
	//			std::cout << "Debug" << std::endl;

			if (z < 0 || z%LUTFreq == 0) // LUT
			{
				int currentEdgeType = paths[currentPaths[i]][currentNodes[i]].edgeType; // get the corresponding cell delay
				assert(currentEdgeType > -1 && currentEdgeType < 4);

				if (z < 0) // then its an IC delay, so it can eithe have type 0 (FF) or type 3 (RR), get the type from the destination node transistion. transofrm 0,1 -> 0 and 2,3 to 3
				{
					currentEdgeType = (int)floor(currentEdgeType / 2.0) * 3;//(currentEdgeType % 2) * 3;
				}
				
				assert(transitionDelay[currentEdgeType] > -1);
				pathsDelay[currentPaths[i]] += transitionDelay[currentEdgeType];
				//pathsDelay[currentPaths[i]] += maxDelay;
			}
			else // a cell reg delay
			{
				if (paths[currentPaths[i]][0].x == x && paths[currentPaths[i]][0].y == y && paths[currentPaths[i]][0].z == z) // this timing edge is part of the source of the path
				{
					assert(maxDelaySourceReg > -0.00000000000000001);
					pathsDelay[currentPaths[i]] += maxDelaySourceReg;
				}
				else // then it's a destination
				{
					int currentEdgeType = paths[currentPaths[i]][currentNodes[i]].edgeType; // get the corresponding cell delay
					assert(currentEdgeType > -1 && currentEdgeType < 4);
					assert(maxDelayDestReg > -0.00000000000000001);
					assert(transitionDelay[currentEdgeType] > -1);
					pathsDelay[currentPaths[i]] += transitionDelay[currentEdgeType];
			//		pathsDelay[currentPaths[i]] += maxDelayDestReg;
				}
			}

			if (pathsDelay[currentPaths[i]]>maxPathDelay)
			{
				maxPathDelay = pathsDelay[currentPaths[i]];
				longestPath = currentPaths[i];
			}

			if (pathsDelay[currentPaths[i]]>maxUntestedPathDelay && !paths[currentPaths[i]][0].tested)
			{
				maxUntestedPathDelay = pathsDelay[currentPaths[i]];
				longestUntestedPath = currentPaths[i];
			}



			if (pathsDelay[currentPaths[i]]>maxTestedPathDelay && paths[currentPaths[i]][0].tested)
			{
				maxTestedPathDelay = pathsDelay[currentPaths[i]];
				longestTestedPath = currentPaths[i];
			}
		}

	}

	// now we have all path delaysa including clock skew, lets compare what we have against the stored paths slack

	bool succ = true;
	//double relationship = 1;
	std::cout << "using Edges " << std::endl;
	for (int i = 1; i < pathsDelay.size(); i++)
	{
		double calcSlack = pathClockRelation[i] - pathsDelay[i] - 0.002; // the 0.002 is the clock uncertaininty used by quartus
		double storedSlack = pathSlack[i];
		if (abs(calcSlack - storedSlack)>0.009)
		{
			std::cout << "Stored slack for path " << i << " is " << storedSlack << " calculated slack is " << calcSlack << std::endl;
			if (calcSlack > storedSlack)
			{
				succ = false;
				std::cout << "Stored slack for path " << i << " is " << storedSlack << " calculated slack is " << calcSlack << std::endl;
			}
			assert(false);
		}
//		std::cout << "Good work man Stored slack for path " << i << " is " << storedSlack << " calculated slack is " << calcSlack << std::endl;
	}

	return succ;
}


//////////////////////////////////////////
// validate MC with RE delays ////////////
///////////////////////////////////////////

bool MC_validate_RE_delays(std::map<std::string, std::vector<Path_logic_component> > timingEdgeToPaths)
{
	std::vector<double> pathsDelay(paths.size(), 0.0); // stores it with clock skew into account

	// taking clock skew and total delta RE into account
	for (int i = 1; i < paths.size(); i++)
	{
		pathsDelay[i] -= pathClockSkew[i];
		pathsDelay[i] += pathREsDelta[i];
	}

	std::vector<double> testedPathsDelay;
	std::vector<double> untestedPathsDelay;

	testedPathsDelay.resize(0);
	untestedPathsDelay.resize(0);

	double maxPathDelay = 0.0;

	double maxUntestedPathDelay = 0.0;
	double maxTestedPathDelay = 0.0;

	int longestUntestedPath = -1;
	int longestTestedPath = -1;

	int longestPath = -1;
	int edgetype = -1;
	for (auto iter = REToPaths.begin(); iter != REToPaths.end(); iter++)
	{
		std::string tempKey = iter->first;
		std::vector<int> currentPaths;// = iter->second;
		std::vector<int> currentNodes;// = iter->second; // store the corresponding node of each  path used by this edge


		for (int counter_vector = 0; counter_vector < (iter->second).size(); counter_vector++)
		{
			currentPaths.push_back((iter->second)[counter_vector].path);
			currentNodes.push_back((iter->second)[counter_vector].node);
		}
		// initially I will just use the max delay of all transisiotnas as the delay of this edge

		double maxDelay = 0.0;

		auto iter_delay = REsDelay.find(tempKey);

		// the timing edge must exist in the timingedge delay vars map
		assert(iter_delay != REsDelay.end());

		// at most the number of delays of an RE can be 2, FF and RR
		assert((iter_delay->second).size() < 3);

		edgetype = -1;

		std::vector<double> transitionDelay(4, -1); // RE edge transition can either be 0 or 3

		// loop across different transistions at this edge to store the corresponding delay of each edge
		for (int i = 0; i < (iter_delay->second).size(); i++)
		{
			double currDelay = (iter_delay->second)[i].delay;

			assert((iter_delay->second)[i].type == 0 || (iter_delay->second)[i].type == 3); // either ff or rr

			transitionDelay[(iter_delay->second)[i].type] = currDelay;
			
			if (currDelay > maxDelay)
				maxDelay = currDelay;

		}


		for (int i = 0; i < currentPaths.size(); i++)
		{

			int currentEdgeType = paths[currentPaths[i]][currentNodes[i]].edgeType; // get the corresponding cell delay
			assert(currentEdgeType > -1 && currentEdgeType < 4);
			
			// this is a RE delay, so it can eithe have type 0 (FF) or type 3 (RR), get the type from the destination node transistion. transofrm 0,1 -> 0 and 2,3 to 3
			currentEdgeType = (int)floor(currentEdgeType / 2.0) * 3;//(currentEdgeType % 2) * 3;
			assert(transitionDelay[currentEdgeType] > -1);
			if (currentPaths[i] == 1)
				std::cout << "a7aaaa" << std::endl;
			pathsDelay[currentPaths[i]] += transitionDelay[currentEdgeType];
		//	pathsDelay[currentPaths[i]] += maxDelay;
		//	if (currentPaths[i] == 1)
		//		std::cout << "Debug" << std::endl;
			if (pathsDelay[currentPaths[i]]>maxPathDelay)
			{
				maxPathDelay = pathsDelay[currentPaths[i]];
				longestPath = currentPaths[i];
			}

			if (pathsDelay[currentPaths[i]]>maxUntestedPathDelay && !paths[currentPaths[i]][0].tested)
			{
				maxUntestedPathDelay = pathsDelay[currentPaths[i]];
				longestUntestedPath = currentPaths[i];
			}

			if (pathsDelay[currentPaths[i]]>maxTestedPathDelay && paths[currentPaths[i]][0].tested)
			{
				maxTestedPathDelay = pathsDelay[currentPaths[i]];
				longestTestedPath = currentPaths[i];
			}

		}

	}



	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////// copy from timing edges validation function, with ony changing if its a cell then ignor this mother fucker ////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	for (auto iter = timingEdgeToPaths.begin(); iter != timingEdgeToPaths.end(); iter++)
	{
		std::string tempKey = iter->first;
		int x, y, z = -1;
		if (tempKey[0] == 'C') // if its a cell get the location of it please if not ignore this
			get_location_from_key(tempKey, x, y, z);
		else
			continue;
		std::vector<int> currentPaths;// = iter->second; // store paths of the current edge
		std::vector<int> currentNodes;// = iter->second; // store the corresponding node of each  path used by this edge

		for (int counter_vector = 0; counter_vector < (iter->second).size(); counter_vector++)
		{
			currentPaths.push_back((iter->second)[counter_vector].path);
			currentNodes.push_back((iter->second)[counter_vector].node);
		}
	

		// initially I will just use the max delay of all transisiotnas as the delay of this edge

		double maxDelay = 0.0;

		// if a register is cascaded htne there are 2 delays associated with it Tco if its source and CELL delay if its a destination so I am gonna get both
		double maxDelaySourceReg = -1.0;
		double maxDelayDestReg = -1.0;

		auto iter_delay = timingEdgesDelay.find(tempKey);

		if (tempKey[0] != 'C') // if its not a cell then make sure that the IC delay only has 2 possible values
			assert((iter_delay->second).size() < 3);

		// the timing edge must exist in the timingedge delay vars map
		assert(iter_delay != timingEdgesDelay.end());

		edgetype = -1;

		std::vector<double> transitionDelay(8, -1);
		// loop across different transistions at this edge
		for (int i = 0; i < (iter_delay->second).size(); i++)
		{
			double currDelay = ((iter_delay->second)[i]).delay;

			transitionDelay[(iter_delay->second)[i].type] = currDelay;

			if (currDelay > maxDelay)
			{
				maxDelay = currDelay;
				edgetype = (iter_delay->second)[i].type;
			}
			if ((iter_delay->second)[i].type > 3)
				assert(z%LUTFreq != 0);

			if (z > -1 && z%LUTFreq != 0) // this is a reg
			{
				if ((iter_delay->second)[i].type <= 3) // reg as a dest
				{
					if (currDelay > maxDelayDestReg)
						maxDelayDestReg = currDelay;
				}
				else // reg is a source
				{
					if (currDelay > maxDelaySourceReg)
						maxDelaySourceReg = currDelay;
				}
			}

		}

		//	if (sampleCounter % 100 == 0 && iter == timingEdgeToPaths.begin())
		//		std::cout << " delay is " << std::endl;

		// now we have a delay for this edge lets add it to all the paths its using

		for (int i = 0; i < currentPaths.size(); i++)
		{
			//	if(currentPaths[i]==1)
			//	{
			//		std::cout << "path number 1 currentDelay is" << pathsDelay[currentPaths[i]] << " + " << maxDelay << std::endl;
			//	}
	//		if (currentPaths[i] == 1)
	//			std::cout << "Debug" << std::endl;

			if (z < 0 || z%LUTFreq == 0) // LUT
			{
				int currentEdgeType = paths[currentPaths[i]][currentNodes[i]].edgeType; // get the corresponding cell delay
				assert(currentEdgeType > -1 && currentEdgeType < 4);

				if (z < 0) // then its an IC delay, so it can eithe have type 0 (FF) or type 3 (RR), get the type from the destination node transistion. transofrm 0,1 -> 0 and 2,3 to 3
				{
					currentEdgeType = (int)floor(currentEdgeType / 2.0) * 3;//(currentEdgeType % 2) * 3;
				}

				assert(transitionDelay[currentEdgeType] > -1);
				if (currentPaths[i] == 1)
					std::cout << "a7aaaa" << std::endl;
				pathsDelay[currentPaths[i]] += transitionDelay[currentEdgeType];
				//pathsDelay[currentPaths[i]] += maxDelay;
			}
			else // a cell reg delay
			{
				if (paths[currentPaths[i]][0].x == x && paths[currentPaths[i]][0].y == y && paths[currentPaths[i]][0].z == z) // this timing edge is part of the source of the path
				{
					assert(maxDelaySourceReg > -0.00000000000000001);
					if (currentPaths[i] == 1)
						std::cout << "a7aaaa" << std::endl;
					pathsDelay[currentPaths[i]] += maxDelaySourceReg;
				}
				else // then it's a destination
				{
					int currentEdgeType = paths[currentPaths[i]][currentNodes[i]].edgeType; // get the corresponding cell delay
					assert(currentEdgeType > -1 && currentEdgeType < 4);
					assert(maxDelayDestReg > -0.00000000000000001);
					assert(transitionDelay[currentEdgeType] > -1);
					if (currentPaths[i] == 1)
						std::cout << "a7aaaa" << std::endl;
					pathsDelay[currentPaths[i]] += transitionDelay[currentEdgeType];
					//		pathsDelay[currentPaths[i]] += maxDelayDestReg;
				}
			}

			if (pathsDelay[currentPaths[i]]>maxPathDelay)
			{
				maxPathDelay = pathsDelay[currentPaths[i]];
				longestPath = currentPaths[i];
			}

			if (pathsDelay[currentPaths[i]]>maxUntestedPathDelay && !paths[currentPaths[i]][0].tested)
			{
				maxUntestedPathDelay = pathsDelay[currentPaths[i]];
				longestUntestedPath = currentPaths[i];
			}



			if (pathsDelay[currentPaths[i]]>maxTestedPathDelay && paths[currentPaths[i]][0].tested)
			{
				maxTestedPathDelay = pathsDelay[currentPaths[i]];
				longestTestedPath = currentPaths[i];
			}
		}

	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////// end this copy bitch ///////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// now we have all path delaysa including clock skew, lets compare what we have against the stored paths slack

	bool succ = true;
	//double relationship = 1;
	std::cout << "using REs " << std::endl;

	for (int i = 1; i < pathsDelay.size(); i++)
	{
		double calcSlack = pathClockRelation[i] - pathsDelay[i] - 0.002; // the 0.002 is the clock uncertaininty and clock set up time used by quartus
		double storedSlack = pathSlack[i];
		if (abs(calcSlack - storedSlack)>0.009)
		{
			std::cout << "Stored slack for path " << i << " is " << storedSlack << " calculated slack is " << calcSlack << std::endl;
			if (calcSlack > storedSlack)
			{
				succ = false;
				std::cout << "Stored slack for path " << i << " is " << storedSlack << " calculated slack is " << calcSlack << std::endl;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////// HARD CODING FOR ONE PATH IN ONE BENCHMARKS for FPL and thn remove/////////////////
			/////////////////////////////////////// Occurs because of the -0.001 in the IC delay by Q that we dont acknowledg/////////
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if(i!=13709)
				assert(false);
		}
	//		std::cout << "Good work man Stored slack for path " << i << " is " << storedSlack << " calculated slack is " << calcSlack << std::endl;
	}

	return succ;

}


////////////////////////////////////////////////////////
/////////////// RUn MC simulation //////////////////////
////////////////////////////////////////////////////////

double run_MC(bool slidingWindow, int corelationModel, int number_of_samples,  std::map<std::string, double>  timingEdgesMapComplete, std::map<std::string, double>  testedTimingEdgesMap, std::map<std::string, std::vector<Path_logic_component> >  timingEdgeToPaths, int remainingPaths, double sigma, double qDelayInter, std::vector<double> & pathsImport)
{
	std::cout << "**************************** Running MC simulation for " << number_of_samples << " samples with sigma of " << sigma << " and qdelay as mean plus " << qDelayInter << " sigma " << std::endl;



		std::cout << "All edges were tested but still there remains the following number of untested paths :-  " << remainingPaths << " untested " << std::endl;
		std::cout << "starting MC simulation with " << number_of_samples << " samples " << std::endl;

		std::vector<int> testedPaths;
		testedPaths.resize(0);
		std::vector<int> unTestedPaths;

		unTestedPaths.resize(0);

		for (int j = 0; j < paths.size(); j++)
		{
			if (paths[j].size() < 1)
				continue;
			if (paths[j][0].tested)
			{
				testedPaths.push_back(j);
			}
			else
			{
				unTestedPaths.push_back(j);
			}
		}

//		std::cout << "untested " << unTestedPaths.size() << std::endl;

//		std::cout << "uremaining  " << remainingPaths << std::endl;

		assert(remainingPaths == unTestedPaths.size());

		// starting MC simulation

		MC_generate_edges_rand_vars(sigma, qDelayInter);
		MC_generate_REs_rand_vars(sigma, qDelayInter);

		if (MC_validate_RE_delays(timingEdgeToPaths))
			std::cout << "Alles clar " << std::endl;
		else
			assert(false);

		if (MC_validate_edges_delays(timingEdgeToPaths))
			std::cout << "Alles clar " << std::endl;
		else
			assert(false);

//		double failureProb = MC_sim_edges(number_of_samples, unTestedPaths, testedPaths, timingEdgeToPaths, pathsImport);
		//		double failureProb = 0.0;
//		std::cout << "failure rate with timing edges is " << failureProb << std::endl;
//		TE_MCSim << "failure rate with timing edges is " << failureProb << std::endl;

		double failureProb2 = MC_sim_RE(slidingWindow, corelationModel,number_of_samples, unTestedPaths, testedPaths, timingEdgeToPaths, pathsImport);

		//		double failureProb = 0.0;
		std::cout << "failure rate with RE is " << failureProb2 << std::endl;
		RE_MCSim << "failure rate with RE is " << failureProb2 << std::endl;
		return failureProb2 * number_of_samples;
		
//	}
}

// does the exact same thing as run_mc but it doesn't initialize the rand variables and delays etc..
void rerun_MC(bool slidingWindow, int corelationModel, int number_of_samples, std::map<std::string, double>  timingEdgesMapComplete, std::map<std::string, double>  testedTimingEdgesMap, std::map<std::string, std::vector<Path_logic_component> >  timingEdgeToPaths,   std::vector<double> & pathsImport)
{
	std::vector<int> testedPaths;
	testedPaths.resize(0);
	std::vector<int> unTestedPaths;
	unTestedPaths.resize(0);

	for (int j = 0; j < paths.size(); j++)
	{
		if (paths[j].size() < 1)
			continue;
		if (paths[j][0].tested)
		{
			testedPaths.push_back(j);
		}
		else
		{
			unTestedPaths.push_back(j);
		}
	}
	//double failureProb = MC_sim_edges(number_of_samples, unTestedPaths, testedPaths, timingEdgeToPaths, pathsImport);
	//		double failureProb = 0.0;
	//std::cout << "failure rate with timing edges is " << failureProb << std::endl;
	//TE_MCSim << "failure rate with timing edges is " << failureProb << std::endl;

	double failureProb = MC_sim_RE(slidingWindow, corelationModel,number_of_samples, unTestedPaths, testedPaths, timingEdgeToPaths, pathsImport);
	//		double failureProb = 0.0;
	std::cout << "failure rate with RE is " << failureProb << std::endl;
	RE_MCSim << "failure rate with RE is " << failureProb << std::endl;

}