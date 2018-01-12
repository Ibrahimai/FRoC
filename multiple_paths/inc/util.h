#pragma once
//#include "globalVar.h"
#include <utility> 
#include <vector>
bool check_control_signal_required(int x, int y, int z);
bool check_control_signal_required_second(int x, int y, int z);
bool delete_path(int path);
bool get_feeder(int x, int y, int z, int portIn, int & feederPath, int & feederNode);
bool get_feeder(int x, int y, int z, int & feederPath, int & feederNode);
bool get_feeder_special(int x, int y, int z, int & feederPath, int & feederNode);
bool get_feederPort_from_BRAM(int x, int y, int z, int portIn, std::string & BRAMoutputPort, int & BRAMoutputPortIndex);


bool check_down_link_edge_transition(int i, int j, int k);
bool delete_path_stratix(int path);
int delete_ALUT_port_stratix(int x, int y, int z, int port);
int delete_ALUT_stratix(int x, int y, int z);
bool is_cascaded_reg(int x, int y, int z); // returns true if the register in loc (x,y,z) is a cascaded register. Meaning that it is a sink and a source at the same time
bool reg_free_input(int x, int y, int z); // returns true if the register in loc (x,y,z) has its input free, only a source
int reverseNumber(int n);
bool get_BRAM_feeder(
	int x, // x loc
	int y, // y loc
	int z, // z loc, should always be zero as it's a BRAM
	std::pair <int, int> BRAMportInputInfo,  // .first represnts the port, .second is the index
	int & feederPath,  // the path feeder for the specific pin
	int & feederNode); // the node feeder for the specific pin


bool get_BRAM_feeder_special(
	int x, // x loc
	int y, // y loc
	int z, // z loc, should always be zero as it's a BRAM
	std::pair <int, int> BRAMportInputInfo,  // .first represnts the port, .second is the index
	int & feederPath,  // the path feeder for the specific pin
	int & feederNode); // the node feeder for the specific pin

std::string portNumbertoName(int BRAMportInfo);
