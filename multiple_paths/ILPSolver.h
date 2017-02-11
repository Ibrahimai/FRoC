#pragma once
void ILP_solve();
void ILP_solve_2();
void ILP_solve_3();
void ILP_solve_max_paths_per_x_bit_stream(int bitStreams, std::vector < std::vector<int> > & pathsSchedule);
void ILP_solve_max_timing_edges(std::map<std::string, double>  testedTimingEdgesMap, std::map<std::string, std::vector<Path_logic_component> >  timingEdgeToPaths, std::map<std::string, double>  timingEdgesMapComplete , bool strictEdgeCounting, bool cascadedRegion);