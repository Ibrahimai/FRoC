/*ibrahim*/
#pragma once
#include "fpgaAarch.h"
#ifdef CycloneIV
#include "cycloneIV_model.h"
#endif

#ifdef StratixV
#include "StratixV.h"
#endif

#ifdef CycloneIV
extern Logic_element fpgaLogic[FPGAsizeX][FPGAsizeY][FPGAsizeZ]; // size of cyclone IV on DE2 board, got it from chip planner, model the logic elements of the chip
														  //std::vector < std::vector <bool>> testingPhases;
#endif
#ifdef StratixV
extern ALUT fpgaLogic[FPGAsizeX][FPGAsizeY][FPGAsizeZ];
extern ALM alms[FPGAsizeX][FPGAsizeY][ALMsinLAB];
#endif

//extern Logic_element fpgaLogic[FPGAsizeX][FPGAsizeY][FPGAsizeZ]; // size of cyclone IV on DE2 board, got it from chip planner, model the logic elements of the chip
																 //std::vector < std::vector <bool>> testingPhases;
extern int numberOfTestPhases;
extern std::vector <double> pathSlack;
extern std::vector< std::vector<Path_node> > paths; // model the paths

extern std::ofstream IgnoredPathStats; // dleete after obtaining stats

extern std::map<std::string, std::vector < Edge_Delay > >  timingEdgeToPaths;

