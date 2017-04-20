#pragma once
#include <fstream>
#include <iostream>
//class Path_logic_component;
void create_location_contraint_file();
void create_WYSIWYGs_file();
//void create_auxil_file(std::vector <Path_logic_component> sinks, std::vector <Path_logic_component> controlSignals, std::vector <Path_logic_component> CoutSignals, std::vector <Path_logic_component> sources); // to be merged with the WYSIWYGs file to complete the source file.
//void create_controller_module(std::vector <Path_logic_component> sinks, std::vector <Path_logic_component> controlSignals, std::vector <Path_logic_component> sources);
void create_or_tree(int inputs, int LUTinputs, int number, std::ofstream& controllerFile);
void create_RCF_file();


bool check_similar_LUT_in_ALM(int i, int j, int k); // returns true if the alm that contains the i,j,k LUT
bool check_c_d_shared(int i, int j, int k); //checks if LUT i,j,k shares port C or D with the other LUT in port D or C