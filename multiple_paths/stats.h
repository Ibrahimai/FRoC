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