/*ibrahim*/
#pragma once
//#define MCsim
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
extern std::vector <double> pathSlack; // stores paths alck used to determine wether an edge is tested through it most critical path or not

extern std::vector <double> pathClockSkew;

extern std::vector< std::vector<Path_node> > paths; // model the paths

extern std::ofstream IgnoredPathStats; // dleete after obtaining stats

extern std::map<std::string, std::vector < Edge_Delay > >  timingEdgesDelay; // map to stroe all timing edges and the delays (FF, FR, RF, RR) of each one

