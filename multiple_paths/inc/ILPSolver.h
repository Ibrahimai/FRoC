#pragma once
void ILP_solve(std::vector<double> pathsImport, bool use_MC, int bitstreams); // optimize per 1 bit stream
//void ILP_solve_2(std::vector<double> pathsImport, bool use_MC, int bitstreams);
//void ILP_solve_3();
void ILP_solve_max_paths_per_x_bit_stream(int bitStreams, std::vector < std::vector<int> > & pathsSchedule, std::vector<double> pathsImport, bool use_MC);
void ILP_solve_max_timing_edges(std::map<std::string, double>  testedTimingEdgesMap, std::map<std::string, std::vector<Path_logic_component> >  timingEdgeToPaths, std::map<std::string, double>  timingEdgesMapComplete , bool strictEdgeCounting, bool cascadedRegion);