/*ibrahim*/
#pragma once
#define MCsim

// MC stuff
#define NOCORELATION 0
#define FULLCORELATION 1
#define PARTIALCORELATION 2

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

////todo: group all these paths* vectors into one data structure
extern std::vector <double> pathSlack; // stores paths alck used to determine wether an edge is tested through it most critical path or not

extern std::vector <double> pathClockSkew; // stores clock skew of each path, which we use in calculating the slack 

extern std::vector <double> pathREsDelta; // store the delta delays of the used REs in each path

extern std::vector <double> pathClockRelation; // store the clock setup relation ship of  each path

extern std::vector< std::vector<Path_node> > paths; // model the paths

extern std::ofstream IgnoredPathStats; // dleete after obtaining stats

extern std::ofstream RE_MCSim; // store MC results

extern std::ofstream TE_MCSim; // store MC results

// monteCarlo stuff

extern std::map<std::string, std::vector < Edge_Delay > >  timingEdgesDelay; // map to stroe all timing edges and their delays (FF, FR, RF, RR) of each one

extern std::map<std::string, std::vector < Edge_Delay > >  cellEdgesDelay; // map to stroe all timing edges and their delays (FF, FR, RF, RR) of each one

extern std::unordered_map<std::string, std::vector < Edge_Delay > >  REsDelay; // map to store all REs and the delays (FF, FR, RF, RR) of each one

extern std::map<std::string, std::vector<RE_logic_component> >  REToPaths; // map to store all RE and for each RE it stores the path and node using it. This is used
