/*#include "fpgaAarch.h"
#ifdef CycloneIV
#include "cycloneIV_model.h"
#endif

#ifdef StratixV
#include "StratixV.h"
#endif
*/
//#define maxPerBitStream

#include "globalVar.h"
#include "parseInput.h"
#include "stats.h" 
//#include "util.h" // included in ignore.h
#include "ignore.h"
#include "createOutputFiles.h"
#include "dummyExtras.h"
#include "completeNetlist.h"
#include "ILPSolver.h"
#include "MC.h"


#ifdef CycloneIV
Logic_element fpgaLogic[FPGAsizeX][FPGAsizeY][FPGAsizeZ]; // size of cyclone IV on DE2 board, got it from chip planner, model the logic elements of the chip
														  //std::vector < std::vector <bool>> testingPhases;

#endif
#ifdef StratixV
ALUT fpgaLogic[FPGAsizeX][FPGAsizeY][FPGAsizeZ];
ALM alms[FPGAsizeX][FPGAsizeY][ALMsinLAB];
#endif

// global varoiables in globalVar.h

int numberOfTestPhases;
std::vector <double> pathSlack;
std::vector <double> pathClockSkew;
std::vector <double> pathClockRelation;
std::vector< std::vector<Path_node> > paths; // model the paths
std::ofstream IgnoredPathStats; // delete after obtaining stats
std::ofstream RE_MCSim; // store MC results

std::ofstream TE_MCSim; // store MC results


std::map<std::string, std::vector < Edge_Delay > >  cellEdgesDelay; // map to stroe all cell edges and their delays (FF, FR, RF, RR) of each one

std::map<std::string, std::vector < Edge_Delay > >  timingEdgesDelay; // map to stroe all timing edges and their delays (FF, FR, RF, RR) of each one

std::unordered_map<std::string, std::vector < Edge_Delay > >  REsDelay; // map to store all REs and the delays (FF, FR, RF, RR) of each one

std::vector <double> pathREsDelta; // store the delta delays of the used REs in each path

std::map<std::string, std::vector<RE_logic_component> >  REToPaths; // map to store all RE and for each RE it stores the paths using it. This is used

void set_testing_phase(int fixed, int change)
{
	int phase1 = paths[fixed][0].testPhase;
	int phase2 = paths[change][0].testPhase;
	if ( phase1 != phase2 )
		return;
	paths[change][0].testPhase++;
	if (paths[change][0].testPhase > numberOfTestPhases - 1)
		numberOfTestPhases = paths[change][0].testPhase + 1;
	// change phase 2 to the inimum allowed phase
}

void assign_test_phases() // incorrect algorithm, see assign_test_phases_2 // assuming  one output per LUT (this outpu can have multiple fan-outs)
{
	int i, j,k;
	int tempComponentX, tempComponentY, tempComponentZ;
	Path_logic_component tempNode;
	for (i = 0; i <(int)paths.size(); i++) // loop across all paths, another approach would be to loop across used logic elements
	{
		for (j = 0; j < (int)paths[i].size(); j++) // loop across nodes in that path
		{
			tempComponentX = paths[i][j].x;
			tempComponentY = paths[i][j].y;
			tempComponentZ = paths[i][j].z;
			for (k = 0; k < (int)fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes.size(); k++) // loop across all nodes sharing the LE used by node j in path i
			{
				tempNode = fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes[k];
				if (i == tempNode.path) // samw path
					continue;
				if (paths[i][j].portIn != paths[tempNode.path][tempNode.node].portIn) // diffenent paths and use different inputs
				{
					if (i > tempNode.path) // paths more critical than the current path should have been handeled already
					{
					//	assert(paths[i][0].testPhase != paths[tempNode.path][0].testPhase);
						if (paths[i][0].testPhase == paths[tempNode.path][0].testPhase)
							std::cout << "error ya she2oo \n" << std::endl;
					//	continue;
					}
					// change the testing phase of path tempNode.path
					if (tempNode.path == 300)
						std::cout << "fef" << std::endl;
					set_testing_phase(i, tempNode.path);
				}

			}
			 
		}
	}
}




int clear_le(int x, int y, int z)
{

	int i;
	int totalDeletedPaths = 0;
	for (i = 0; i < (int)fpgaLogic[x][y][z].nodes.size(); i++)
	{
		if (delete_path(fpgaLogic[x][y][z].nodes[i].path))
			totalDeletedPaths++;
	}
	return totalDeletedPaths;
}


int check_number_of_timing_edges()
{
	int i, j, k;
	int sX, sY, sZ, sP, dX, dY, dZ, dP; // source port is the output port of a node, destination port is the output port of the destination
	int pIn;
	std::string tempKey;
	std::map<std::string, double> timingEdgeSlack;
	int bestPath, tempPath, tempNode;
	for (i = 1; i < (int)paths.size(); i++) // loop across all paths
	{
		for (j = 0; j < (int)paths[i].size()-1; j++) // loop across all nodes in each path
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
			dX = paths[i][j+1].x;
			dY = paths[i][j+1].y;
			dZ = paths[i][j+1].z;
			dP = paths[i][j+1].portOut;

			// check if this edge is already considered 
			tempKey = "sX" + std::to_string(sX) + "sY" + std::to_string(sY) + "sZ" + std::to_string(sZ) + "sP" + std::to_string(sP) + "dX" + std::to_string(dX) + "dY" + std::to_string(dY) + "dZ" + std::to_string(dZ) + "dP" + std::to_string(dP);
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
					if (paths[tempPath][tempNode].portIn == pIn && paths[tempPath][tempNode].portOut == dP) // uses the fact that the first path found is the one with lowest slack
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

void delete_all()
{
	for (int i = 0; i <  (int)paths.size(); i++)
		if (paths[i].size()>0)
		{
#ifdef StratixV
			delete_path_stratix(i);
#endif
#ifdef CycloneIV
			delete_path(i);
#endif
		}
			
}


void cycloneIV_stuff(bool & scheduleChanged, std::vector < std::vector<int> > & pathsSchedule, int bitStreams,  int feedbackPaths, int remainingPaths, std::map<std::string, double>  & testedTimingEdgesMap, std::map<std::string, std::vector<Path_logic_component> >  timingEdgeToPaths, std::map<std::string, double>  timingEdgesMapComplete, bool strictCoverage)
{
	int i;
	calc_stats();
	bool casacadedRegion = false;
	if (remainingPaths > feedbackPaths)
		remove_feedback_paths();
	else
	{
		std::cout << "starting risky region" << std::endl;
		casacadedRegion = true;
	}
	// try ibrahim
	//delete_especial_reconvergent_fanout();
	//ILP_solve();
//	ILP_solve_2();
	ILP_solve_max_timing_edges(  testedTimingEdgesMap,  timingEdgeToPaths,  timingEdgesMapComplete, strictCoverage, casacadedRegion);
//	ILP_solve_3();

#ifdef maxPerBitStream
	if (scheduleChanged)
	{
		ILP_solve_max_paths_per_x_bit_stream(bitStreams, pathsSchedule);
		scheduleChanged = false;
	}
	else
	{
		std::cout << "------------------------------- Not going through the solver as we already know the schedule we are deleteing stuff that are not in  " << pathsSchedule.size() - bitStreams << std::endl;
		for (int k = 1; k < paths.size(); k++)
		{
			if (paths[k][0].deleted)
				continue;
			bool pathTested = false;
			for (int j = 0; j < pathsSchedule[pathsSchedule.size() - bitStreams].size(); j++)
			{
				if (k == pathsSchedule[pathsSchedule.size() - bitStreams][j])
				{
					pathTested = true;
					break;
				}
			}
			
			if (!pathTested)
				assert(delete_path(k));
		}
	}
#endif
	

//	if (remove_fanin_higher_than_three()>0) // added to ILP
//		scheduleChanged = true;

	std::cout << "after removing fanin higher than three  number of Luts is ,";
	int totalTimingEdges = check_number_of_timing_edges_more();
	std::cout << "total number of edges " << totalTimingEdges << std::endl;
	check_LE_outputs();

	if(remove_arithLUT_with_two_inputs_and_no_cin() > 0)
		scheduleChanged = true;

	std::cout << "after removing situation when adder uses two inputs no cin and cannot control cin  number of luts is  ,";
	check_LE_outputs();

	if(remove_to_match_routing_constraint() > 0 )
		scheduleChanged = true;

	std::cout << "after removing routing congestion number of LUTs is  ,";
	check_LE_outputs();

//	if(remove_to_fix_off_path_inputs() > 0) // added to ILP
//		scheduleChanged = true;
	
//	if(remove_to_toggle_source() > 0) // added to ILP
//		scheduleChanged = true;


	numberOfTestPhases = 1;
	assign_test_phases_ib();
	std::cout << "after deleting all ignored paths number of LUTs is :" << std::endl;
	check_LE_outputs();
	std::vector <std::vector<int> > test_structure;
	test_structure.resize(numberOfTestPhases);
	//////////////////////////////////////////////////////// stats about path and timing edge coverage
	std::cout << "//////////////\\\\\\\\\\\\ ibrahim number of timing edges tested before = " << testedTimingEdgesMap.size() << std::endl;
	update_timing_edges_of_all_paths(testedTimingEdgesMap);
	std::cout << "//////////////\\\\\\\\\\\\ ibrahim number of timing edges tested after = " << testedTimingEdgesMap.size() << std::endl;
	print_path_coverage_to_file();
	totalTimingEdges = check_number_of_timing_edges_more();
	std::cout << "total number of edges " << totalTimingEdges << std::endl;


	create_location_contraint_file();
	std::cout << "location file created " << std::endl;
	create_WYSIWYGs_file(); // also creates auxillary 
	std::cout << "wysiwyg create " << std::endl;
							//create_controller_module();
	create_RCF_file();

	std::cout << "rcf created " << std::endl;

	for (i = 1; i < (int)paths.size(); i++)
	{
		if (!paths[i][0].deleted)
			test_structure[paths[i][0].testPhase].push_back(i);
		else
		{
			assert(paths[i][0].testPhase == -1);
		}
	}
	unsigned int j;
	std::cout << " test phases look like : " << std::endl;
	for (i = 0; i < (int)test_structure.size(); i++)
	{
		std::cout << "Phase " << i << " has " << test_structure[i].size() << " paths and they are : ";
		for (j = 0; j < test_structure[i].size(); j++)
		{
			std::cout << test_structure[i][j] << " ";
		}
		std::cout << std::endl;
	}


	// update information of what paths are tested
	update_testedPaths(test_structure);
	update_paths_complete();
	update_currentFabric();



}


int remainig_paths(std::vector< std::vector<Path_node> > paths_total)
{
	int total = 0;
	for (int i = 1; i <(int)paths_total.size(); i++)
	{

		if (!paths_total[i][0].deleted)
			total++;
	}

	return total;
}

#ifdef StratixV 
void stratixV_stuff()
{
	int i;
	//	int totalTimingEdges;
	///// finished parsing
	int sharedPorts = 0;
	int shitMan = 0;
	///////////////////////////// stat functions before deleting
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ; k++)
			{
				//			if (fpgaLogic[i][j][k].utilization < 1) // not utilized
				//				continue;

				for (int index = 0; index < LUTinputSize; index++)
				{
					if (fpgaLogic[i][j][k].isInputPortShared[index]) // this input is shared
					{
						sharedPorts++;
						if (fpgaLogic[i][j][k].sharedWith[index] != index)
						{
							shitMan++;
						}

					}
				}
			}
		}
	}
	LUT_inputs_stat();
	calc_stats();


	remove_excess_fan_in(); // deletes stuff
	calc_stats();
	//delete_all();
	calc_stats();
	handle_port_e();  // deletes stuff
	calc_stats();
	std::cout << "********************************************** srcond time *****************************" << std::endl;
	calc_stats();
	//remove_excess_fan_in();
	//calc_stats();
	assign_test_phases_ib();
	std::vector <std::vector<int> > test_structure;
	test_structure.resize(numberOfTestPhases);
	int testedPaths = 0;
	for (i = 1; i <(int)paths.size(); i++)
	{
		if (!paths[i][0].deleted)
		{
			test_structure[paths[i][0].testPhase].push_back(i);
			testedPaths++;
		}
		else
		{
			assert(paths[i][0].testPhase == -1);
		}
	}
	//	read_routing(argv[2]);

	handle_port_d_shared_with_e();  // deletes stuff
	calc_stats();

	create_WYSIWYGs_file();
	create_location_contraint_file();
	create_RCF_file();


	add_redundant_LUT();// must be called after location constraint file function is called, as it appends to the file where as location constraint file overwrites the text file(location file).



	
}
#endif
int main(int argc, char* argv[])
{
	
	if (parseIn(argc, argv) != 1)
	{
		std::cout << "Error incorrect input parameters, expecting <metaData> <routing data> <routing data of calibration bitstream> <toplevel name used to wrtie output ot text file>";
		return 0;
	}
	std::cout << "what the fack fewfwef wrgregt54r" << std::endl;
	//IgnoredPathStats.open("stats_File.txt");

	std::string temp =  argv[4];
	std::string stats_file_name = temp + "_stats.txt";
	std::cout << timingEdgesDelay.size() << std::endl;
	IgnoredPathStats.open(stats_file_name);
	IgnoredPathStats << "ILP" << "\t" << "#ofIn" << "\t" << "adder" << "\t" << "routng" << "\t" << "offPath" << "\t" << "toggl_src" << "\t" << "reconv_fnout" << "\t" << "tested timing edges relaxed" << "\t" << "testing timing edges strict" <<std::endl;
	set_netlist(); // set original copy of netlist (without deleting anything)
	int remainingPaths = paths.size() - 1; // paths[0] is not really a path
	int feedbackPaths = count_cascaded_paths();
	get_cascaded_paths_inverting_behaviour();

	// generate timing edges info before deleting any thing

	// store all timing edges and its associated longest delay
	std::map<std::string, double>  timingEdgesMapComplete;

	// store all timing edges and all paths using each edge
	std::map<std::string, std::vector<Path_logic_component> >  timingEdgeToPaths;

	generate_timing_edges_of_all_paths(timingEdgesMapComplete, timingEdgeToPaths);

	bool  scheduleChanged = true;
	std::vector < std::vector<int> > pathsSchedule;
	int x = 6;
#ifdef MCsim
	assert(timingEdgesDelay.size() == timingEdgesMapComplete.size());


	std::cout << "True number of timing edges including IC & Cell delays : " << timingEdgesMapComplete.size() << std::endl;

	// structure to store tested timing edges
	std::map<std::string, double>  testedTimingEdgesMap;

	int number_of_samples = 5000;
	
	bool strictCoverage = true;
	double sigma;
	double qDelayInter;

	std::string REMC_file_name = temp + "_RE_MCsim.txt";
	std::string TEMC_file_name = temp + "_TE_MCsim.txt";
	RE_MCSim.open(REMC_file_name);
	TE_MCSim.open(TEMC_file_name);

	if (argc > 6)
	{
		number_of_samples = atoi(argv[6]);
		sigma = atof(argv[7]);
		qDelayInter = atof(argv[8]);
	}

	print_paths_delays(temp);
	run_MC(number_of_samples, strictCoverage, timingEdgesMapComplete, testedTimingEdgesMap, timingEdgeToPaths, remainingPaths, sigma, qDelayInter);
	return 0;
#endif
	while (true)
	{
	//	scheduleChanged = true;
		cycloneIV_stuff(scheduleChanged, pathsSchedule, x,feedbackPaths, remainingPaths, testedTimingEdgesMap, timingEdgeToPaths, timingEdgesMapComplete, strictCoverage);
		IgnoredPathStats << testedTimingEdgesMap.size() << "\t";
		IgnoredPathStats << count_timing_edges_realistic(testedTimingEdgesMap, timingEdgesMapComplete) << "\t" << std::endl;

		int testedEdgesTillNow;

		if (strictCoverage)
			testedEdgesTillNow = count_timing_edges_realistic(testedTimingEdgesMap, timingEdgesMapComplete);
		else
			testedEdgesTillNow = testedTimingEdgesMap.size();

		get_allPathsTested(remainingPaths);			
		///////////////////////////////// MC stuff begin ////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef MCsim
		int testedEdges;
		if (strictCoverage)
			testedEdges = count_timing_edges_realistic(testedTimingEdgesMap, timingEdgesMapComplete);
		else
			testedEdges = testedTimingEdgesMap.size();

			if (testedEdges == timingEdgesMapComplete.size()) // then all edges has been tested
		{
			/*	run_MC(number_of_samples, strictCoverage, timingEdgesMapComplete, testedTimingEdgesMap, timingEdgeToPaths, remainingPaths);

			get_allPathsTested(remainingPaths);

			std::cout << "All edges were tested but still there remains the following number of untested paths :-  " << remainingPaths << "untested " << std::endl;
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

			assert(remainingPaths == unTestedPaths.size());

			// starting MC simulation

			MC_generate_edges_rand_vars(0.05, 3);
			MC_generate_REs_rand_vars(0.05, 3);

			if (MC_validate_RE_delays(timingEdgeToPaths))
				std::cout << "Alles clar " << std::endl;
			else
				assert(false);

			if (MC_validate_edges_delays(timingEdgeToPaths))
				std::cout << "Alles clar " << std::endl;
			else
				assert(false);

			double failureProb = MC_sim_edges(number_of_samples, unTestedPaths, testedPaths, timingEdgeToPaths);
			//		double failureProb = 0.0;
			std::cout << "failure rate with timing edges is " << failureProb << std::endl;

			failureProb = MC_sim_RE(number_of_samples, unTestedPaths, testedPaths, timingEdgeToPaths);
			//		double failureProb = 0.0;
			std::cout << "failure rate with timing edges is " << failureProb << std::endl;*/
			return 0;
		}
#endif // MCsim

		///////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////// MC stuff end ////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////

		if (get_allPathsTested(remainingPaths))// || (testedEdgesTillNow == timingEdgesMapComplete.size()))
		{
			print_stats(argv);
			count_timing_edges_realistic(testedTimingEdgesMap, timingEdgesMapComplete);
		//	if (x == 0)
		//	{
				std::cout << "############################################" << std::endl << "##############################################" << std::endl;
				std::cout << "############################################" << std::endl << "##############################################" << std::endl;
				std::cout << "      " << remainingPaths << "remainig" << std::endl;
				std::cout << "############################################" << std::endl << "##############################################" << std::endl;
				std::cout << "############################################" << std::endl << "##############################################" << std::endl;
		//	}
			break;
		}
		else
		{
			std::cout << "############################################" << std::endl << "##############################################" << std::endl;
			std::cout << "############################################" << std::endl << "##############################################" << std::endl;
			std::cout << "      " << remainingPaths << "remainig" << std::endl;
			std::cout << "############################################" << std::endl << "##############################################" << std::endl;
			std::cout << "############################################" << std::endl << "##############################################" << std::endl;




			

			if (remainingPaths == feedbackPaths)
				set_number_of_bitstreams_ohne_feedback();
		}

	}

#ifdef StratixV
	stratixV_stuff();
	check_routing(argv[3]);


	if (compare_routing())
		std::cout << "routing check passed maaaaaan NAIIIIIICE" << std::endl;
	else
		std::cout << "routing check failed see previous line " << std::endl;
#endif
	/*
	
	
		int i;
//	int totalTimingEdges;
	///// finished parsing
	int sharedPorts = 0;
	int shitMan = 0;
	///////////////////////////// stat functions before deleting
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ; k++)
			{
	//			if (fpgaLogic[i][j][k].utilization < 1) // not utilized
	//				continue;

				for (int index = 0; index < LUTinputSize; index++)
				{
					if (fpgaLogic[i][j][k].isInputPortShared[index]) // this input is shared
					{
						sharedPorts++;
						if (fpgaLogic[i][j][k].sharedWith[index] != index)
						{
							shitMan++;
						}

					}
				}
			}
		}
	}
	LUT_inputs_stat();
	calc_stats();


	remove_excess_fan_in(); // deletes stuff
	calc_stats();
	//delete_all();
	calc_stats();
	handle_port_e();  // deletes stuff
	calc_stats();
	std::cout << "********************************************** srcond time *****************************" << std::endl;
	calc_stats();
	//remove_excess_fan_in();
	//calc_stats();
	assign_test_phases_ib();
	std::vector <std::vector<int> > test_structure;
	test_structure.resize(numberOfTestPhases);
	int testedPaths = 0;
	for (i = 1; i <(int)paths.size(); i++)
	{
		if (!paths[i][0].deleted)
		{ 
			test_structure[paths[i][0].testPhase].push_back(i);
			testedPaths++;
		}
		else
		{
			assert(paths[i][0].testPhase == -1);
		}
	}
//	read_routing(argv[2]);

	handle_port_d_shared_with_e();  // deletes stuff
	calc_stats();

	create_WYSIWYGs_file();
	create_location_contraint_file();
	create_RCF_file();


	add_redundant_LUT();// must be called after location constraint file function is called, as it appends to the file where as location constraint file overwrites the text file(location file).

	
	*/
/*
	///////////////////////////////////////////// pre-synthesis work to remove paths
	remove_fanin_higher_than_three();
	std::cout << "after removing fanin higher than three  number of Luts is ,";
	totalTimingEdges = check_number_of_timing_edges_more();
	std::cout << "total number of edges " << totalTimingEdges << std::endl;
	check_LE_outputs();
	remove_arithLUT_with_two_inputs_and_no_cin();
	std::cout << "after removing situation when adder uses two inputs no cin and cannot control cin  number of luts is  ,";
	check_LE_outputs();
	remove_to_match_routing_constraint();
	std::cout << "after removing routing congestion number of LUTs is  ,";
	check_LE_outputs();


	numberOfTestPhases = 1;
	assign_test_phases_ib();
	std::cout << "after deleting all ignored paths number of LUTs is :" << std::endl;
	check_LE_outputs();
	std::vector <std::vector<int> > test_structure;
	test_structure.resize(numberOfTestPhases);


	//////////////////////////////////////////////////////// stats about path and timing edge coverage
	print_path_coverage_to_file();
	totalTimingEdges = check_number_of_timing_edges_more();
	std::cout << "total number of edges " << totalTimingEdges << std::endl;




	///////////////////////////////////////// read in routing constraints. This should be done after deleting paths NOT before
	read_routing(argv[2]);



	/////////////////////////////////////////// create files
	create_location_contraint_file();
	create_WYSIWYGs_file(); // also creates auxillary , controller module
	create_RCF_file();


	for (i = 1; i <(int)paths.size(); i++)
	{
		if (!paths[i][0].deleted)
			test_structure[paths[i][0].testPhase].push_back(i);
		else
		{
			assert(paths[i][0].testPhase == -1);
		}
	}
	unsigned int j;
	std::cout << " test phases look like : " << std::endl;
	for (i = 0; i < (int)test_structure.size(); i++)
	{
		std::cout << "Phase " << i << " has " << test_structure[i].size() << " paths and they are : ";
		for (j = 0; j < test_structure[i].size(); j++)
		{
			std::cout << test_structure[i][j] << " ";
		}
		std::cout << std::endl;
	}
	std::vector<int> deletedPaths;

	
	int sharedPorts = 0;
	int shitMan = 0;
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ; k++)
			{
				if (fpgaLogic[i][j][k].utilization < 1) // not utilized
					continue;

				for (int index = 0; index < LUTinputSize; index++)
				{
					if (fpgaLogic[i][j][k].isInputPortShared[index]) // this input is shared
					{
						sharedPorts++;
						if (fpgaLogic[i][j][k].sharedWith[index] != index)
						{
							shitMan++;
						}

					}
				}
			}
		}
	}*/
	return 0;
}