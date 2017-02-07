#pragma once
#include "MC.h"
#include "globalVar.h"

std::map<std::string, std::vector < Rand_Edge_Delay > >  timingEdgesRandVars;

//dev_per_to_mean is the ration of dev / mean, quartus_delay is how do we interperet the delay reported by quartus, 1 means mean + sigma
void generate_edges_rand_vars(double dev_per_to_mean, int quartus_delay) 
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
double MC_sim(int num_of_simulations, std::vector<int> untestedPaths, std::vector<int> testedPaths, std::map<std::string, std::vector<int> > timingEdgeToPaths)
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

		// loop over all timing edges to get the delay of each edge and add it to the paths using it
		for (auto iter = timingEdgeToPaths.begin(); iter != timingEdgeToPaths.end(); iter++)
		{
			std::string tempKey = iter->first;
			std::vector<int> currentPaths = iter->second;

			int x, y, z = -1;
			if(tempKey[0]=='C') // if its a cell get the location of it please
				get_location_from_key(tempKey, x, y, z);

			// initially I will just use the max delay of all transisiotnas as the delay of this edge

			double maxDelay = 0.0;

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
					 
				if(z<0 || z%LUTFreq==0) // LUT
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
			std::cout << "Maximum untested path " << longestUntestedPath <<  " delay is " << maxUntestedPathDelay << " maximum tested path " << longestTestedPath << " delay is " << maxTestedPathDelay << std::endl;

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