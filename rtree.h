#include "file_manager.h"
#include "tuple"
#include <cstring>

class Node{
    public:
        int id;
        //MBR stored as first d coordinates as 1 point and next d coordinates correspond to the second point
        //Compare between ith and (i+dimensonality)th coordinates, the first point has the lower value

        //For Leaf nodes d+1th location to 2*d-1th location are all -1
        int* current_MBR;
        int parent_id;
        int* children;
        int** children_MBR;

        Node(int id, int dimensionality, int* current_MBR, int parent_id, int maxCap);

        Node(int id, int dimensionality, int maxCap){
            this->id = id;
            this->current_MBR = new int[2*dimensionality];
            for (int i=0;i<2*dimensionality;i++){
                this->current_MBR[i] = -1;
            }
            this->parent_id = -1;
            this->children = new int[maxCap];
            this->children_MBR = new int*[maxCap];
            for (int i=0; i<maxCap; i++){
                this->children[i] = -1;
                this->children_MBR[i] = new int[2*dimensionality];
            }
        };


        // in terms of size of char
        int size_of_node(int dimensionality, int maxCap){
            int size = sizeof(int)/sizeof(char) * (2+(2*dimensionality)+maxCap+(maxCap*2*dimensionality));
            return size;
        };





        Node split(int orginal_node_id,int& new_node_id,int dim,int mC,FileHandler& fh,Node new_node_to_add);

        bool is_leaf(Node node, int maxCap, int dimensionality){
            if (node.children[0]==-1){
                return true;
            }
            for (int dimension=0; dimension<dimensionality; dimension++){
                if (node.current_MBR[dimension]!=node.current_MBR[dimension+dimensionality]){
                    return false;
                }
            }
            
            return false;
        };


};

Node getNode(int id, int dimensionality, int maxCap, FileHandler& fh);
void storeNode(int id,FileHandler& fh,int dime,int maxCap, Node &n);
std::tuple<bool,Node> insert(int* P, int root_id, int dimensionality, int maxCap, FileHandler& fh,int& new_node_id);

