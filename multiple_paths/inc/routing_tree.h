/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   routing_tree.h
 * Author: jmeijers
 *
 * Created on June 5, 2017, 1:41 PM
 */
#ifndef ROUTING_TREE_H
#define ROUTING_TREE_H

#include <iostream>
#include <vector>

//hardware types, DNM stands for DNM, for paths that end in hardware that FRoC doesn't test
enum Hardware {C4, R4, LOCAL_INTERCONNECT, LOCAL_LINE, LE_BUFFER, LCCOMB, FF, DNM};



class routing_node{
public:
    //constructor
    routing_node(Hardware typetype, int XX, int YY, int NN, int II, int LL){
        X = XX;
        Y = YY;
        N = NN;
        I = II;
        L = LL;
        type = typetype;
        
    }
    routing_node(const routing_node & b){
        X = b.X;
        Y = b.Y;
        N = b.N;
        I = b.I;
        L = b.L;
        type = b.type;
    }
    ~routing_node(){
        for(unsigned i = 0; i < this->children.size(); i++){
            delete this->children[i];
        }

    }
    
    std::string get_name();
    
    
    
    //Type of hardware of the node (e.g. wire, buffer, etc.)
    Hardware type;
    //the location identifiers of the hardware
    int X, Y, N, I;
    //length identifier if item is a wire
    int L;
    //The pieces of routing the node is connected to
    std:: vector <routing_node*> children;
    //the paths (of the overarching design) that contain this node
    std:: vector <int> paths;
};

class routing_tree{
public:
    routing_node * head; //either a LCCOMB or FF, source of the whole tree
    void set_head(routing_node b){
        head = new routing_node(b);
    }
    ~routing_tree(){
        delete head;
    }
    routing_tree(const routing_tree & b){
        head = new routing_node(*(b.head));
    }
    routing_tree(){
        head = NULL;
        //nothing to do
    }
    
    routing_tree & operator=(const routing_tree & b){
        if(head){
            delete head;
        }
        
        head = new routing_node(*(b.head));
        return *this;
    }

};

int routing_tree_maker(std::string rcf_file_loc, std::string qsf_file_loc);

#endif /* ROUTING_TREE_H */