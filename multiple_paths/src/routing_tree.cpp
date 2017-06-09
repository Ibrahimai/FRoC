/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <cstdlib>
#include "routing_tree.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include "globalVar.h"

using namespace std;

/*
 * Parses a project's Quartus-produced .rcf and .qsf files to produce a series of
 * tree structures that represent the complete output of a LUT or FF in a design
 * This includes all routing from the LUT or FF, complete with fanouts  
 */


int routing_tree_maker(string rcf_file_loc, string qsf_file_loc) {
    ifstream rcf_file;
    ifstream qsf_file;
    rcf_file.open(rcf_file_loc);
    qsf_file.open(qsf_file_loc);
    
    if(rcf_file.is_open() && qsf_file.is_open()){
        cout << "both open" << endl;
    }
    //routing_tree a;
    //key is the name, the data is the hardware name
    unordered_map<string, string> signal_names;
    
    //the string key will be the hardware type and location, exactly as appears in qsf file
    //declared in globalVar.h
    //unordered_map<string, routing_tree> routing_trees;
    

    //parse through qsf file for all LUTs and FFs
    string temp_line;

    while (!qsf_file.eof()) {
        getline(qsf_file, temp_line);
        if (temp_line[4] == 'l' && temp_line[5] == 'o' && temp_line[6] == 'c' &&
                temp_line[7] == 'a' && temp_line[8] == 't') {
            //line is set_location_assignment
            stringstream line_stream(temp_line);
            line_stream.ignore(100, ' ');
            getline(line_stream, temp_line, ' '); //get the name of the item
            if ((temp_line[0] == 'F' && temp_line[1] == 'F') ||
                    (temp_line[0] == 'L' && temp_line[1] == 'C')) {
                //flip flop
                line_stream.ignore(10, ' '); //ignore "-to"
               string name;
                if(line_stream.peek() == '\"') {
                    //name is in quotes
                    line_stream.get();
                    getline(line_stream, name, '\"');
                    if(line_stream.peek() == '\n') line_stream.get();
                }
                else //name is not in quotes
                    getline(line_stream, name); //get the variable name the FF is linked to
                //link variable name and id in map
                signal_names[name] = temp_line;

                //get position info
                stringstream position_stream(temp_line);
                int X, Y, N;
                position_stream.ignore(100, 'X');
                position_stream >> X;
                position_stream.ignore(100, 'Y');
                position_stream >> Y;
                position_stream.ignore(100, 'N');
                position_stream >> N;
                
                
                Hardware node_type;
                //determine if FF or LUT
                if (temp_line[0] == 'F')
                    node_type = FF;
                else
                    node_type = LCCOMB;
                //head node
                routing_node tmp_node(node_type, X, Y, N, 0, 0);
                //make the routing tree that starts with this node
                routing_tree tmp;
                tmp.set_head(tmp_node);
                //put the tree into the map
                routing_trees[temp_line] = tmp;
                
                //cout << X << ' ' << Y << ' ' << N << ' ' << name << endl;
            } 
        }
    }
    //All ID's are found and put in maps. Now need to read .rcf file
    

    while (!rcf_file.eof()){
        //string is the label, routing_node is pointer to node that the label refers to
        unordered_map<string, routing_node*> labels;
        getline(rcf_file, temp_line);
        if(rcf_file.eof()) break;
        if(temp_line[0] == 's' && temp_line[1] == 'i' && temp_line[12] == '='){
            //signal_name =, line to start a tree
            stringstream start_line(temp_line);
            start_line.ignore(1000, '=');
            if(start_line.peek() == ' ') start_line.get(); //get ' ' after '='
            getline(start_line, temp_line, ' '); //get the identifier
            auto iter = signal_names.find(temp_line); //look for name in the map
            if(iter != signal_names.end()){
                //start signal is one we want to track
                routing_tree* current_root = &routing_trees[(signal_names[temp_line])]; //pointer to the root for the tree we are interested in
                getline(rcf_file, temp_line, '}'); //get whole routing declaration (between { and } )
                stringstream routing_line(temp_line);
                routing_node* current_node = current_root->head;
                routing_node* head_node = current_node;
                
                while(!routing_line.eof()){
                    
                    routing_line.ignore(1000, '\t'); //assumes lines in {} are indented, which they should be
                    getline(routing_line, temp_line, ';'); //get full statement
                    if(routing_line.eof()) break;
                    string label;
                    bool set_new_label = false;
                    if(temp_line[0] == 'l'){
                        //label
                        set_new_label = true;
                        stringstream label_line(temp_line);
                        label_line.ignore(1000, '=');
                        if(label_line.peek() == ' ') label_line.get(); //get '= '
                        getline(label_line, label, ','); //get label name
                        if(label_line.peek() == ' ') label_line.get();
                        getline(label_line, temp_line); //get remainder of line to process
                    }
                    
                    
                    int L = 0;
                    if(temp_line[0] == 'L' || temp_line[0] == 'C' || temp_line[0] == 'R'){
                        //LOCAL_INTERCONNECT/LOCAL_LINE/LE_BUFFER
                        
                        Hardware type;
                        if(temp_line[0] == 'C'){
                            type = C4;
                            stringstream length_stream(temp_line);
                            length_stream.get();
                            length_stream >> L; //get wire length
                        }
                        else if(temp_line[0] == 'R'){
                            type = R4;
                            stringstream length_stream(temp_line);
                            length_stream.get();
                            length_stream >> L; //get wire length
                        }
                        else if(temp_line[6] == 'I'){
                            type = LOCAL_INTERCONNECT;
                        }
                        else if(temp_line[6] == 'L'){
                            type = LOCAL_LINE;
                        }
                        else if(temp_line[6] == 'F'){
                            type = LE_BUFFER;
                        }
                        else {
                            type = DNM;
                        }
                        
                        int X, Y, N, I;
                        
                        stringstream hardware_line(temp_line);
                        hardware_line.ignore(100, 'X');
                        hardware_line >> X;
                        hardware_line.ignore(100, 'Y');
                        hardware_line >> Y;
                        hardware_line.ignore(100, 'S');
                        hardware_line >> N;
                        hardware_line.ignore(100, 'I');
                        hardware_line >> I;
                        
                        
                        //make a new node and add it as a child of the current node
                        routing_node* temp_node = new routing_node(type, X, Y, N, I, L);
                        
                        current_node->children.push_back(temp_node);
                        
                        //next identifier will now come from this node, set it as current node
                        current_node = current_node->children[current_node->children.size() - 1];
                        if(set_new_label){
                            //add new label that points to the node
                            labels[label] = current_node;
                        }
                        
                    }
                    
                    else if(temp_line[0] == 'b'){
                        //branch_point
                        stringstream branch_line(temp_line);
                        branch_line.ignore(1000, '=');
                        if(branch_line.peek() == ' ') branch_line.get();
                        getline(branch_line, label, ';');
                        
                        auto iter = labels.find(label); //look for name in the map
                        if(iter != labels.end()){
                            //set current_node to where the label points
                            current_node = labels[label];
                            
                        }
                        else {
                            cout << "Error, label " << label << " used but not found" << endl;
                            return 1;
                        }
                        
                    }
                    else if(temp_line[0] == 'd' && temp_line[1] == 'e' && temp_line[2] == 's' && temp_line[3] == 't'){
                        //dest
                        stringstream dest_line(temp_line);
                        dest_line.ignore(100, '(');
                        if(dest_line.peek() == ' ') dest_line.get();
                        string dest;
                        getline(dest_line, dest, ',');
                        
                        auto iter = signal_names.find(dest); //look for name in the map
                        if(iter != signal_names.end()){
                            //dest exists, add to current node's children
                            routing_node* tmp_node = new routing_node(*(routing_trees[(signal_names[dest])].head));
                            current_node->children.push_back(tmp_node); 
                        }
                        else{
                            //is a signal we don't track, add a random one to the current node's children
                            routing_node* temp_node = new routing_node(DNM, 0, 0, 0, 0, 0);
                            current_node->children.push_back(temp_node);
                            
                        }
                        
                        current_node = head_node;
                    }
                    
                    
                }
                
                
            }
        }
        
    }
    
    //string name = signal_names["wire7121~0"];
    //print_tree((routing_trees[name]).head, " ");
    
    
    return 0;
}

string routing_node::get_name(){
    ostringstream name;
    if(type == C4){
        name << 'C' << L;
    }
    else if(type == R4){
        name << 'R' << L;
    }
    else if(type == LOCAL_INTERCONNECT){
        name << "LOCAL_INTERCONNECT";
    }
    else if(type == LOCAL_LINE){
        name << "LOCAL_LINE";
    }
    else if(type == LE_BUFFER) {
        name << "LE_BUFFER";
    }
    else if(type == LCCOMB) {
        name << "LCCOMB";
    }
    else if(type == FF) {
        name << "FF";
    }
    else {
        return "*";
    }
    
    name << ':' << 'X' << X << 'Y' << Y;
    
    if(type == FF || type == LCCOMB){
        name << 'N' << N;
    }
    else{
        name << 'S' << N;
        name << 'I' << I;
    }
    
    return name.str();
}