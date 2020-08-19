#include "file_manager.h"

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

        int numNodesPerPage(int dimensionality, int maxCap);

        Node getNode(int id, int dimensionality, int maxCap, FileHandler fh);

        void storeNode(int id,FileHandler fh,int dime,int maxCap, Node n);


        bool check_if_leaf(int node_id,int dim,int mC,FileHandler fh);


        Node split(int orginal_node_id,int* new_node_id,int dim,int mC,FileHandler fh,Node new_node_to_add);

        std::tuple<bool,Node> insert(int* P, int root_id, int dimensionality, int maxCap, FileHandler fh,int* new_node_id);

        bool PointQuery(int* P, int node_id, int dimensionality, int maxCap, FileHandler fh);

        bool is_leaf(Node node, int maxCap){
            for (int i=0; i<maxCap; i++){
                if (node.children[i]!=-1){
                    return false;
                }
            }
            return true;
        };

};