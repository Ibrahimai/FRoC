#pragma once
void update_currentFabric();
void set_netlist();
void update_paths_complete();
void update_testedPaths(std::vector <std::vector<int> > test_structure);
bool get_allPathsTested(int & remaining);
void print_stats();