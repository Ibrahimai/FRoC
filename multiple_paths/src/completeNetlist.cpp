#include "globalVar.h"
#include "completeNetlist.h"
#include "util.h"

std::vector< std::vector<Path_node> > paths_complete; // model the paths
Logic_element fpgaLogic_complete[FPGAsizeX][FPGAsizeY][FPGAsizeZ]; // size of cyclone IV on DE2 board, got it from chip planner, model the logic elements of the chip
std::vector< std::vector<int> > testedPaths; // 2-d array of tested path, index is bitstream and this gives a vector of the tested paths at the corresponding bitSteam
std::vector< std::vector <int > > testPathsDistribution;// first index gives number of bit stream, this gives access to a vector of int, the ith element in this vector is the number of paths tested at the ith phase.

int bitstreamsOhneFeedback = -1;

void set_netlist()// sets the complete netlist, should be called immediately after parsing the input and before deleting any paths
{

	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ; k++)
			{
				fpgaLogic_complete[i][j][k] = fpgaLogic[i][j][k];
			}
		}
	}
	paths_complete = paths;

	testedPaths.clear();
	testedPaths.resize(0);
	testPathsDistribution.clear();
	testPathsDistribution.resize(0);
}


void update_paths_complete() // this is called after deleting paths and the whole process, this function marks the tested path as tested
{
	for (int i = 0; i < (int)paths.size(); i++)
	{
		if (paths[i].size()>0)
		{
			if (!paths[i][0].deleted) // if this path is not deleted in the current bitstream, then it is tesed so mark it as tested
			{
				assert(!paths_complete[i][0].tested); // make sure that path i was not tested before
				paths_complete[i][0].tested = true;
			}
		}

	}
}

void update_testedPaths(std::vector <std::vector<int> > test_structure) // test structure has the index of test phase and test_structure[i] contains all paths tested at phase i;
{
	std::vector<int> tested_path_in_current_bitstream; // all paths tested in the current bitstream
	std::vector <int> tested_path_in_phase;
	tested_path_in_current_bitstream.resize(0);
	tested_path_in_phase.resize(0);

	for (int i = 0; i < (int)test_structure.size(); i++)
	{
		tested_path_in_phase.push_back((int)test_structure[i].size());
		for (int j = 0; j < (int)test_structure[i].size(); j++)
		{
			tested_path_in_current_bitstream.push_back(test_structure[i][j]);
		}
	}

	/// double check
	int total = 0;
	for (int i = 0; i < (int)tested_path_in_phase.size(); i++)
	{
		total += tested_path_in_phase[i];
	}
	assert(total == (int)tested_path_in_current_bitstream.size());
	///// end double check

	testedPaths.push_back(tested_path_in_current_bitstream);
	testPathsDistribution.push_back(tested_path_in_phase);
}

void update_currentFabric() // resets the current global variables to the original netlst
{
	///// reset
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ; k++)
			{
				fpgaLogic[i][j][k] = fpgaLogic_complete[i][j][k];
			}
		}
	}
	paths = paths_complete;


	////// start deleting tested paths

	for (int i = 0; i <(int)paths_complete.size(); i++)
	{
		for (int j = 0; j < (int)paths_complete[i].size(); j++)
		{
			if (paths_complete[i][0].tested)
			{
				assert(delete_path(i));
				break;
			}
			else
			{
				break;
			}
		}
	}
}


void set_number_of_bitstreams_ohne_feedback()
{

	bitstreamsOhneFeedback = (int)testedPaths.size();

}

bool get_allPathsTested(int & remaining) // returns true if all paths are tested, false otherwise and remaining indicate the number of untested paths
{
	remaining = 0;
	for (int i = 0; i <(int)paths_complete.size(); i++)
	{
		for (int j = 0; j < (int)paths_complete[i].size(); j++)
		{
			if (!paths_complete[i][0].tested)
			{
				remaining++;
				break;
			}
		}
	}

	if (remaining==0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void print_stats(char* argv[])
{
	std::cout << "**********************************************************DONE BIAAAATCH**************************************************" << std::endl;
	std::cout << "Stats on the genereated bit streams are:- " << std::endl;
	std::cout << "A) " << testedPaths.size() << " Bitstreams are required to test 100% of the given paths. " << std::endl;
	std::cout << "B) The number of paths at each bit stream and the number of test phases are as follows:- " << std::endl;
	for (int i = 0; i < (int)testPathsDistribution.size(); i++)
	{
		std::cout << "\t Bitstream number " << i << " has " << testPathsDistribution[i].size() << " test phases and " << testedPaths[i].size() << " tested paths" << std::endl;;
		for (int j = 0; j < (int)testPathsDistribution[i].size(); j++)
		{
			std::cout << "\t\t Test phase number " << j << " has " << testPathsDistribution[i][j] << " paths" << std::endl;
		}
	}

	IgnoredPathStats << "=============================================================================================================" << std::endl;
	IgnoredPathStats << "=============================================================================================================" << std::endl;
	IgnoredPathStats << "=============================================================================================================" << std::endl << std::endl;

	IgnoredPathStats << "#\tPa\tPh" << std::endl;
	for (int i = 0; i < (int)testPathsDistribution.size(); i++)
	{
		IgnoredPathStats <<  i << "\t" << testedPaths[i].size() << "\t" << testPathsDistribution[i].size() << std::endl;;

	}

	// arv[4] top_level
	std::ofstream output_summary;
	std::string temp = argv[4];
	std::string stats_file_name = temp + "_summary.txt";
	output_summary.open(stats_file_name);

	output_summary << stats_file_name << std::endl;
	output_summary << "Number of Paths : " << paths.size() - 1 << std::endl;
	output_summary << "Critical Path Delay : " << pathSlack[1] << " , Best slack is : " << pathSlack[pathSlack.size() - 1] << std::endl;
	output_summary << "Calibration Bitstreams Required : " << testPathsDistribution.size() << std::endl;
	output_summary << "To test all paths while ignoring the cascaded paths we needed " << bitstreamsOhneFeedback << std::endl;
	output_summary << "#\tPh\tPa" << std::endl;

	for (int i = 0; i < (int)testPathsDistribution.size(); i++)
	{
		output_summary <<  i << "\t" << testPathsDistribution[i].size() << "\t" << testedPaths[i].size() << "\t" << std::endl;
	}



	return;
}