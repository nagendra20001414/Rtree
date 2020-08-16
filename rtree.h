#include "file_manager.h"

class Node{
    public:
        int id;
        int* current_MBR;
        int parent_id;
        int* children;
        int** children_MBR;

        Node(int id, int dimensionality, int* current_MBR, int parent_id, int maxCap);

        int numNodesPerPage(int dimensionality, int maxCap);

        Node getNode(int id, int dimensionality, int maxCap, FileHandler fh);

        void insert(int* P, int root_id);

};