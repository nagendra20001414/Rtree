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

int numNodesPerPage(int dimensionality, int maxCap){
    int size = sizeof(int) * (2+(2*dimensionality)+maxCap+(maxCap*2*dimensionality));
    return int(PAGE_CONTENT_SIZE/size);
}

Node getNode(int id, int dimensionality, int maxCap, FileHandler fh){
    int int_increment = sizeof(int)/sizeof(char);
    int page_num = int(id/numNodesPerPage(dimensionality, maxCap));
    PageHandler ph = fh.PageAt(page_num);
    int page_offset = id % numNodesPerPage(dimensionality, maxCap);
    page_offset *= (((2*dimensionality)+2+maxCap+(maxCap*2*dimensionality))*int_increment);
    int read_pointer = page_offset;
    char *data = ph.GetData();
    int node_id;
    // memcpy(&data[page_offset], &node_id, sizeof(int));
    memcpy(&node_id, &data[read_pointer],  sizeof(int));
    read_pointer+=int_increment;
    int* current_MBR = new int[2*dimensionality];
    // memcpy(&data[page_offset+int_increment], &current_MBR, 2*dimensionality*sizeof(int));
    memcpy(current_MBR, &data[read_pointer],  2*dimensionality*sizeof(int));
    read_pointer+=(2*dimensionality)*int_increment;
    int parent_id;
    // memcpy(&data[page_offset+2*dimensionality*int_increment], &parent_id, sizeof(int));
    memcpy(&parent_id,&data[read_pointer],  sizeof(int));
    read_pointer+=int_increment;
    int* children = new int[maxCap];
    // memcpy(&data[page_offset+((2*dimensionality)+1)*int_increment], &children, maxCap*sizeof(int));
    memcpy(children, &data[read_pointer],  maxCap*sizeof(int));
    read_pointer+=maxCap*int_increment;

    Node myNode = Node(id, dimensionality, current_MBR, parent_id, maxCap);
    for (int i=0;i<maxCap;i++){
        memcpy(myNode.children_MBR[i],&data[read_pointer],sizeof(int)*2*dimensionality);
        read_pointer+=2*dimensionality*int_increment;
    }
    // memcpy(&data[page_offset+((2*dimensionality)+2+maxCap)*int_increment], &myNode.children_MBR[0], maxCap*2*dimensionality*sizeof(int));

    fh.UnpinPage(page_num);
    // fh.FlushPages();
    return myNode;
}
//Function to store a node in a page
void storeNode(int id,FileHandler fh,int dim,int maxCap,Node n){
    int page_num = int(id/numNodesPerPage(dim,maxCap));
    int page_mod = id%numNodesPerPage(dim,maxCap);
    int node_size = sizeof(int) * (2+(2*dim)+maxCap+(maxCap*2*dim));
    int int_inc = sizeof(int)/sizeof(char); 
    int start_pos = page_mod*node_size;

    std::cout<<"Page Num: "<<page_num<<" page_mod: "<<page_mod<<" node_size:"<<node_size<<" int_inc: "<<int_inc<<std::endl;
    PageHandler ph;
    if (page_mod ==0){
        ph = fh.NewPage();
    }else{
        ph = fh.PageAt(page_num);
    }
    char* data = ph.GetData();

    memcpy(&data[start_pos],&(n.id),sizeof(int));
    start_pos+=int_inc;

    memcpy(&data[start_pos],(n.current_MBR),sizeof(int)*2*dim);

    start_pos+=2*dim*int_inc;
    memcpy(&data[start_pos],&(n.parent_id),sizeof(int));

    start_pos+=int_inc;

    memcpy(&data[start_pos],(n.children),sizeof(int)*maxCap);

    
    start_pos+=maxCap*int_inc;
    for (int i=0;i<maxCap;i++){
        memcpy(&data[start_pos],(n.children_MBR[i]),sizeof(int)*2*dim);

        start_pos+=2*dim*int_inc;
    }
    
    fh.MarkDirty(page_num);
    fh.FlushPage(page_num);
    // fh.FlushPages();
}

//generates MBR from an old MBR and d-dimensional point
int* generate_new_mbr(int* current_mbr, int* new_point,int dim){

    int* new_mbr = new int[2*dim];
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

// generates MBR from an old MBR and another MBR
void generate_new_mbr_2d(int* mbr1,int* mbr2, int* new_mbr,int dim){
    // int new_mbr[2*dim];
    for (int i=0;i<dim;i++){
        new_mbr[i] = std::min(std::min(mbr1[i],mbr1[i+dim]),std::min(mbr2[i],mbr2[i+dim]));
        new_mbr[i+dim] = std::max(std::max(mbr1[i],mbr1[i+dim]),std::max(mbr2[i],mbr2[i+dim]));
    }
    // return new_mbr;
}

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
    for (int i=0;i<dim;i++){
        if (n.current_MBR[i] == n.current_MBR[i+dim]){
            continue;
        }else{
            return false;
        }
    }
    return true;

}
//Distance between 2 points
int distance_points(int* p1, int*p2, int dim){
    int dis = 0;
    for (int i=0;i<dim;i++){
        dis+= pow(p1[i]-p2[i],2);
    }
    return dis;
}

//Distance Between 2 MBRs
int distance_mbrs(int* mbr1, int* mbr2, int* new_mbr, int dim){
    int v1 = get_volume(mbr1,dim),v2=get_volume(mbr2,dim);
    int v_new=get_volume(new_mbr,dim);

    return v_new-(v1+v2);
}

//Splits a node into 2. Returns the new node
Node Node::split(int original_node_id,int *new_node_id,int dim,int mC,FileHandler fh,Node new_node_to_add){
    Node org_node = getNode(original_node_id,dim,mC,fh);

    int min_fill = ceil(mC/2);
    //getting the farthest seeds
    int e1,e2,max_distance=-1;
    
    int* children = org_node.children;

    //Have I checked the new_node_to_add?
    bool checked_new_node;

    //Checking for the farthest 2 seeds
    for (int i=0;i<mC;i++){
        if (org_node.children[i] == -1){
            break;
        }
        checked_new_node = false;

        for (int j=i+1;j<mC+1;j++){
            if ((j==mC || org_node.children[j] == -1) && checked_new_node==true){
                break;
            }else if(j==mC || org_node.children[j]==-1){
                checked_new_node=true;
                int d;
                if (new_node_to_add.current_MBR[dim+1]==-1){
                    d = distance_points(org_node.children_MBR[i],new_node_to_add.current_MBR,dim);
                }else{
                    int new_mbr[2*dim];
                    generate_new_mbr_2d(org_node.children_MBR[i],new_node_to_add.current_MBR,new_mbr,dim);
                    d = distance_mbrs(org_node.children_MBR[i],new_node_to_add.current_MBR,new_mbr,dim);
                }
                if (d>max_distance){
                    e1 = i;
                    e2 = mC;
                    max_distance = d;
                }
            }else{
                int d;
                if (new_node_to_add.current_MBR[dim+1]==-1){
                    d = distance_points(org_node.children_MBR[i],org_node.children_MBR[j],dim);
                }else{
                    int new_mbr[2*dim];
                    generate_new_mbr_2d(org_node.children_MBR[i],org_node.children_MBR[j],new_mbr,dim);
                    d = distance_mbrs(org_node.children_MBR[i],org_node.children_MBR[j],new_mbr,dim);
                }
                if (d>max_distance){
                    e1 = i;
                    e2 = j;
                    max_distance = d;
                }
            }
             
        }
    }

    //selecting which key goes to which. to new_node means true
    bool groups[mC+1];
    groups[e1]=true;
    groups[e2]=false;
    int count1=1,count2=1;
    int* mbr1 = org_node.children_MBR[e1];
    int* mbr2;
    if (e2 == mC){
        mbr2 = new_node_to_add.current_MBR;
    }else{
        mbr2 = org_node.children_MBR[e2];
    }

    for (int i=0;i<mC+1;i++){
        if (i==e1||i==e2){
            continue;
        }else{
            if (count1==min_fill || count2==min_fill){
                int start_count1 = count1,start_count2 = count2;
                for(int j=i;j<mC+1;j++){
                    if (j==e1 || j==e2){
                        continue;
                    }else{
                        int temp_mbr[2*dim];
                        if (start_count1 == min_fill){
                            if (j==mC){
                                generate_new_mbr_2d(mbr2,new_node_to_add.current_MBR,temp_mbr,dim);
                            }else{
                                generate_new_mbr_2d(mbr2,org_node.children_MBR[j],temp_mbr,dim);
                            }
                            mbr2 = temp_mbr;
                            groups[j]=true;
                            count2++;
                        }else{
                            if (j==mC){
                                generate_new_mbr_2d(mbr1,new_node_to_add.current_MBR,temp_mbr,dim);
                            }else{
                                generate_new_mbr_2d(mbr1,org_node.children_MBR[j],temp_mbr,dim);
                            }
                            mbr1 = temp_mbr;
                            groups[j]=false;
                            count1++;
                        }
                    }
                }
                break;
            }


            //See Step 13 of insert. and complete this portion. 
            //be mindful that when j==mC we look at the new_node
            int d1, d2;
            // int* new_mbr1,*new_mbr2;
            int new_mbr1[2*dim];
            int new_mbr2[2*dim];
            int v1 = get_volume(mbr1,dim),v2 = get_volume(mbr2,dim);

            generate_new_mbr_2d(mbr1,org_node.children_MBR[i],new_mbr1,dim);
            d1 = distance_mbrs(mbr1,org_node.children_MBR[i],new_mbr1,dim);

            if (i==mC){
                generate_new_mbr_2d(mbr2,new_node_to_add.current_MBR,new_mbr2,dim);
                d2 = distance_mbrs(mbr2,new_node_to_add.current_MBR,new_mbr2,dim);
            }else{
                generate_new_mbr_2d(mbr2,org_node.children_MBR[i],new_mbr2,dim);
                d2 = distance_mbrs(mbr2,org_node.children_MBR[i],new_mbr2,dim);
            }

            if (d1<d2 || (d1==d2 && v1<v2)||(d1==d2&&v1==v2 && count1<count2)){
                groups[i]==false;
                mbr1= new_mbr1;
                count1++;
            }else{
                groups[i] =true;
                mbr2= new_mbr2;
                count2++;
            }

        }
    }

    //We now have groups with true meaning the new_node and false meaning the old_node
    int numC2=0;
    int numC1=0;
    // int new_node_MBR[2*dim];
    Node new_node = Node(*new_node_id,dim,mbr2,-1,mC);

    for (int i=0;i<mC+1;i++){
        bool t = groups[i];

        if (t){

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

            if (i!=mC){
                org_node.children[numC1]=org_node.children[i];
                org_node.children_MBR[numC1++] = org_node.children_MBR[i];
            }else{
                org_node.children[numC1]=new_node_to_add.id;
                org_node.children_MBR[numC1++] = new_node_to_add.current_MBR;
                new_node_to_add.parent_id = original_node_id;
            }
        }
    }
    storeNode(new_node.id,fh,dim,mC,new_node);
    (*new_node_id)++;

    return new_node;
}

//The main insert function
std::tuple<bool,Node> Node::insert(int* P, int root_id, int dimensionality, int maxCap, FileHandler fh,int* new_node_id){
    Node root_node = getNode(root_id,dimensionality,maxCap,fh);
    

    //Will come true if only root and is the first insert
    bool is_leaf = check_if_leaf(root_id,dimensionality,maxCap,fh);
    Node new_node = Node(*new_node_id,dimensionality,P,root_id,maxCap);
    for (int i=dimensionality;i<2*dimensionality;i++){
        new_node.current_MBR[i] = P[i-dimensionality];
    }
    if (is_leaf){
        storeNode(*new_node_id,fh,dimensionality,maxCap,new_node);
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
                (*new_node_id)++;
                return std::make_tuple(false,Node(-1,0,{},-1,0)); //to change
            }else{
                (*new_node_id)++;
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
                storeNode(root_id,fh,dimensionality,maxCap,root_node);
                return std::make_tuple(false,Node(-1,0,{},-1,0));
            }else{
                if (num_children<maxCap){
                    root_node.children_MBR[best_children] = generate_new_mbr(root_node.children_MBR[best_children],P,dimensionality);
                    root_node.current_MBR = generate_new_mbr(root_node.current_MBR,P,dimensionality);
                    root_node.children[num_children] = n.id;
                    root_node.children_MBR[num_children] = n.current_MBR;
                    storeNode(root_id,fh,dimensionality,maxCap,root_node);
                    return std::make_tuple(false,Node(-1,0,{},-1,0));
                }else{
                    //update happens in split itself.
                    Node ns = split(root_id,new_node_id,dimensionality,maxCap,fh,n);
                    storeNode(root_id,fh,dimensionality,maxCap,root_node);

                    //Check if this root_node was actually the root node of the full tree
                    if (root_node.parent_id==-1){
                        int new_root_mbr[2*dimensionality];
                        generate_new_mbr_2d(root_node.current_MBR,ns.current_MBR,new_root_mbr,dimensionality);
                        Node new_root = Node(*new_node_id,dimensionality,new_root_mbr,-1,maxCap);
                        ns.parent_id = new_root.id;
                        root_node.parent_id = new_root.id;
                        new_root.children[0] = ns.id;
                        new_root.children_MBR[0] = ns.current_MBR;
                        new_root.children[1] = root_node.id;
                        new_root.children_MBR[1] = root_node.current_MBR;
                        storeNode(*new_node_id,fh,dimensionality,maxCap,new_root);
                        storeNode(ns.id,fh,dimensionality,maxCap,ns);
                        storeNode(root_node.id,fh,dimensionality,maxCap,root_node);
                        (*new_node_id)++;
                        return std::make_tuple(false,new_root);
                    }
                    return std::make_tuple(true,ns);
                    //Need to update as well as split
                }

            }
        }
    }
    
    
}

bool does_contain_mbr(int* mbr, int* point, int dimensionality){
    for (int dimension=0; dimension<dimensionality; dimension++){
        if (point[dimension]<mbr[dimension] || point[dimension]>mbr[dimensionality+dimension]){
            return false;
        }
    }
    return true;
}

bool Node::PointQuery(int* P, int node_id, int dimensionality, int maxCap, FileHandler fh){
    Node search_node = getNode(node_id, dimensionality, maxCap, fh);

    if (is_leaf(search_node, maxCap, dimensionality)){
        // if the node is a leaf
        for (int child=0; child<maxCap; child++){
            if (search_node.children[child]==-1){
                return false;
            }
            bool found = true;
            for (int dimension=0; dimension<dimensionality; dimension++){
                if (search_node.children_MBR[child][dimension]!=P[dimension] || search_node.children_MBR[child][dimension+dimensionality]!=P[dimension]){
                    found=false;
                    break;
                }
            }
            if (found){
                return true;
            }
        }
        return false;
    }
    else{
        for (int child=0; child<maxCap; child++){
            if (search_node.children[child]==-1){
                break;
            }
            if (does_contain_mbr(search_node.children_MBR[child], P, dimensionality)){
                if (PointQuery(P, search_node.children[child], dimensionality, maxCap, fh)){
                    return true;
                }
            }
        }
        return false;
    }
}



void saveNode(Node new_node, int dimensionality, int maxCap, PageHandler ph, int node_num){
    int offset = node_num*new_node.size_of_node(dimensionality, maxCap);
    char* data = ph.GetData();
    int int_increment = sizeof(int)/sizeof(char);
    memcpy(&new_node.id, &data[offset], sizeof(int));
    memcpy(&new_node.current_MBR, &data[offset+int_increment], 2*dimensionality*sizeof(int));
    memcpy(&new_node.parent_id, &data[offset+((2*dimensionality+1)*int_increment)], sizeof(int));
    memcpy(&new_node.children, &data[offset+((2*dimensionality+2)*int_increment)], maxCap*sizeof(int));
    for (int child=0; child<maxCap; child++){
        memcpy(&new_node.children_MBR[child], &data[offset+((2*dimensionality+2+maxCap)*int_increment)], 2*dimensionality*sizeof(int));
    }
}

int* getMBR(int** children_MBR, int* children, int maxCap, int dimensionality){
    int* MBR = new int[2*dimensionality];
    for (int dimension=0; dimension<dimensionality; dimension++){
        MBR[dimension] = children_MBR[0][dimension];
        MBR[dimension+dimensionality] = children_MBR[0][dimension+dimensionality];
    }
    for (int child=1; child<maxCap; child++){
        for (int dimension=0; dimension<dimensionality; dimension++){
            MBR[dimension] = std::min(MBR[dimension], children_MBR[child][dimension]);
            MBR[dimension+dimensionality] = std::min(MBR[dimension+dimensionality], children_MBR[child][dimension+dimensionality]);
        }
    }
    return MBR;
}

Node getNode_and_setParent(int id, int dimensionality, int maxCap, FileHandler fh, int parent_id){
    int int_increment = sizeof(int)/sizeof(char);
    int page_num = int(id/numNodesPerPage(dimensionality, maxCap));
    PageHandler ph = fh.PageAt(page_num);
    int page_offset = id % numNodesPerPage(dimensionality, maxCap);
    page_offset *= (((2*dimensionality)+2+maxCap+(maxCap*2*dimensionality))*int_increment);
    char *data = ph.GetData();
    int node_id;
    memcpy(&data[page_offset], &node_id, sizeof(int));
    int* current_MBR = new int[2*dimensionality];
    memcpy(&data[page_offset+int_increment], &current_MBR, 2*dimensionality*sizeof(int));
    memcpy(&parent_id, &data[page_offset+2*dimensionality*int_increment], sizeof(int));
    int* children = new int[maxCap];
    memcpy(&data[page_offset+((2*dimensionality)+1)*int_increment], &children, maxCap*sizeof(int));
    Node myNode = Node(id, dimensionality, current_MBR, parent_id, maxCap);
    memcpy(&data[page_offset+((2*dimensionality)+1+maxCap)*int_increment], &myNode.children_MBR[0], maxCap*2*dimensionality*sizeof(int));
    fh.MarkDirty(ph.GetPageNum());
    fh.FlushPage(ph.GetPageNum());
    fh.UnpinPage(ph.GetPageNum());
    // fh.FlushPages();
    return myNode;
}


int AssignParents(FileHandler fh, int start_index, int end_index, int dimensionality, int maxCap){
    PageHandler parent_nodes = fh.NewPage();
    int num_nodes_in_page = 0;
    int num_parents_created = 0;
    int node_id = end_index+1;
    // number of nodes parents needed to be assigned
    int remaining_nodes = end_index-start_index+1;
    while(remaining_nodes!=0){
        int S = std::min(maxCap, remaining_nodes);
        if (num_nodes_in_page==numNodesPerPage(dimensionality, maxCap)){
            fh.MarkDirty(parent_nodes.GetPageNum());
            fh.FlushPage(parent_nodes.GetPageNum());
            fh.UnpinPage(parent_nodes.GetPageNum());
            parent_nodes = fh.NewPage();
            num_nodes_in_page = 0;
        }
        Node new_node = Node(node_id, dimensionality, maxCap);
        node_id++; num_parents_created++;
        for (int i=0; i<S; i++){
            Node child_node = getNode_and_setParent(end_index+1-remaining_nodes, dimensionality, maxCap, fh, node_id-1);
            new_node.children[i] = end_index+1-remaining_nodes;
            new_node.children_MBR[i] = child_node.current_MBR;
            remaining_nodes--;
        }
        new_node.current_MBR = getMBR(new_node.children_MBR, new_node.children, maxCap, dimensionality);
        saveNode(new_node, dimensionality, maxCap, parent_nodes, num_nodes_in_page);
        num_nodes_in_page++;
    }
    if (num_parents_created>1){
        AssignParents(fh, end_index+1, end_index+num_parents_created, dimensionality, maxCap);
    }else{
        return node_id-1; //root node's id
    }
    return -1;
}

std::tuple<FileHandler, int> BulkLoad(const char* filename, int N, int maxCap, int dimensionality){
    FileManager fm;

    FileHandler fh = fm.OpenFile(filename);

    int remaining_points = N;
    int node_id = 0;
    FileHandler fh_out = fm.CreateFile("rtree.txt");
    PageHandler ph;
    char* data;
    PageHandler ph_out = fh_out.NewPage();
    int num_nodes_in_page = 0;
    while(remaining_points!=0){
        int S = std::min(maxCap, remaining_points);
        Node new_node = Node(node_id, dimensionality, maxCap);
        node_id += 1;
        if (num_nodes_in_page==numNodesPerPage(dimensionality, maxCap)){
            fh_out.MarkDirty(ph_out.GetPageNum());
            fh_out.FlushPage(ph_out.GetPageNum());
            fh_out.UnpinPage(ph_out.GetPageNum());
            ph_out = fh_out.NewPage();
            num_nodes_in_page = 0;
        }
        for (int i=0; i<S; i++){
            ph = fh.PageAt(N-remaining_points);
            data = ph.GetData();
            // Unpinning the page.
            fh.UnpinPage(N-remaining_points);
            int* current_MBR = (int*)data;
            new_node.children[i] = N-remaining_points; remaining_points--;
            for (int dimension=0; dimension<dimensionality; dimension++){
                new_node.children_MBR[i][dimension] = data[dimension];
                new_node.children_MBR[i][dimension+dimensionality] = data[dimension];
            }
        }
        new_node.current_MBR = getMBR(new_node.children_MBR, new_node.children, maxCap, dimensionality);
        saveNode(new_node, dimensionality, maxCap, ph_out, num_nodes_in_page);
        num_nodes_in_page++;
    }
    fm.CloseFile(fh);
    int root_id = AssignParents(fh_out, 0, node_id-1, dimensionality, maxCap);
    std::tuple <FileHandler, int> ans;
    ans = std::make_tuple(fh_out, root_id);
    return ans;
}

