#pragma once
#include <fstream>
#include <iostream>
#include "globalVar.h"
//class Path_logic_component;
void create_location_contraint_file(int bitStreamNumber);
void create_WYSIWYGs_file(int bitStreamNumber, std::vector<BRAM>  memories);
//void create_auxil_file(std::vector <Path_logic_component> sinks, std::vector <Path_logic_component> controlSignals, std::vector <Path_logic_component> CoutSignals, std::vector <Path_logic_component> sources); // to be merged with the WYSIWYGs file to complete the source file.
//void create_controller_module(std::vector <Path_logic_component> sinks, std::vector <Path_logic_component> controlSignals, std::vector <Path_logic_component> sources);
void create_or_tree(int inputs, int LUTinputs, int number, std::ofstream& controllerFile);
void create_RCF_file(int bitStreamNumber, std::vector<BRAM>  memories);


bool check_similar_LUT_in_ALM(int i, int j, int k); // returns true if the alm that contains the i,j,k LUT
bool check_c_d_shared(int i, int j, int k); //checks if LUT i,j,k shares port C or D with the other LUT in port D or C


//returns a string that will be assigned to the intermediate signal of the BRAM
// this is used for BRAM as their WYISYWGS are harder
//example of returned string "{1'b0, 1'b0, PATHxNODEx, etc...}"
// the returned string connects thre BRAM to the previous nodes
// if nothing is connected it defaults it to gnd
std::string assign_BRAM_intemediateSignals(BRAM memory, int BRAMportInfo);


// prints a WYSIWYG for a single BRAM (memoryCell) into the verilogFile
void BRAM_WYSIWYG_cycloneIV(BRAM memoryCell, std::ofstream & verilogFile, bool testingBRAMsOnly);

// pritns WysyWigs for all BRAMs, this might be used for testing BRAMs only
void generate_BRAMsWYSYWIGs(std::vector<BRAM>  memories, int bitStreamNumber);

