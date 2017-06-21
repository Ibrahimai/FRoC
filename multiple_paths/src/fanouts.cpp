/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "fanouts.h"
#include <sstream>
using namespace std;

//recursively adds fanouts to all routing pieces in a routing tree
void add_node_fanouts(routing_node & node, std::map<std::string, int> & branchLabel,
        std::ofstream & RoFile, std::string node_name, std::string branch_statemnet) {

    //run through all branches, check if they were included in routing
    for (unsigned i = 0; i < node.children.size(); i++) {
        if (branchLabel.find(node.children[i]->get_name()) != branchLabel.end()) {
            //item is already routed as a part of a FRoC path
            //must find the branch statement that points to this node in the rcf file
            //then will call this function to add its children as fanouts or add the fanouts of its children
            ostringstream new_branch;
            new_branch << "label_" << branchLabel[node.children[i]->get_name()] << "_" << node.children[i]->get_name();
            string tmp_string = new_branch.str();
            //recursion on the node
            add_node_fanouts(*node.children[i], branchLabel, RoFile, node_name, tmp_string);
        } else {
            //item is not routed as a FRoC path, must route it as a fanout after branch statement
            //wires, buffers, etc. can be simply added as a piece of routing
            //LUTS, Local Lines, and Local Interconnects can only be placed if there is room in the LAB
            //If there is room, their LUT's placement must be locked down
            //FF's, IO pins, and other items cannot currently be modeled
            if (node.children[i]->type != LCCOMB && node.children[i]->type != FF && node.children[i]->type != LOCAL_LINE && node.children[i]->type != LOCAL_INTERCONNECT && node.children[i]->type != DNM) { 
                //fanouts are simply name FANOUTX with X ranging from 0-> #fanouts - 1
                ostringstream fanout_name;
                fanout_name << "FANOUT" << all_fanouts.size();
                //add the fanout to the rcf file
                //branch to fanout source
                RoFile << "\tbranch_point = " << branch_statemnet << ';' << endl;
                //specify the first piece of routing
                RoFile << '\t' << node.children[i]->get_name() << ';' << endl;
                //following the first piece, allow any routing to get to the destination
                RoFile << "\tzero_or_more, *" << ';' << endl;
                //define the destination (FANOUTX, which is a virtual IO pin -> a hanging LUT that Quartus will not get rid of)
                RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;

                //add the fanout name to the list of fanouts (to be added to the verilog file)
                struct single_fanout tmp_fanout;
                tmp_fanout.fanout_name = fanout_name.str();
                tmp_fanout.node_name = node_name; //name of current node, for assignment in verilog file
                all_fanouts.push_back(tmp_fanout);
                numberOfFanouts++;
            } else if (node.children[i]->type == LCCOMB) { 
                //The lut must be placed to be replicated
                int X, Y, N;
                X = node.children[i]->X;
                Y = node.children[i]->Y;
                N = node.children[i]->N;
                bool placed = false; //true if placement is found
                //check the Node's specific LUT to see if it can be placed there
                if (fpgaLogic[X][Y][N].utilization == 0 && !fanoutLUTPlacement[X][Y][N] && X != 0 && Y != 0) {
                    //The node can be placed at its LUT
                    fanoutLUTPlacement[X][Y][N] = true;
                    //get fanout name
                    ostringstream fanout_name;
                    fanout_name << "FANOUT" << all_fanouts.size();
                    
                    //define the branch point of the fanout
                    RoFile << "\tbranch_point = " << branch_statemnet << ';' << endl;
                    //the branch goes directly from the branch point to the LUT, no need for zero or more * 
                    RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;

                    //make the placement name to be added to the placement constraints file
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
                else if(terminal_LUTS.find(node.children[i]->get_name()) == terminal_LUTS.end()){ //check the node's LAB to see if any LUTs can be used for an approximate replication if it hasn't been replicated already
//                    for (N = 0; N < FPGAsizeZ; N += 2) {
//                        if (fpgaLogic[X][Y][N].utilization == 0 && !fanoutLUTPlacement[X][Y][N] && X != 0 && Y != 0) {
//                            //The node can be placed at its LUT
//                            fanoutLUTPlacement[X][Y][N] = true;
//
//                            ostringstream fanout_name;
//                            fanout_name << "FANOUT" << all_fanouts.size();
//                            
//                            RoFile << "\tbranch_point = " << branch_statemnet << ';' << endl;
//                            RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;
//
//                            ostringstream placement;
//                            placement << "set_location_assignment LCCOMB_X" << X << "_Y" << Y << "_N" << N << " -to " << fanout_name.str();
//
//                            //add the fanout name to the list of fanouts (to be added to the verilog file)
//                            struct single_fanout tmp_fanout;
//                            tmp_fanout.fanout_name = fanout_name.str();
//                            tmp_fanout.node_name = node_name;
//                            tmp_fanout.placement = placement.str();
//                            all_fanouts.push_back(tmp_fanout);
//                            numberOfFanouts++;
//                            numberOfPlacedFanouts++;
//                            placed = true;
//                            break;
//                        }
//                    }
                    if(!placed) numberOfIgnoredFanouts++;
                }
                
            } else if (node.children[i]->type != FF && node.children[i]->type != DNM) { //check to see if Local_Line/Local_Interconnect can be replicated
                //Local Lines and Local Interconnects require a LUT to be placed in their LAB
                int X, Y, N;
                X = node.children[i]->X;
                Y = node.children[i]->Y;
                N = node.children[i]->N;
                bool placed = false;
                //check to see if the fanout can be placed at a specific LUT in the LAB where it needs to be to connect to the Local Line/Interconnect
                for (N = 0; N < FPGAsizeZ; N += 2) {
                    if (fpgaLogic[X][Y][N].utilization == 0 && !fanoutLUTPlacement[X][Y][N] && X != 0 && Y != 0) {
                        //The node can be placed at its LUT
                        fanoutLUTPlacement[X][Y][N] = true;

                        ostringstream fanout_name;
                        fanout_name << "FANOUT" << all_fanouts.size();
                        //state the branch point
                        RoFile << "\tbranch_point = " << branch_statemnet << ';' << endl;
                        //route the fanout
                        RoFile << '\t' << node.children[i]->get_name() << ';' << endl;
                        //most likely unnecessary, but doesn't hurt
                        RoFile << "\tzero_or_more, *" << ';' << endl;
                        //destination is the LUT that was found
                        RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;
                        
                        //placement info for placement constraints file
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

//takes a source node as input and calls the above function to recursively add fanouts to all children
void add_fanouts_to_routing(routing_tree & tree, std::map<std::string, int> & branchLabel, std::ofstream & RoFile, std::string node_name){
    //check to ensure that the source used to call the function does in fact have children that are parts of the path
    //if children that are part of the path are found, add the children's fanouts to the rcf file
    routing_node* node = tree.head; 
    bool map_first_fanouts = false;
    for(unsigned i = 0; i < node->children.size(); i++){
        if(branchLabel.find(node->children[i]->get_name()) != branchLabel.end()){
            //child is part of the FRoC path, get the branch label for the node
            ostringstream new_branch;
            new_branch << "label_" << branchLabel[node->children[i]->get_name()] << "_" << node->children[i]->get_name();
            string tmp_string = new_branch.str();
            add_node_fanouts(*node->children[i], branchLabel, RoFile, node_name, tmp_string);
            map_first_fanouts = true;
            
        }
    }
    
    if(map_first_fanouts) {
        //1 or more of the sources children were part of a FRoC path
        //Therefore the sources other immediate children can be routed
        for (unsigned i = 0; i < node->children.size(); i++) {
            if (branchLabel.find(node->children[i]->get_name()) == branchLabel.end()) {
                //child is not part of a FRoC path and must be modeled
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
                    else if(terminal_LUTS.find(node->children[i]->get_name()) == terminal_LUTS.end()){ //check to ensure the LUT is supposed to be placed
//                        for(N = 0; N < FPGAsizeZ; N += 2){
//                            if (fpgaLogic[X][Y][N].utilization == 0 && !fanoutLUTPlacement[X][Y][N] && X != 0 && Y != 0) {
//                                //The node can be placed at its LUT
//                                fanoutLUTPlacement[X][Y][N] = true;
//
//                                ostringstream fanout_name;
//                                fanout_name << "FANOUT" << all_fanouts.size();
//                                //add the fanout to the rcf file, no need for branch point because the branch is coming directly from the LUT output
//
//                                RoFile << "\tdest = " << '(' << fanout_name.str() << ')' << ';' << endl << endl;
//
//                                ostringstream placement;
//                                placement << "set_location_assignment LCCOMB_X" << X << "_Y" << Y << "_N" << N << " -to " << fanout_name.str();
//
//                                //add the fanout name to the list of fanouts (to be added to the verilog file)
//                                struct single_fanout tmp_fanout;
//                                tmp_fanout.fanout_name = fanout_name.str();
//                                tmp_fanout.node_name = node_name;
//                                tmp_fanout.placement = placement.str();
//                                all_fanouts.push_back(tmp_fanout);
//                                numberOfFanouts++;
//                                numberOfPlacedFanouts++;
//                                placed = true;
//                                break;
//                            }
//                        }
                        if(!placed) numberOfIgnoredFanouts++;
                    }
                    
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
    //open old and new top files
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
    
    //just inside the closing bracket for module top, declare all the fanouts as outputs
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
    //get lines from the old file and print them to the new file until "endmodule" is reached
    while(!(file_line[0] == 'e' && file_line[1] == 'n' && file_line[2] == 'd' && file_line[3] == 'm') &&  !oldVerilogFile.eof()){
        newVerilogFile << file_line << endl;
        getline(oldVerilogFile, file_line);
    }
    
    //add fanout assignments
    newVerilogFile << endl << endl << "//fanout assignments" << endl;
    
    //go through all fanouts and assign them to their respective nodes
    for(unsigned i = 0; i < all_fanouts.size(); i++){
        newVerilogFile << "assign " << all_fanouts[i].fanout_name << " = " << all_fanouts[i].node_name << ";" << endl;
    }
    newVerilogFile << endl << "endmodule";
    
    oldVerilogFile.close();
    newVerilogFile.close();
    
    //open the old location file and append it
    stringstream location_file_name;
    location_file_name << "LocationFile_" << bitStream << ".txt";
    
    ofstream location_file;
    location_file.open(location_file_name.str(), ofstream::app);
    location_file << endl;
    //go through all the fanouts and declare them as virtual I/O's
    //if the fanout has a placement statement, add that too
    for(unsigned i = 0; i < all_fanouts.size(); i++){
        location_file << "set_instance_assignment -name VIRTUAL_PIN ON -to " << all_fanouts[i].fanout_name << endl;
        if (all_fanouts[i].placement != ""){ //check to see if Virtual IO placement needs to be specified
            location_file << all_fanouts[i].placement << endl;
        }
    }
    
    location_file.close();
}