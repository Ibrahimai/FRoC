#pragma once
void ILP_solve();
void ILP_solve_2();
void ILP_solve_3();
void ILP_solve_max_timing_edges(std::map<std::string, double>  testedTimingEdgesMap, std::map<std::string, std::vector<int> >  timingEdgeToPaths, std::map<std::string, double>  timingEdgesMapComplete , bool strictEdgeCounting);