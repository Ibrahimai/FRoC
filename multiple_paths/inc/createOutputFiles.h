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
// if nothing is connected it defaults it to 1 except for WE it defaults to gnd
std::string assign_BRAM_intemediateSignals(BRAM memory, int BRAMportInfo, int memIndex, std::vector<BRAM> memoriesList, bool & portUsed);


// prints a WYSIWYG for a single BRAM (memoryCell) into the verilogFile
std::string BRAM_WYSIWYG_cycloneIV(BRAM memoryCell, std::ofstream & verilogFile,
	bool testingBRAMsOnly, std::vector<BRAM>  memories,
	std::vector <Path_logic_component> & sinks, int bitStreamNumber);

// pritns WysyWigs for all BRAMs, this might be used for testing BRAMs only
void generate_BRAMsWYSYWIGs(std::vector<BRAM>  memories, int bitStreamNumber);

// return true if BRAM at i and j has a controller
// returnts the controller type and the size of the tested port
bool isBRAMwithController(int i, int j, int & testedPortSize, int & controllerType, std::vector<BRAM>  memories);

// returns the size of the port port of the BRAM located in x & y
// ignores the weird case of 2 bram wysiwygs located in the same pgysical BRAM
int getBRAMPortSize(int x, int y, int BRAMportInfo, std::vector<BRAM>  memories);

