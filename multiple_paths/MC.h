#pragma once
#include <iostream>
#include <random>
#include <map>

class Rand_Edge_Delay
{
public:
	int type;
	std::normal_distribution<double> randVars;
	Rand_Edge_Delay()
	{
		type = -1;
		std::normal_distribution<double> randVarstemp(5.0, 2.0);
		randVars = randVarstemp;
	}

	Rand_Edge_Delay(int ed, std::normal_distribution<double> randVarstemp)
	{
		type = ed;
		randVars = randVarstemp;
	}

};



void generate_edges_rand_vars(double dev_per_to_mean, int quartus_delay);
double MC_sim(int num_of_simulations, std::vector<int> untestedPaths, std::vector<int> testedPaths, std::map<std::string, std::vector<int> >  timingEdgeToPaths);