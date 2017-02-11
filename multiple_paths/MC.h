#pragma once
#include <iostream>
#include <random>
#include <map>


// forward class definition
class Path_logic_component;

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



void MC_generate_edges_rand_vars(double dev_per_to_mean, int quartus_delay);
void MC_generate_REs_rand_vars(double dev_per_to_mean, int quartus_delay);
double MC_sim_edges(int num_of_simulations, std::vector<int> untestedPaths, std::vector<int> testedPaths, std::map<std::string, std::vector<Path_logic_component> >  timingEdgeToPaths);
bool MC_validate_edges_delays(std::map<std::string, std::vector<Path_logic_component> > timingEdgeToPaths);
bool MC_validate_RE_delays(std::map<std::string, std::vector<Path_logic_component> > timingEdgeToPaths);