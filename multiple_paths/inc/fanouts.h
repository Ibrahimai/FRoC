/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   fanouts.h
 * Author: jmeijers
 *
 * Created on June 5, 2017, 5:07 PM
 */

#ifndef FANOUTS_H
#define FANOUTS_H

#include "routing_tree.h"
#include "globalVar.h"

//adds fanouts to the routing between two LUTs/FF by adding the needed info to the rc file
void add_fanouts_to_routing(routing_tree & tree, std::map<std::string, int> & branchLabel, std::ofstream & RoFile, std::string node_name);

void edit_files_for_routing(int bitStream);

#endif /* FANOUTS_H */

