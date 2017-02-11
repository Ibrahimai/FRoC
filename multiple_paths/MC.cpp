#pragma once
#include "MC.h"
#include "globalVar.h"

std::map<std::string, std::vector < Rand_Edge_Delay > >  timingEdgesRandVars;
std::map<std::string, std::vector < Rand_Edge_Delay > >  Cells_REsRandVars;


//dev_per_to_mean is the ration of dev / mean, quartus_delay is how do we interperet the delay reported by quartus, 1 means mean + sigma
void MC_generate_REs_rand_vars(double dev_per_to_mean, int quartus_delay)
{
	Cells_REsRandVars.clear();

	// loop over the global variable timingEdgesDelay to add cell delay to rand variable Cells_RErandsvars
	for (auto iter = timingEdgesDelay.begin(); iter != timingEdgesDelay.end(); iter++)
	{



		std::string tempKey = iter->first;
		// if its a cell delay thne the sirst parts of the string should be CELLsX
		if (!(tempKey[0] == 'c' && tempKey[1] == 'E' && tempKey[2] == 'L' && tempKey[3] == 'L')) // if its not a cell delay then continue, we dont care about IC delay
			continue;


		////// begin trial, push cell delay into celledgesdelay map
		assert(cellEdgesDelay.find(tempKey) == cellEdgesDelay.end());
		cellEdgesDelay.insert(std::pair<std::string, std::vector < Edge_Delay > >(iter->first, iter->second));
		/////// end trial

		std::vector< Edge_Delay > currentEdge = iter->second;

		std::vector <Rand_Edge_Delay > currentEdgeRand; // vectore storing (edgyeType,randVariabl) of the current edge
		currentEdgeRand.resize(0);

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
			Rand_Edge_Delay temp = Rand_Edge_Delay(currentEdge[i].type, randVarstemp);
			currentEdgeRand.push_back(temp);
		}

		Cells_REsRandVars.insert(std::pair<std::string, std::vector<Rand_Edge_Delay > >(tempKey, currentEdgeRand));
	}


	// loop over the global variable REsDelay to add RE delay 
	for (auto iter = REsDelay.begin(); iter != REsDelay.end(); iter++)
	{



		std::string tempKey = iter->first;
	

		std::vector< Edge_Delay > currentEdge = iter->second;

		std::vector <Rand_Edge_Delay > currentEdgeRand; // vectore storing (edgyeType,randVariabl) of the current edge
		currentEdgeRand.resize(0);

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
			Rand_Edge_Delay temp = Rand_Edge_Delay(currentEdge[i].type, randVarstemp);
			currentEdgeRand.push_back(temp);
		}

		Cells_REsRandVars.insert(std::pair<std::string, std::vector<Rand_Edge_Delay > >(tempKey, currentEdgeRand));
	}


}

//dev_per_to_mean is the ration of dev / mean, quartus_delay is how do we interperet the delay reported by quartus, 1 means mean + sigma
void MC_generate_edges_rand_vars(double dev_per_to_mean, int quartus_delay) 
{
	 
	timingEdgesRandVars.clear();


	// loop over the global variable timingEdgesDelay
	for (auto iter = timingEdgesDelay.begin(); iter != timingEdgesDelay.end(); iter++)
	{
		
		

		std::string tempKey = iter->first;
		std::vector< Edge_Delay > currentEdge = iter->second;

		std::vector <Rand_Edge_Delay > currentEdgeRand; // vectore storing (edgyeType,randVariabl) of the current edge
		currentEdgeRand.resize(0);

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
		}

		timingEdgesRandVars.insert(std::pair<std::string, std::vector<Rand_Edge_Delay > >(tempKey, currentEdgeRand));
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
// returns a probability of failure
double MC_sim_edges(int num_of_simulations, std::vector<int> untestedPaths, std::vector<int> testedPaths, std::map<std::string, std::vector<Path_logic_component> > timingEdgeToPaths)
{
	int num_of_failures = 0;

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
						currentEdgeType = floor(currentEdgeType / 2) * 3;//(currentEdgeType % 2) * 3;
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

	// print problematic paths
	for (int k = 0; k < problematicPaths.size(); k++)
	{
		std::cout << "Path " << problematicPaths[k].first << " is a problem in " << problematicPaths[k].second << " samples." << std::endl;
	}
	std::cout << "=----=-=-09-09=-=-09-0=-0==-0-9=-=0- " << std::endl;

	int slowestPath = longestPaths[0].first;
	for (int k = 0; k < longestPaths.size(); k++)
	{
		if (longestPaths[k].first > slowestPath)
			slowestPath = longestPaths[k].first;
		std::cout << "Path " << longestPaths[k].first << " is the longest in " << longestPaths[k].second << " samples." << std::endl;
	}
	std::cout << "Number of paths that were the longest " << longestPaths.size() << std::endl;
	std::cout << "Fastest slowest path was " << slowestPath << std::endl;


	return (num_of_failures*1.0) / num_of_simulations;
}


// MC sim with individual elements delay

double MC_sim_RE(int num_of_simulations, std::vector<int> untestedPaths, std::vector<int> testedPaths, std::map<std::string, std::vector<int> > timingEdgeToPaths)
{
	int num_of_failures = 0;

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



	for (int sampleCounter = 0; sampleCounter < num_of_simulations; sampleCounter++) // creates number of simulations
	{
		// vector to store all paths sizes
		std::vector<double> pathsDelay(paths.size(), 0.0); // stores it with clock skew into account

														   // taking clock skew into account
		for (int i = 1; i < paths.size(); i++)
		{
			pathsDelay[i] -= pathClockSkew[i];
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

			// the timing edge must exist in the timingedge delay vars map
			assert(iter_delay != Cells_REsRandVars.end());

			// at most the number of delays of an RE can be 2, FF and RR
			assert((iter_delay->second).size() < 2);

			// loop across different transistions at this edge to get the max delay
			for (int i = 0; i < (iter_delay->second).size(); i++)
			{
				double currDelay = ((iter_delay->second)[i].randVars)(gen);
				if (currDelay > maxDelay)
					maxDelay = currDelay;			
			}

			for (int i = 0; i < currentPaths.size(); i++)
			{
				pathsDelay[currentPaths[i]] += maxDelay;	

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
		/////////////////////// copied this from the mc_sim_edges and only changed if its IC delay then ignore it //////////////
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		for (auto iter = timingEdgeToPaths.begin(); iter != timingEdgeToPaths.end(); iter++)
		{
			std::string tempKey = iter->first;
			std::vector<int> currentPaths = iter->second;

			int x, y, z = -1;
			if (tempKey[0] == 'C') // if its a cell get the location of it please
				get_location_from_key(tempKey, x, y, z);
			else
				continue;

			// initially I will just use the max delay of all transisiotnas as the delay of this edge

			double maxDelay = 0.0;

			// if a register is cascaded htne there are 2 delays associated with it Tco if its source and CELL delay if its a destination so I am gonna get both
			double maxDelaySourceReg = -1.0;
			double maxDelayDestReg = -1.0;

			auto iter_delay = timingEdgesRandVars.find(tempKey);

			// the timing edge must exist in the timingedge delay vars map
			assert(iter_delay != timingEdgesRandVars.end());

			// loop across different transistions at this edge
			for (int i = 0; i < (iter_delay->second).size(); i++)
			{
				double currDelay = ((iter_delay->second)[i].randVars)(gen);
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

			//	if (sampleCounter % 100 == 0 && iter == timingEdgeToPaths.begin())
			//		std::cout << " delay is " << std::endl;

			// now we have a delay for this edge lets add it to all the paths its using

			for (int i = 0; i < currentPaths.size(); i++)
			{
				//	if(currentPaths[i]==1)
				//	{
				//		std::cout << "path number 1 currentDelay is" << pathsDelay[currentPaths[i]] << " + " << maxDelay << std::endl;
				//	}

				if (z<0 || z%LUTFreq == 0) // LUT
					pathsDelay[currentPaths[i]] += maxDelay;
				else
				{
					if (paths[currentPaths[i]][0].x == x && paths[currentPaths[i]][0].y == y && paths[currentPaths[i]][0].z == z) // this timing edge is part of the source of the path
					{
						assert(maxDelaySourceReg > -0.00000000000000001);
						pathsDelay[currentPaths[i]] += maxDelaySourceReg;
					}
					else // then it's a destination
					{
						assert(maxDelayDestReg > -0.00000000000000001);
						pathsDelay[currentPaths[i]] += maxDelayDestReg;
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
			std::cout << "Maximum untested path " << longestUntestedPath << " delay is " << maxUntestedPathDelay << " maximum tested path " << longestTestedPath << " delay is " << maxTestedPathDelay << std::endl;

		}

	}

	// print problematic paths
	for (int k = 0; k < problematicPaths.size(); k++)
	{
		std::cout << "Path " << problematicPaths[k].first << " is a problem in " << problematicPaths[k].second << " samples." << std::endl;
	}
	std::cout << "=----=-=-09-09=-=-09-0=-0==-0-9=-=0- " << std::endl;
	for (int k = 0; k < longestPaths.size(); k++)
	{
		std::cout << "Path " << longestPaths[k].first << " is the longest in " << longestPaths[k].second << " samples." << std::endl;
	}


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
		int x, y, z = -1;
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
					currentEdgeType = floor(currentEdgeType / 2) * 3;//(currentEdgeType % 2) * 3;
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
	double relationship = 1;
	std::cout << "using Edges " << std::endl;
	for (int i = 1; i < pathsDelay.size(); i++)
	{
		double calcSlack = relationship - pathsDelay[i] - 0.002; // the 0.002 is the clock uncertaininty used by quartus
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
		std::cout << "Good work man Stored slack for path " << i << " is " << storedSlack << " calculated slack is " << calcSlack << std::endl;
	}

	return succ;
}



// validate MC with RE delays
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
			currentEdgeType = floor(currentEdgeType / 2) * 3;//(currentEdgeType % 2) * 3;
			assert(transitionDelay[currentEdgeType] > -1);
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
					currentEdgeType = floor(currentEdgeType / 2) * 3;//(currentEdgeType % 2) * 3;
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

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////// end this copy bitch ///////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// now we have all path delaysa including clock skew, lets compare what we have against the stored paths slack

	bool succ = true;
	double relationship = 1;
	std::cout << "using REs " << std::endl;

	for (int i = 1; i < pathsDelay.size(); i++)
	{
		double calcSlack = relationship - pathsDelay[i] - 0.002; // the 0.002 is the clock uncertaininty used by quartus
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
			std::cout << "Good work man Stored slack for path " << i << " is " << storedSlack << " calculated slack is " << calcSlack << std::endl;
	}

	return succ;

}


