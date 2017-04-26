#pragma once
//#include "globalVar.h"

bool check_control_signal_required(int x, int y, int z);
bool check_control_signal_required_second(int x, int y, int z);
bool delete_path(int path);
bool get_feeder(int x, int y, int z, int portIn, int & feederPath, int & feederNode);
bool get_feeder(int x, int y, int z, int & feederPath, int & feederNode);
bool get_feeder_special(int x, int y, int z, int & feederPath, int & feederNode);
bool check_down_link_edge_transition(int i, int j, int k);
bool delete_path_stratix(int path);
int delete_ALUT_port_stratix(int x, int y, int z, int port);
int delete_ALUT_stratix(int x, int y, int z);
bool is_cascaded_reg(int x, int y, int z); // returns true if the register in loc (x,y,z) is a cascaded register. Meaning that it is a sink and a source at the same time
bool reg_free_input(int x, int y, int z); // returns true if the register in loc (x,y,z) has its input free, only a source