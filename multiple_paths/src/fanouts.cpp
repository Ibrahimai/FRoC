/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "fanouts.h"
#include <sstream>
using namespace std;

void add_node_fanouts(routing_node & node, std::map<std::string, int> & branchLabel,
        std::ofstream & RoFile, std::string node_name, std::string branch_statemnet) {

    //run through all branches, check if they were included in routing
    for (unsigned i = 0; i < node.children.size(); i++) {
        if (branchLabel.find(node.children[i]->get_name()) != branchLabel.end()) {
            //item is already mapped, route it's children after finding branch statement
            ostringstream new_branch;
            new_branch << "label_" << branchLabel[node.children[i]->get_name()] << "_" << node.children[i]->get_name();
            string tmp_string = new_branch.str();
            add_node_fanouts(*node.children[i], branchLabel, RoFile, node_name, tmp_string);
        } else {
            //item is not mapped, must route it as a fanout after branch statement
            if (node.children[i]->type != LCCOMB && node.children[i]->type != FF && node.children[i]->type != LOCAL_LINE && node.children[i]->type != LOCAL_INTERCONNECT && node.children[i]->type != DNM) { //can't add LUTs / FFs to routing file
                ostringstream fanout_name;
                fanout_name << "FANOUT" << all_fanouts.size();
                //add the fanout to the rcf file
                RoFile << "\tbranch_point = " << branch_statemnet << ';' << endl;
                RoFile << '\t' << node.children[i]->get_name() << ';' << endl;
                RoFile << "\tzero_or_more, *" << ';' << endl;
                RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;

                //add the fanout name to the list of fanouts (to be added to the verilog file)
                struct single_fanout tmp_fanout;
                tmp_fanout.fanout_name = fanout_name.str();
                tmp_fanout.node_name = node_name;
                all_fanouts.push_back(tmp_fanout);
                numberOfFanouts++;
            } else if (node.children[i]->type == LCCOMB) { //check to see if LUT can be replicated
                int X, Y, N;
                X = node.children[i]->X;
                Y = node.children[i]->Y;
                N = node.children[i]->N;
                bool placed = false;
                if (fpgaLogic[X][Y][N].utilization == 0 && !fanoutLUTPlacement[X][Y][N] && X != 0 && Y != 0) {
                    //The node can be placed at its LUT
                    fanoutLUTPlacement[X][Y][N] = true;

                    ostringstream fanout_name;
                    fanout_name << "FANOUT" << all_fanouts.size();
                    
                    RoFile << "\tbranch_point = " << branch_statemnet << ';' << endl;
                    RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;

                    ostringstream placement;
                    placement << "set_location_assignment LCCOMB_X" << X << "_Y" << Y << "_N" << N << " -to " << fanout_name.str();

                    //add the fanout name to the list of fanouts (to be added to the verilog file)
                    struct single_fanout tmp_fanout;
                    tmp_fanout.fanout_name = fanout_name.str();
                    tmp_fanout.node_name = node_name;
                    tmp_fanout.placement = placement.str();
                    all_fanouts.push_back(tmp_fanout);
                    numberOfFanouts++;
                    numberOfPlacedFanouts++;
                    placed = true;
                } else {
                    for (N = 0; N < FPGAsizeZ; N += 2) {
                        if (fpgaLogic[X][Y][N].utilization == 0 && !fanoutLUTPlacement[X][Y][N] && X != 0 && Y != 0) {
                            //The node can be placed at its LUT
                            fanoutLUTPlacement[X][Y][N] = true;

                            ostringstream fanout_name;
                            fanout_name << "FANOUT" << all_fanouts.size();
                            
                            RoFile << "\tbranch_point = " << branch_statemnet << ';' << endl;
                            RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;

                            ostringstream placement;
                            placement << "set_location_assignment LCCOMB_X" << X << "_Y" << Y << "_N" << N << " -to " << fanout_name.str();

                            //add the fanout name to the list of fanouts (to be added to the verilog file)
                            struct single_fanout tmp_fanout;
                            tmp_fanout.fanout_name = fanout_name.str();
                            tmp_fanout.node_name = node_name;
                            tmp_fanout.placement = placement.str();
                            all_fanouts.push_back(tmp_fanout);
                            numberOfFanouts++;
                            numberOfPlacedFanouts++;
                            placed = true;
                            break;
                        }
                    }
                }
                if(!placed) numberOfIgnoredFanouts++;
            } else if (node.children[i]->type != FF && node.children[i]->type != DNM) { //check to see if Local_Line/Local_Interconnect can be replicated
                int X, Y, N;
                X = node.children[i]->X;
                Y = node.children[i]->Y;
                N = node.children[i]->N;
                bool placed = false;
                for (N = 0; N < FPGAsizeZ; N += 2) {
                    if (fpgaLogic[X][Y][N].utilization == 0 && !fanoutLUTPlacement[X][Y][N] && X != 0 && Y != 0) {
                        //The node can be placed at its LUT
                        fanoutLUTPlacement[X][Y][N] = true;

                        ostringstream fanout_name;
                        fanout_name << "FANOUT" << all_fanouts.size();
                        RoFile << "\tbranch_point = " << branch_statemnet << ';' << endl;
                        if (node.children[i]->type != DNM) //only route it if it is to a known connection, otherwise, route it randomly
                            RoFile << '\t' << node.children[i]->get_name() << ';' << endl;
                        RoFile << "\tzero_or_more, *" << ';' << endl;
                        RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;

                        ostringstream placement;
                        placement << "set_location_assignment LCCOMB_X" << X << "_Y" << Y << "_N" << N << " -to " << fanout_name.str();

                        //add the fanout name to the list of fanouts (to be added to the verilog file)
                        struct single_fanout tmp_fanout;
                        tmp_fanout.fanout_name = fanout_name.str();
                        tmp_fanout.node_name = node_name;
                        tmp_fanout.placement = placement.str();
                        all_fanouts.push_back(tmp_fanout);
                        numberOfFanouts++;
                        numberOfPlacedFanouts++;
                        placed = true;
                        break;
                    }
                }
                if(!placed) numberOfIgnoredFanouts++;
            }
            else {
                //item could not be replicated
                numberOfImpossibleFanouts++;
            }
        }
    }
}


void add_fanouts_to_routing(routing_tree & tree, std::map<std::string, int> & branchLabel, std::ofstream & RoFile, std::string node_name){
    //check if beginning is included in branch labels at all. If not, don't include fanouts
    routing_node* node = tree.head; 
    bool map_first_fanouts = false;
    for(unsigned i = 0; i < node->children.size(); i++){
        if(branchLabel.find(node->children[i]->get_name()) != branchLabel.end()){
            ostringstream new_branch;
            new_branch << "label_" << branchLabel[node->children[i]->get_name()] << "_" << node->children[i]->get_name();
            string tmp_string = new_branch.str();
            add_node_fanouts(*node->children[i], branchLabel, RoFile, node_name, tmp_string);
            map_first_fanouts = true;
            
        }
    }
    
    if(map_first_fanouts) {
        for (unsigned i = 0; i < node->children.size(); i++) {
            if (branchLabel.find(node->children[i]->get_name()) == branchLabel.end()) {
                if (node->children[i]->type != LCCOMB && node->children[i]->type != FF && node->children[i]->type != LOCAL_LINE && node->children[i]->type != LOCAL_INTERCONNECT && node->children[i]->type != DNM) {
                    ostringstream fanout_name;
                    fanout_name << "FANOUT" << all_fanouts.size();
                    //add the fanout to the rcf file, no need for branch point because the branch is coming directly from the LUT output
                    RoFile << '\t' << node->children[i]->get_name() << ';' << endl;
                    RoFile << "\tzero_or_more, *" << ';' << endl;
                    RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;

                    //add the fanout name to the list of fanouts (to be added to the verilog file)
                    struct single_fanout tmp_fanout;
                    tmp_fanout.fanout_name = fanout_name.str();
                    tmp_fanout.node_name = node_name;
                    all_fanouts.push_back(tmp_fanout);
                    numberOfFanouts++;
                }
                else if (node->children[i]->type == LCCOMB){ //check to see if LUT can be replicated
                    int X, Y, N;
                    X = node->children[i]->X;
                    Y = node->children[i]->Y;
                    N = node->children[i]->N;
                    bool placed = false;
                    
                    if(fpgaLogic[X][Y][N].utilization==0 && !fanoutLUTPlacement[X][Y][N] && X != 0 && Y != 0){
                        //The node can be placed at its LUT
                        fanoutLUTPlacement[X][Y][N] = true;

                        ostringstream fanout_name;
                        fanout_name << "FANOUT" << all_fanouts.size();
                        //add the fanout to the rcf file, no need for branch point because the branch is coming directly from the LUT output
                        
                        RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;

                        ostringstream placement;
                        placement << "set_location_assignment LCCOMB_X" << X << "_Y" << Y << "_N" << N << " -to " << fanout_name.str();
                        
                        //add the fanout name to the list of fanouts (to be added to the verilog file)
                        struct single_fanout tmp_fanout;
                        tmp_fanout.fanout_name = fanout_name.str();
                        tmp_fanout.node_name = node_name;
                        tmp_fanout.placement = placement.str();
                        all_fanouts.push_back(tmp_fanout);
                        numberOfFanouts++;
                        numberOfPlacedFanouts++;
                        placed = true;
                    }
                    else{
                        for(N = 0; N < FPGAsizeZ; N += 2){
                            if (fpgaLogic[X][Y][N].utilization == 0 && !fanoutLUTPlacement[X][Y][N] && X != 0 && Y != 0) {
                                //The node can be placed at its LUT
                                fanoutLUTPlacement[X][Y][N] = true;

                                ostringstream fanout_name;
                                fanout_name << "FANOUT" << all_fanouts.size();
                                //add the fanout to the rcf file, no need for branch point because the branch is coming directly from the LUT output

                                RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;

                                ostringstream placement;
                                placement << "set_location_assignment LCCOMB_X" << X << "_Y" << Y << "_N" << N << " -to " << fanout_name.str();

                                //add the fanout name to the list of fanouts (to be added to the verilog file)
                                struct single_fanout tmp_fanout;
                                tmp_fanout.fanout_name = fanout_name.str();
                                tmp_fanout.node_name = node_name;
                                tmp_fanout.placement = placement.str();
                                all_fanouts.push_back(tmp_fanout);
                                numberOfFanouts++;
                                numberOfPlacedFanouts++;
                                placed = true;
                                break;
                            }
                        }
                    }
                    if(!placed) numberOfIgnoredFanouts++;
                }
                else if (node-> children[i]->type != FF && node->children[i]->type != DNM){ //check to see if Local_Line/Local_Interconnect can be replicated
                    int X, Y, N;
                    X = node->children[i]->X;
                    Y = node->children[i]->Y;
                    N = node->children[i]->N;
                    bool placed = false;

                    for (N = 0; N < FPGAsizeZ; N += 2) {
                        if (fpgaLogic[X][Y][N].utilization == 0 && !fanoutLUTPlacement[X][Y][N] && X != 0 && Y != 0) {
                            //The node can be placed at its LUT
                            fanoutLUTPlacement[X][Y][N] = true;

                            ostringstream fanout_name;
                            fanout_name << "FANOUT" << all_fanouts.size();
                            //add the fanout to the rcf file, no need for branch point because the branch is coming directly from the LUT output
                            if (node->children[i]->type != DNM) //only route it if it is to a known connection, otherwise, route it randomly
                                RoFile << '\t' << node->children[i]->get_name() << ';' << endl;
                            RoFile << "\tzero_or_more, *" << ';' << endl;
                            RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;

                            ostringstream placement;
                            placement << "set_location_assignment LCCOMB_X" << X << "_Y" << Y << "_N" << N << " -to " << fanout_name.str();

                            //add the fanout name to the list of fanouts (to be added to the verilog file)
                            struct single_fanout tmp_fanout;
                            tmp_fanout.fanout_name = fanout_name.str();
                            tmp_fanout.node_name = node_name;
                            tmp_fanout.placement = placement.str();
                            all_fanouts.push_back(tmp_fanout);
                            numberOfFanouts++;
                            numberOfPlacedFanouts++;
                            placed = true;
                            break;
                        }
                    }
                    if(!placed) numberOfIgnoredFanouts++;
                }
                else {
                    //fanout could not be placed
                    numberOfImpossibleFanouts++;
                }
            }
        }
    }
    
}

void edit_files_for_routing(int bitStream){
    ofstream newVerilogFile;
    ifstream oldVerilogFile;
    stringstream verilogFileName;
    verilogFileName << "top_" << bitStream << ".v";
    oldVerilogFile.open(verilogFileName.str());
    verilogFileName.str(string());
    verilogFileName << "top_" << bitStream << "_fanout.v";
    newVerilogFile.open(verilogFileName.str());
    
    char next_character;
    next_character = oldVerilogFile.get();
    while(next_character != ')'){ //look for end of module top declaration to add outputs
        newVerilogFile.put(next_character);
        next_character = oldVerilogFile.get();
    }
    
    for(unsigned i = 0; i < all_fanouts.size(); i++){
        newVerilogFile << ", output " << all_fanouts[i].fanout_name;
    }
    
    while(next_character != '\n'){ //finish the module declaration
        newVerilogFile.put(next_character);
        next_character = oldVerilogFile.get();
    }
    newVerilogFile.put(next_character); //add the newline
    
    string file_line;
    getline(oldVerilogFile, file_line);
    //go through until endmodule
    while(!(file_line[0] == 'e' && file_line[1] == 'n' && file_line[2] == 'd' && file_line[3] == 'm') &&  !oldVerilogFile.eof()){
        newVerilogFile << file_line << endl;
        getline(oldVerilogFile, file_line);
    }
    
    newVerilogFile << endl << endl << "//fanout assignments" << endl;
    
    for(unsigned i = 0; i < all_fanouts.size(); i++){
        newVerilogFile << "assign " << all_fanouts[i].fanout_name << " = " << all_fanouts[i].node_name << ";" << endl;
    }
    newVerilogFile << endl << "endmodule";
    
    oldVerilogFile.close();
    newVerilogFile.close();
    
    stringstream location_file_name;
    location_file_name << "LocationFile_" << bitStream << ".txt";
    
    ofstream location_file;
    location_file.open(location_file_name.str(), ofstream::app);
    location_file << endl;
    for(unsigned i = 0; i < all_fanouts.size(); i++){
        location_file << "set_instance_assignment -name VIRTUAL_PIN ON -to " << all_fanouts[i].fanout_name << endl;
        if (all_fanouts[i].placement != ""){
            location_file << all_fanouts[i].placement << endl;
        }
    }
    
    location_file.close();
}