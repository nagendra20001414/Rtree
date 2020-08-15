#include "rtree.h"

Node::Node(int id, int d, int* current_MBR, int parent_id, int maxCap){
    this->id = id;
    this-> d = d;
    this->current_MBR = new int[2*d];
    for (int i=0; i<2*d; i++){
        this->current_MBR[i] = current_MBR[i];
    }
    this->parent_id = parent_id;
    this->maxCap = maxCap;
    this->children = new int[maxCap];
    this->children_MBR = new int*[maxCap];
    for (int i=0; i<maxCap; i++){
        this->children[i] = -1;
        this->children_MBR[i] = new int[2*d];
    }
}