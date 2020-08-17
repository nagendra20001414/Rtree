#include "rtree.h"
#include "file_manager.h"
#include <iostream>
#include <cstring>
#include <math.h>
#include "constants.h"


//Constructor
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

int* generate_new_mbr(int* current_mbr, int* new_point,int dim){

    int new_mbr[2*dim];
    for(int i=0;i<2*dim;i++){
        if (i<dim){
            new_mbr[i] = std::min(std::min(current_mbr[i],current_mbr[i+dim]),new_point[i]);

        }else{
            new_mbr[i] = std::max(std::max(current_mbr[i],current_mbr[i+dim]),new_point[i-dim]);
            // new_vol *= abs(new_mbr[i]-new_mbr[i-dim]);
        }
    } 
    return new_mbr;
}

int get_volume(int* mbr,int dim){
    int vol = 1;
    for (int i=0;i<dim;i++){
        vol *= (abs(mbr[i+dim]-mbr[i]));
    }
    return vol;
}

bool Node::check_if_leaf(int node_id,int dim,int mC,FileHandler fh){
    Node n = getNode(node_id,dim,mC,fh);
    for (int i=dim;i<2*dim;i++){
        if (n.current_MBR[i] == -1){
            continue;
        }else{
            return false;
        }
    }
    return true;

}

int distance(int* p1, int*p2, int dim){
    int dis = 0;
    for (int i=0;i<2*dim;i++){
        dis+= pow(p1[i]-p2[i],2);
    }
    return dis;
}

void Node::split(int original_node_id,int new_node_id,int dim,int mC,FileHandler fh){
    Node org_node = getNode(original_node_id,dim,mC,fh);

    int* children = org_node.children;
    for (int i=0;i<dim-1;i++){
        
        for (int j=i+1;j<dim;j++){



        }
    }


}

void Node::update_mbr(int* P, int node_id,int dimensionality, int maxCap,FileHandler fh, bool to_split, int new_node_id){
    Node root_node = getNode(node_id,dimensionality,maxCap,fh);
    int num_children=0;
    for (int i=0;i<maxCap;i++){
        if (root_node.children[i]==-1){
            break;
        }else{
            num_children++;
        }
    }

    //A callback to one higher level if the nodes split
    bool to_split = false;
    int new_node_id = new_node_id;
    
    if (num_children<maxCap){

        //just update mbr above

    }else{
        split()
        to_split = true;
        // update_mbr(P,root_node)
    }
    if (root_node.parent_id != -1){
        update_mbr(P,root_node.parent_id,dimensionality,maxCap,fh,to_split,new_node_id);
    }

}


void Node::insert(int* P, int root_id, int dimensionality, int maxCap, FileHandler fh,int new_node_id){
    Node root_node = getNode(root_id,dimensionality,maxCap,fh);
    

    //Will come true if only root and is the first insert
    bool is_leaf = check_if_leaf(root_id,dimensionality,maxCap,fh);

    if (is_leaf){
        //Store P here
    }else{

        bool is_child_leaf = check_if_leaf(children[0],dimensionality,maxCap,fh);




        //Level below this node has leaves
        if (is_child_leaf){
            int num_children=0;
            for (int i=0;i<maxCap;i++){
                if (root_node.children[i]==-1){
                    break;
                }else{
                    num_children++;
                }
            }
            if (num_children<maxCap){
                root_node
            }else{

            }
            

        }else{

            int m = ceil(maxCap/2);
            int* children = root_node.children;

            int best_children;
            int best_volume;
            int min_inc=-1;

            for (int i=0;i<maxCap;i++){
                if (children[i]==-1){
                    break;
                }else{
                    int vol = get_volume(root_node.children_MBR[i],dimensionality);
                    int* new_mbr = generate_new_mbr(root_node.children_MBR[i],P,dimensionality);
                    int new_vol = get_volume(new_mbr,dimensionality);
                    int diff = new_vol-vol;
                    if (min_inc==-1){
                        min_inc = diff;
                        best_children = children[i];
                        best_volume = vol;
                    }else if (min_inc>diff){
                        min_inc= diff;
                        best_children = children[i];
                        best_volume = vol;
                    }else if(min_inc==diff){
                        if (best_volume>vol){
                            best_children = children[i];
                            best_volume = vol;
                        }
                    }
                }
            }
            insert(P,best_children,dimensionality,maxCap,fh,new_node_id);
        }
    }
    
    
}



    // for (int i=0;i<maxCap;i++){
    //     if (children[i]==-1){
    //         break;
    //     }else{
    //         // int diff = mbr_increase(root_node.children_MBR[i],P,dimensionality);
    //         int vol = get_volume(root_node.children_MBR[i],dimensionality);
    //         int* new_mbr = generate_new_mbr(root_node.children_MBR[i],P,dimensionality);
    //         int new_vol = get_volume(new_mbr,dimensionality);
    //         int diff = new_vol-vol;
    //         if (min_inc==-1){
    //             min_inc = diff;
    //             best_children[ind++]=i;
    //         }else if (min_inc>diff){
    //             min_inc= diff;
    //             ind= 0;
    //             best_children[ind++]=i;
    //         }else if(min_inc==diff){
    //             best_children[ind++]=i;
    //         }
    //     }
    // }
    // if (min_inc == -1){ //Every node is empty
    //     root_node.children[0] = new_node_id;
    //     root_node.children_MBR[0] = 

    // }




    // root_node_data = 
// }