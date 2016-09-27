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
void remove_fanin_higher_than_three();
void remove_arithLUT_with_two_inputs_and_no_cin();
std::vector<Path_logic_component> number_of_distinct_inputs_to_lab(int x, int y, double & numberOfDistinctInputs);
void remove_to_match_routing_constraint();
void delete_especial_reconvergent_fanout();
void assign_test_phases_ib();
void generate_pathRelationGraph(std::vector < std::vector <int> > & pathRelationGraph);
void unkown(std::vector < std::vector <int> > & pathRelationGraph, int i, Path_node  tempCell);
void add_connection(std::vector < std::vector <int> > & pathRelationGraph, int x, int y);
int reduce_ALUT_inputs(int x, int y, int z, int topInputs, int bottomInputs);
int reduce_ALM_inputs(int x, int y, int z);
void remove_excess_fan_in();
void handle_port_e();
int reduce_due_to_port_e(int x, int y, int z);
void handle_port_d_shared_with_e(); // handles the case when the created LUT has 6 inputs and one of them is port D from the pin that is also connected to port E (coming from the original circuit). In that case somehting must go because we cannot connect D

void add_cascaded_edges_to_pathRelationGraph(std::vector < std::vector <int> > & pathRelationGraph);// add edges to the PRG to handle cascaded paths