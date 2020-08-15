class Node{
    public:
        int id;
        int d;
        int* current_MBR;
        int parent_id;
        int maxCap;
        int* children;
        int** children_MBR;

        Node(int id, int d, int* current_MBR, int parent_id, int maxCap);

};