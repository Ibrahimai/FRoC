#pragma once

int check_number_of_timing_edges_more();
void check_critical_path_from_Input_toCout();
void check_LE_outputs();
//void count_cascaded_paths();
int count_cascaded_paths(); // returns number of feedback paths
void get_cascaded_paths_inverting_behaviour(); // prints the number of feedback paths with odd number of inversion
void calc_stats();
void print_path_coverage_to_file();
void LUT_inputs_stat();
//void generate_timing_edges_of_all_paths(std::map<std::string, double> & timingEdgeSlack);
void update_timing_edges_of_all_paths(std::map<std::string, double> & timingEdgeSlack);
int count_timing_edges_realistic(std::map<std::string, double>  testedTimingEdgeSlack, std::map<std::string, double>  completeTimingEdgeSlack);
void generate_timing_edges_of_all_paths(std::map<std::string, double> & timingEdgeSlack, std::map<std::string, std::vector<int> > & timingEdgeToPaths);
