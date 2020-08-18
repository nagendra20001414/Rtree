#include "rtree.h"
#include "file_manager.h"
#include <iostream>
#include <cstring>
#include <math.h>
#include <tuple>
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
//Function to store a node in a page
void Node::storeNode(int id,FileHandler fh,int dim,int maxCap,Node n){
    int page_num = int(id/numNodesPerPage(dim,maxCap));
    int page_mod = id%numNodesPerPage(dim,maxCap);
    int node_size = sizeof(int) * (2+(2*dim)+maxCap+(maxCap*2*dim));
    // int int_inc = sizeof(int)/sizeof(char); //Need to check this
    int start_pos = page_mod*node_size;
    PageHandler ph;
    if (page_mod ==0){
        ph = fh.NewPage();
    }else{
        ph = fh.PageAt(page_num);
    }
    char* data = ph.GetData();
    memcpy(&data[start_pos++],&(n.id),sizeof(int));
    // start_pos++;
    memcpy(&data[start_pos],&(n.current_MBR),sizeof(2*dim));
    start_pos+=2*dim;
    memcpy(&data[start_pos++],&(n.parent_id),sizeof(int));
    memcpy(&data[start_pos],&(n.children),sizeof(int)*maxCap);
    start_pos+=maxCap;
    memcpy(&data[start_pos],&(n.children_MBR),sizeof(int)*maxCap*dim*2);
}

//generates MBR from an old MBR and d-dimensional point
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

//generates MBR from an old MBR and another MBR
// int* generate_new_mbr_2d(int* current_mbr,int* new_mbr, int dim){
//     int new_mbr[2*dim];
//     for (int i=0;i<2*dim;i++){

//     }
// }

//Function to generate volume from an MBR
int get_volume(int* mbr,int dim){
    int vol = 1;
    for (int i=0;i<dim;i++){
        vol *= (abs(mbr[i+dim]-mbr[i]));
    }
    return vol;
}

//Given a node id, is it a leaf?
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
//Distance between 2 MBRs
int distance(int* p1, int*p2, int dim){
    int dis = 0;
    for (int i=0;i<2*dim;i++){
        dis+= pow(p1[i]-p2[i],2);
    }
    return dis;
}

//Splits a node into 2. Returns the new node
Node Node::split(int original_node_id,int *new_node_id,int dim,int mC,FileHandler fh,Node new_node_to_add){
    Node org_node = getNode(original_node_id,dim,mC,fh);

    int min_fill = ceil(mC/2);
    //getting the farthest seeds
    int e1,e2;
    int max_distance = -1;
    int* children = org_node.children;
    bool checked_new_node = false;
    for (int i=0;i<mC-1;i++){
        if (org_node.children[i] == -1){
            break;
        }
        checked_new_node = false;
        for (int j=i+1;j<mC;j++){
            if (org_node.children[j] == -1 && checked_new_node==true){
                break;
            }else if(org_node.children[j]==-1){
                checked_new_node=true;
                int d = distance(org_node.children_MBR[i],new_node.current_MBR,dim);
                if (d>max_distance){
                    e1 = i;
                    e2 = mC;
                    max_distance = d;
                }
            }else{
                int d = distance(org_node.children_MBR[i],org_node.children_MBR[j],dim);
                if (d>max_distance){
                    e1 = i;
                    e2 = j;
                    max_distance = d;
                }
            }
             
        }
    }

    //selecting which key goes to which
    bool groups[mC+1];
    groups[e1]=true;
    groups[e2]=false;
    int count1=1,count2=1;

    for (int i=0;i<mC+1;i++){
        if (i==e1||i==e2){
            continue;
        }else{
            if (count1==min_fill){
                for(int j=i;j<mC+1;j++){
                    if (j==e1 || j==e2){
                        continue;
                    }else{
                        groups[j]=true;
                    }
                }
                break;
            }else if (count2==min_fill){

                for(int j=i;j<mC+1;j++){
                    if (j==e1 || j==e2){
                        continue;
                    }else{
                        groups[j]=false;
                    }
                }
                break;                
            }
            //See Step 13 of insert. and complete this portion. 
            //be mindful that when j==mC we look at the new_node
            int d1 =0, d2 = 0;
            if (d1<d2){
                //to fill
            }else{
                //to fill
            }
        }
    }

    //We now have groups with true meaning the new_node and false meaning the old_node
    int numC2=0;
    int numC1=0;
    int new_node_MBR[2*dim];
    Node new_node = Node(*new_node_id,dim,new_node_MBR,-1,mC);
    bool c1 = false;
    bool c2 = false;
    for (int i=0;i<mC+1;i++){
        bool t = groups[i];

        if (t){
            if (c2==false){
                c2= true;
                new_node.current_MBR = org_node.children_MBR[i]; //Since this'll never run for i==mC it is ok to not check.
            }else{
                //To add!
                // new_node.current_MBR = generate_new_mbr_2d
            }
            if (i!=mC){
                new_node.children[numC2]=org_node.children[i];
                new_node.children_MBR[numC2++] = org_node.children_MBR[i];
                Node child_node = getNode(org_node.children[i],dim,mC,fh);
                child_node.parent_id = *new_node_id;
            }else{
                new_node.children[numC2]=new_node_to_add.id;
                new_node.children_MBR[numC2++] = new_node_to_add.current_MBR;
                new_node_to_add.parent_id = *new_node_id;
            }

        }else{
            if (c1==false){
                c1= true;
                org_node.current_MBR = org_node.children_MBR[i]; //Since this'll never run for i==mC it is ok to not check.
            }else{
                //TO add!
                // org_ndoe.current_MBR = generate_new_mbr_2d 
            }
            if (i!=mC){
                org_node.children[numC1]=org_node.children[i];
                org_node.children_MBR[numC1++] = org_node.children_MBR[i];
                Node child_node = getNode(org_node.children[i],dim,mC,fh);
                child_node.parent_id = original_node_id;
            }else{
                new_node.children[numC1]=new_node_to_add.id;
                new_node.children_MBR[numC1++] = new_node_to_add.current_MBR;
                new_node_to_add.parent_id = original_node_id;
            }
        }
    }
    new_node_id++;

    return new_node;
}

//The main insert function
std::tuple<bool,Node> Node::insert(int* P, int root_id, int dimensionality, int maxCap, FileHandler fh,int *new_node_id){
    Node root_node = getNode(root_id,dimensionality,maxCap,fh);
    

    //Will come true if only root and is the first insert
    bool is_leaf = check_if_leaf(root_id,dimensionality,maxCap,fh);

    if (is_leaf){
        //Store P here
    }else{

        bool is_child_leaf = check_if_leaf(children[0],dimensionality,maxCap,fh);
        int num_children=0;
        for (int i=0;i<maxCap;i++){
            if (root_node.children[i]==-1){
                break;
            }else{
                num_children++;
            }
        }


        //Level below this node has leaves
        if (is_child_leaf){
            Node new_node = Node(*new_node_id,dimensionality,P,root_id,maxCap);
            if (num_children<maxCap){
                root_node.children[num_children] = *new_node_id;
                root_node.children_MBR[num_children] = P;
                new_node_id++;
                return std::make_tuple(false,NULL); //to change
            }else{
                new_node_id++;
                Node ns = split(root_id,new_node_id,dimensionality,maxCap,fh,new_node);
                return std::make_tuple(true,ns);
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
                        best_children = i;
                        best_volume = vol;
                    }else if (min_inc>diff){
                        min_inc= diff;
                        best_children = i;
                        best_volume = vol;
                    }else if(min_inc==diff){
                        if (best_volume>vol){
                            best_children = i;
                            best_volume = vol;
                        }
                    }
                }
            }
            //This'll return <bool,Node>. Even if bool is false, still need to update the MBR(both children and yourself)
            std::tuple<bool,Node> tup = insert(P,children[best_children],dimensionality,maxCap,fh,new_node_id);
            bool b = std::get<0>(tup);
            Node n = std::get<1>(tup);
            if (b==false){
                //update MBR here
                root_node.children_MBR[best_children] = generate_new_mbr(root_node.children_MBR[best_children],P,dimensionality);
                root_node.current_MBR = generate_new_mbr(root_node.current_MBR,P,dimensionality);
                return std::make_tuple(false,NULL);
            }else{
                if (num_children<maxCap){
                    root_node.children_MBR[best_children] = generate_new_mbr(root_node.children_MBR[best_children],P,dimensionality);
                    root_node.current_MBR = generate_new_mbr(root_node.current_MBR,P,dimensionality);
                    root_node.children[num_children] = n.id;
                    root_node.children_MBR[num_children] = n.current_MBR;
                    return std::make_tuple(false,NULL);
                }else{
                    //update happens in split itself.
                    Node ns = split(root_id,new_node_id,dimensionality,maxCap,fh,n);
                    return std::make_tuple(true,ns);
                    //Need to update as well as split

                }

            }
        }
    }
    
    
}


