#pragma once
#include <vector>
#include <map>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>
#undef NDEBUG
#include <assert.h>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <math.h>  
//#include "globalVar.h"
class Path_logic_component;
class Path_node;

#include "util.h"
////////////////////////////////////////
////////////functions//////////////////
///////////////////////////////////////
int remove_fanin_higher_than_three();
int remove_arithLUT_with_two_inputs_and_no_cin();
std::vector<Path_logic_component> number_of_distinct_inputs_to_lab(int x, int y, double & numberOfDistinctInputs);
int remove_to_match_routing_constraint();
void delete_especial_reconvergent_fanout();
void assign_test_phases_ib(bool ILPform);
void generate_pathRelationGraph(std::vector < std::vector <int> > & pathRelationGraph);
void unkown(std::vector < std::vector <int> > & pathRelationGraph, int i, Path_node  tempCell);
void add_connection(std::vector < std::vector <int> > & pathRelationGraph, int x, int y);
int reduce_ALUT_inputs(int x, int y, int z, int topInputs, int bottomInputs);
int reduce_ALM_inputs(int x, int y, int z);
void remove_excess_fan_in();
void handle_port_e();
int reduce_due_to_port_e(int x, int y, int z);
void handle_port_d_shared_with_e(); // handles the case when the created LUT has 6 inputs and one of them is port D from the pin that is also connected to port E (coming from the original circuit). In that case somehting must go because we cannot connect D
int remove_to_fix_off_path_inputs();
void add_cascaded_edges_to_pathRelationGraph(std::vector < std::vector <int> > & pathRelationGraph);// add edges to the PRG to handle cascaded paths
int remove_to_fix_off_path_inputs_of_BRAM();
// add edges for BRAMs
void add_edges_for_BRAMs(std::vector < std::vector <int> > & pathRelationGraph);

int  remove_to_toggle_source();
void remove_feedback_paths();
int remove_LUT_or_FF_in_LE();