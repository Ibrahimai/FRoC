#pragma once
#include <string>

#ifdef StratixV 
void add_redundant_LUT(); // this function check ALMs that are half full (one LUT is used) and that connects to the LUT through port D, in this case to ensure that port D is actually used we reserve the free LUT to prevent the tool from ignoring the routing constraint and using port E instead of D
bool need_to_add_dummy(std::string target);// check if the given blockInputMux (target) connects to the shitty pin
#endif