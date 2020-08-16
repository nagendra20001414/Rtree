#include "rtree.h"
#include "file_manager.h"
#include<iostream>
#include<cstring>
#include "constants.h"

Node::Node(int id, int dimensionality, int* current_MBR, int parent_id, int maxCap){
    this->id = id;
    this->current_MBR = new int[2*dimensionality];
    for (int i=0; i<2*dimensionality; i++){
        this->current_MBR[i] = current_MBR[i];
    }
    this->parent_id = parent_id;
    this->children = new int[maxCap];
    this->children_MBR = new int*[maxCap];
    for (int i=0; i<maxCap; i++){
        this->children[i] = -1;
        this->children_MBR[i] = new int[2*dimensionality];
    }
}

int Node::numNodesPerPage(int dimensionality, int maxCap){
    int size = sizeof(int) * (2+(2*dimensionality)+maxCap+(maxCap*2*dimensionality));
    return int(PAGE_CONTENT_SIZE/size);
}

Node Node::getNode(int id, int dimensionality, int maxCap, FileHandler fh){
    PageHandler ph = fh.PageAt(int(id/numNodesPerPage(dimensionality, maxCap)));
    char *data = ph.GetData();
    int int_increment = sizeof(int)/sizeof(char);
    int node_id;
    memcpy(&data[0], &node_id, sizeof(int));
    int* current_MBR = new int[2*dimensionality];
    memcpy(&data[int_increment], &current_MBR[0], 2*dimensionality*sizeof(int));
    int parent_id;
    memcpy(&data[2*dimensionality*int_increment], &parent_id, sizeof(int));
    int* children = new int[maxCap];
    memcpy(&data[((2*dimensionality)+1)*int_increment], &children[0], maxCap*sizeof(int));
    Node myNode = Node(id, dimensionality, current_MBR, parent_id, maxCap);
    memcpy(&data[((2*dimensionality)+1+maxCap)*int_increment], &myNode.children_MBR[0], maxCap*2*dimensionality*sizeof(int));
    return myNode;
}

void Node::insert(int* P, int root_id){
    // root_node_data = 
}