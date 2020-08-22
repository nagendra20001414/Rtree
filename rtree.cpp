#include "rtree.h"
#include "file_manager.h"
#include <iostream>
#include <fstream>
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
    int size = sizeof(int)/sizeof(char) * (2+(2*dimensionality)+maxCap+(maxCap*2*dimensionality));
    return int(PAGE_CONTENT_SIZE/size);
}

Node getNode(int id, int dimensionality, int maxCap, FileHandler& fh){
    int int_increment = sizeof(int)/sizeof(char);
    int page_num = int(id/numNodesPerPage(dimensionality, maxCap));
    // std::cout << page_num << " page access" << std::endl;
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

    Node myNode = Node(node_id, dimensionality, current_MBR, parent_id, maxCap);
    for (int i=0;i<maxCap;i++){
        memcpy(myNode.children_MBR[i],&data[read_pointer],sizeof(int)*2*dimensionality);
        read_pointer+=2*dimensionality*int_increment;
        myNode.children[i] = children[i];
    }
    // memcpy(&data[page_offset+((2*dimensionality)+2+maxCap)*int_increment], &myNode.children_MBR[0], maxCap*2*dimensionality*sizeof(int));
    // fh.MarkDirty(page_num);
    fh.UnpinPage(page_num);
    fh.FlushPage(page_num);

    // fh.FlushPages();
    return myNode;
}
//Function to store a node in a page
void storeNode(int id,FileHandler& fh,int dim,int maxCap,Node &n){
    int page_num = int(id/numNodesPerPage(dim,maxCap));
    int page_mod = id%numNodesPerPage(dim,maxCap);
    int node_size = sizeof(int) * (2+(2*dim)+maxCap+(maxCap*2*dim));
    int int_inc = sizeof(int)/sizeof(char); 
    int start_pos = page_mod*node_size;

    // std::cout<<"Page Num: "<<page_num<<" page_mod: "<<page_mod<<" node_size:"<<node_size<<" int_inc: "<<int_inc<<std::endl;
    PageHandler ph;
    if (page_mod ==0 && fh.LastPage().GetPageNum()<page_num){
        ph = fh.NewPage();
        // std::cout<<"new page allocated."<<std::endl;
    }else{
        ph = fh.PageAt(page_num);
        // std::cout<<"old page selected"<<std::endl;
    }
    char* data = ph.GetData();

    memcpy(&data[start_pos],(&n.id),sizeof(int));
    start_pos+=int_inc;

    memcpy(&data[start_pos],(n.current_MBR),sizeof(int)*2*dim);
    start_pos+=2*dim*int_inc;

    memcpy(&data[start_pos],(&n.parent_id),sizeof(int));

    start_pos+=int_inc;
    // n.children[0] = 6969;
    memcpy(&data[start_pos],(n.children),sizeof(int)*maxCap);
    // for (int i=0;i<2;i++){
    //     std::cout<<n.children[i]<<"\n";
    // }
    // int test;
    // memcpy(&test,&data[start_pos],sizeof(int));
    // std::cout<<"test: "<<test<<"\n";
    start_pos+=maxCap*int_inc;
    for (int i=0;i<maxCap;i++){
        memcpy(&data[start_pos],(n.children_MBR[i]),sizeof(int)*2*dim);

        start_pos+=2*dim*int_inc;
    }
    
    fh.MarkDirty(page_num);
    fh.UnpinPage(page_num);
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
bool check_if_leaf(int node_id,int dim,int mC,FileHandler& fh){
    Node n = getNode(node_id,dim,mC,fh);
    if (n.children[0]==-1){
        return true;
    }
    return false;

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
Node Node::split(int original_node_id,int &new_node_id,int dim,int mC,FileHandler& fh,Node new_node_to_add){
    Node org_node = getNode(original_node_id,dim,mC,fh);
    // std::cout<<"Going to split node: "<<org_node.id<<std::endl;
    // std::cout<<"This node has children: ";
    // for (int i=0;i<mC;i++){
    //     std::cout<<org_node.children[i]<<" ";
    // }
    // std::cout<<"\n";
    int min_fill = ceil(mC/2);
    //getting the farthest seeds
    int e1=0,e2=1,max_distance=-1;
    
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
                // if (new_node_to_add.current_MBR[dim+1]==-1){
                if (new_node_to_add.children[0]==-1){
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

    // std::cout<<"Farthest 2 seed's id: "<<e1<<":"<<org_node.children[e1]<<" and "<<e2<<":"<<org_node.children[e2]<<std::endl;
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
                groups[i]=false;
                mbr1= new_mbr1;
                count1++;
            }else{
                groups[i] =true;
                mbr2= new_mbr2;
                count2++;
            }

        }
    }

    // std::cout<<"groups: ";
    // for (int i=0;i<mC+1;i++){
    //     std::cout<<groups[i]<<" ";
    // }
    // std::cout<<"\n";
    //We now have groups with true meaning the new_node and false meaning the old_node
    int numC2=0;
    int numC1=0;
    // int new_node_MBR[2*dim];
    Node new_node = Node(new_node_id,dim,mbr2,org_node.parent_id,mC);

    for (int i=0;i<mC+1;i++){
        bool t = groups[i];

        if (t==true){

            if (i!=mC){
                new_node.children[numC2]=org_node.children[i];
                new_node.children_MBR[numC2++] = org_node.children_MBR[i];
                Node child_node = getNode(org_node.children[i],dim,mC,fh);
                child_node.parent_id = new_node_id;
                storeNode(child_node.id,fh,dim,mC,child_node);
            }else{
                new_node.children[numC2]=new_node_to_add.id;
                new_node.children_MBR[numC2++] = new_node_to_add.current_MBR;
                new_node_to_add.parent_id = new_node_id;
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
    for (int i=numC1;i<mC;i++){
        org_node.children[i]=-1;
    }
    for (int i=numC2;i<mC;i++){
        new_node.children[i]=-1;
    }
    storeNode(new_node_to_add.id,fh,dim,mC,new_node_to_add);
    storeNode(new_node.id,fh,dim,mC,new_node);
    storeNode(org_node.id,fh,dim,mC,org_node);
    (new_node_id)++;
    return new_node;
}

//The main insert function
std::tuple<bool,Node> insert(int* P, int root_id, int dimensionality, int maxCap, FileHandler& fh,int& new_node_id){
    Node root_node = getNode(root_id,dimensionality,maxCap,fh);
    // std::cout<<"Current Node: "<<root_node.id<<std::endl;
    // std::cout<<"Children ";
    // for (int i=0;i<maxCap;i++){
    //     std::cout<<root_node.children[i]<<" ";
    // }
    // std::cout<<std::endl;
    //Will come true if only root and is the first insert
    bool is_leaf = check_if_leaf(root_id,dimensionality,maxCap,fh);
    // std::cout<<"is_leaf: "<<is_leaf<<std::endl;

    Node new_node = Node(new_node_id,dimensionality,P,root_id,maxCap);
    for (int i=dimensionality;i<2*dimensionality;i++){
        new_node.current_MBR[i] = P[i-dimensionality];
    }
    if (is_leaf){
        root_node.children[0] = new_node_id;
        root_node.children_MBR[0] = P;
        root_node.current_MBR = P;
        
        storeNode(new_node_id,fh,dimensionality,maxCap,new_node);
        storeNode(root_id,fh,dimensionality,maxCap,root_node);
        new_node_id++;
        return std::make_tuple(false,Node(-1,0,{},-1,0));
        //Store P here
    }else{

        bool is_child_leaf = check_if_leaf(root_node.children[0],dimensionality,maxCap,fh);
        // std::cout<<"Is child: "<<root_node.children[0] <<" a leaf? "<<is_child_leaf<<"\n";
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
            // Node new_node = Node(new_node_id,dimensionality,P,root_id,maxCap);
            if (num_children<maxCap){
                root_node.children[num_children] = new_node_id;
                root_node.children_MBR[num_children] = P;
                root_node.current_MBR = generate_new_mbr(root_node.current_MBR,P,dimensionality);
                storeNode(root_id,fh,dimensionality,maxCap,root_node);
                storeNode(new_node_id,fh,dimensionality,maxCap,new_node);
                (new_node_id)++;
                return std::make_tuple(false,Node(-1,0,{},-1,0)); //to change
            }else{
                (new_node_id)++;
                Node ns = new_node.split(root_id,new_node_id,dimensionality,maxCap,fh,new_node);
                if (root_node.parent_id==-1){
                    int new_root_mbr[2*dimensionality];
                    generate_new_mbr_2d(root_node.current_MBR,ns.current_MBR,new_root_mbr,dimensionality);
                    Node new_root = Node(new_node_id,dimensionality,new_root_mbr,-1,maxCap);
                    ns.parent_id = new_root.id;
                    root_node.parent_id = new_root.id;
                    new_root.children[0] = ns.id;
                    new_root.children_MBR[0] = ns.current_MBR;
                    new_root.children[1] = root_node.id;
                    new_root.children_MBR[1] = root_node.current_MBR;
                    storeNode(new_node_id,fh,dimensionality,maxCap,new_root);
                    storeNode(ns.id,fh,dimensionality,maxCap,ns);
                    storeNode(root_node.id,fh,dimensionality,maxCap,root_node);
                    (new_node_id)++;
                    return std::make_tuple(false,new_root);
                }
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
                    n.parent_id = root_id;
                    storeNode(n.id,fh,dimensionality,maxCap,n);
                    storeNode(root_id,fh,dimensionality,maxCap,root_node);
                    return std::make_tuple(false,Node(-1,0,{},-1,0));
                }else{
                    //update happens in split itself.
                    Node ns = n.split(root_id,new_node_id,dimensionality,maxCap,fh,n);
                    storeNode(root_id,fh,dimensionality,maxCap,root_node);

                    //Check if this root_node was actually the root node of the full tree
                    if (root_node.parent_id==-1){
                        int new_root_mbr[2*dimensionality];
                        generate_new_mbr_2d(root_node.current_MBR,ns.current_MBR,new_root_mbr,dimensionality);
                        Node new_root = Node(new_node_id,dimensionality,new_root_mbr,-1,maxCap);
                        ns.parent_id = new_root.id;
                        root_node.parent_id = new_root.id;
                        new_root.children[0] = ns.id;
                        new_root.children_MBR[0] = ns.current_MBR;
                        new_root.children[1] = root_node.id;
                        new_root.children_MBR[1] = root_node.current_MBR;
                        storeNode(new_node_id,fh,dimensionality,maxCap,new_root);
                        storeNode(ns.id,fh,dimensionality,maxCap,ns);
                        storeNode(root_node.id,fh,dimensionality,maxCap,root_node);
                        (new_node_id)++;
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

bool PointQuery(int* P, int node_id, int dimensionality, int maxCap, FileHandler& fh){
    Node search_node = getNode(node_id, dimensionality, maxCap, fh);
    std::cout << "ok" << std::endl;

    if (search_node.is_leaf(search_node, maxCap, dimensionality)){
        // if the node is a leaf
        for (int dimension=0; dimension<dimensionality; dimension++){
            if (search_node.current_MBR[dimension]!=P[dimension] || search_node.current_MBR[dimension+dimensionality]!=P[dimension]){
                return false;
            }
        }
        return true;
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



void saveNode(Node new_node, int dimensionality, int maxCap, PageHandler& ph, int node_num){
    // int offset = node_num*new_node.size_of_node(dimensionality, maxCap);
    int offset = (new_node.id%numNodesPerPage(dimensionality, maxCap)) * sizeof(int)/sizeof(char) * (2+(2*dimensionality)+maxCap+(maxCap*2*dimensionality));
    char* data = ph.GetData();
    int int_increment = sizeof(int)/sizeof(char);
    memcpy(&data[offset], &new_node.id, sizeof(int));
    memcpy(&data[offset+int_increment], new_node.current_MBR, 2*dimensionality*sizeof(int));
    memcpy(&data[offset+((2*dimensionality+1)*int_increment)], &new_node.parent_id, sizeof(int));
    memcpy(&data[offset+((2*dimensionality+2)*int_increment)], new_node.children, maxCap*sizeof(int));
    for (int child=0; child<maxCap; child++){
        memcpy(&data[offset+((2*dimensionality+2+maxCap+(child*2*dimensionality))*int_increment)], new_node.children_MBR[child], 2*dimensionality*sizeof(int));
    }

}

int* getMBR(int** children_MBR, int* children, int maxCap, int dimensionality){
    int* MBR = new int[2*dimensionality];
    for (int dimension=0; dimension<dimensionality; dimension++){
        MBR[dimension] = children_MBR[0][dimension];
        MBR[dimension+dimensionality] = children_MBR[0][dimension+dimensionality];
    }
    for (int child=1; child<maxCap; child++){
        if (children[child]!=-1){
            for (int dimension=0; dimension<dimensionality; dimension++){
                MBR[dimension] = std::min(MBR[dimension], children_MBR[child][dimension]);
                MBR[dimension+dimensionality] = std::max(MBR[dimension+dimensionality], children_MBR[child][dimension+dimensionality]);
            }
        }
    }
    return MBR;
}

Node getNode_and_setParent(int id, int dimensionality, int maxCap, FileHandler& fh, int parent_id, PageHandler& ph_using){
    int int_increment = sizeof(int)/sizeof(char);
    int page_num = int(id/numNodesPerPage(dimensionality, maxCap));
    // std::cout << page_num << " page access" << std::endl;
    if (page_num==ph_using.GetPageNum()){
        int page_offset = id % numNodesPerPage(dimensionality, maxCap);
        page_offset *= (((2*dimensionality)+2+maxCap+(maxCap*2*dimensionality))*int_increment);
        int read_pointer = page_offset;
        char *data = ph_using.GetData();
        int node_id;
        // memcpy(&data[page_offset], &node_id, sizeof(int));
        memcpy(&node_id, &data[read_pointer],  sizeof(int));
        read_pointer+=int_increment;
        int* current_MBR = new int[2*dimensionality];
        // memcpy(&data[page_offset+int_increment], &current_MBR, 2*dimensionality*sizeof(int));
        memcpy(current_MBR, &data[read_pointer],  2*dimensionality*sizeof(int));
        read_pointer+=((2*dimensionality)*int_increment);
        // int parent_id;
        // memcpy(&data[page_offset+2*dimensionality*int_increment], &parent_id, sizeof(int));
        memcpy(&data[read_pointer], &parent_id,  sizeof(int));
        read_pointer+=int_increment;
        int* children = new int[maxCap];
        // memcpy(&data[page_offset+((2*dimensionality)+1)*int_increment], &children, maxCap*sizeof(int));
        memcpy(children, &data[read_pointer],  maxCap*sizeof(int));
        read_pointer+=(maxCap*int_increment);

        Node myNode = Node(node_id, dimensionality, current_MBR, parent_id, maxCap);
        for (int i=0;i<maxCap;i++){
            memcpy(myNode.children_MBR[i],&data[read_pointer],sizeof(int)*2*dimensionality);
            read_pointer+=(2*dimensionality*int_increment);
            myNode.children[i] = children[i];
        }

    }else{

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
        read_pointer+=((2*dimensionality)*int_increment);
        // int parent_id;
        // memcpy(&data[page_offset+2*dimensionality*int_increment], &parent_id, sizeof(int));
        memcpy(&data[read_pointer], &parent_id,  sizeof(int));
        read_pointer+=int_increment;
        int* children = new int[maxCap];
        // memcpy(&data[page_offset+((2*dimensionality)+1)*int_increment], &children, maxCap*sizeof(int));
        memcpy(children, &data[read_pointer],  maxCap*sizeof(int));
        read_pointer+=(maxCap*int_increment);

        Node myNode = Node(node_id, dimensionality, current_MBR, parent_id, maxCap);
        for (int i=0;i<maxCap;i++){
            memcpy(myNode.children_MBR[i],&data[read_pointer],sizeof(int)*2*dimensionality);
            read_pointer+=(2*dimensionality*int_increment);
            myNode.children[i] = children[i];
        }
        // memcpy(&data[page_offset+((2*dimensionality)+2+maxCap)*int_increment], &myNode.children_MBR[0], maxCap*2*dimensionality*sizeof(int));
        // fh.MarkDirty(page_num);
        fh.MarkDirty(page_num);
        fh.UnpinPage(page_num);
        fh.FlushPage(page_num);

        // fh.FlushPages();
        return myNode;
    }
    Node myNode = Node(id, dimensionality, maxCap);
    return myNode;

}


int AssignParents(FileHandler& fh, int start_index, int end_index, int dimensionality, int maxCap){
    std::cout << "parents from " << start_index << " to " << end_index << std::endl; 
    int num_nodes_per_page = numNodesPerPage(dimensionality, maxCap);
    PageHandler parent_nodes;
    int num_nodes_in_page = 0;
    if ((end_index+1)%num_nodes_per_page==0){
        parent_nodes = fh.NewPage();
    }else{
        int page_id = int((end_index+1)/num_nodes_per_page);
        parent_nodes = fh.PageAt(page_id);
        num_nodes_in_page = ((end_index+1)%num_nodes_per_page);
    }
    std::cout << "curr " << num_nodes_in_page << std::endl;
    int num_parents_created = 0;
    int node_id = end_index+1;
    // number of nodes parents needed to be assigned
    int remaining_nodes = end_index-start_index+1;
    while(remaining_nodes!=0){
        int S = std::min(maxCap, remaining_nodes);
        if (num_nodes_in_page==num_nodes_per_page){
            fh.MarkDirty(parent_nodes.GetPageNum());
            fh.UnpinPage(parent_nodes.GetPageNum());
            fh.FlushPage(parent_nodes.GetPageNum());
            std::cout << parent_nodes.GetPageNum() << " written2" << std::endl;
            parent_nodes = fh.NewPage();
            num_nodes_in_page = 0;
        }
        Node new_node = Node(node_id, dimensionality, maxCap);
        node_id++; num_parents_created++;
        if (node_id==165){
            int n;
            memcpy(&n, &parent_nodes.GetData()[2368], sizeof(int));
            std::cout << n << " ok1" << std::endl;
        }
        for (int i=0; i<S; i++){
            Node child_node = getNode_and_setParent(end_index+1-remaining_nodes, dimensionality, maxCap, fh, node_id-1, parent_nodes);
            // Node child_node = getNode(end_index+1-remaining_nodes, dimensionality, maxCap, fh);
            new_node.children[i] = end_index+1-remaining_nodes;
            new_node.children_MBR[i] = child_node.current_MBR;
            remaining_nodes--;
        }
        new_node.current_MBR = getMBR(new_node.children_MBR, new_node.children, maxCap, dimensionality);
        saveNode(new_node, dimensionality, maxCap, parent_nodes, num_nodes_in_page);
        // std::cout << "d " << new_node.id << std::endl;
        num_nodes_in_page++;
    }
    if (num_nodes_in_page>0){
        fh.MarkDirty(parent_nodes.GetPageNum());
        fh.UnpinPage(parent_nodes.GetPageNum());
        fh.FlushPage(parent_nodes.GetPageNum());
        std::cout << parent_nodes.GetPageNum() << " written1" <<std::endl;
    }
    if (end_index+1 <= 163 && node_id-1>=163){
        Node n = getNode(163, dimensionality, maxCap, fh);
        std::cout << n.id << " u" << std::endl;
        n = getNode(174, dimensionality, maxCap, fh);
        std::cout << n.id << " u" << std::endl;
    }
    if (num_parents_created>1){
        return AssignParents(fh, end_index+1, node_id-1, dimensionality, maxCap);
    }else{
        return node_id-1; //root node's id
    }
    return -1;
}

std::tuple<FileHandler, int, int> BulkLoad(FileHandler& fh, FileHandler& fh_out, int N, int maxCap, int dimensionality){

    int remaining_points = N;
    int node_id = 0;
    PageHandler ph;
    char* data;
    PageHandler ph_out = fh_out.NewPage();
    int num_nodes_in_page = 0;
    int num_points_per_page = (int)PAGE_CONTENT_SIZE/(sizeof(int)*dimensionality);
    std::cout << "nodes per page: " << numNodesPerPage(dimensionality, maxCap) << std::endl;
    std::cout << ph_out.GetPageNum() << std::endl;
    while (remaining_points!=0){
        if (num_nodes_in_page==numNodesPerPage(dimensionality, maxCap)){
            fh_out.MarkDirty(ph_out.GetPageNum());
            fh_out.UnpinPage(ph_out.GetPageNum());
            fh_out.FlushPage(ph_out.GetPageNum());
            ph_out = fh_out.NewPage();
            std::cout << ph_out.GetPageNum() << std::endl;
            num_nodes_in_page = 0;
        }
        int page_id = (int)((N-remaining_points)/num_points_per_page);
        ph = fh.PageAt(page_id);
        data = ph.GetData();
        // Unpinning the page.
        fh.UnpinPage(page_id);
        fh.FlushPage(page_id);
        int offset = ((N-remaining_points)%num_points_per_page)*dimensionality;
        Node new_node = Node(node_id, dimensionality, maxCap);
        node_id +=1;
        int* current_MBR = (int*)data;
        for (int dimension=0; dimension < dimensionality; dimension++){
            new_node.current_MBR[dimension] = current_MBR[dimension];
            new_node.current_MBR[dimension+dimensionality] = current_MBR[dimension];
        }
        saveNode(new_node, dimensionality, maxCap, ph_out, num_nodes_in_page);
        num_nodes_in_page++;
        remaining_points--;
    }
    if (num_nodes_in_page>0){
        fh_out.MarkDirty(ph_out.GetPageNum());
        fh_out.UnpinPage(ph_out.GetPageNum());
        fh_out.FlushPage(ph_out.GetPageNum());
        std::cout << ph_out.GetPageNum() << " wriiten" << std::endl;
   }
    int root_id = AssignParents(fh_out, 0, N-1, dimensionality, maxCap);
    // fh_out.FlushPages();
    std::tuple <FileHandler, int, int> ans;
    ans = std::make_tuple(fh_out, root_id, root_id+1);
    return ans;
}

void checkTree(FileHandler& fh, int numNodes, int dimensionality, int maxCap){
    int num_nodes_per_page = numNodesPerPage(dimensionality, maxCap);
    PageHandler ph;
    std::cout << "num: " << numNodes << std::endl;
    for (int i=0; i<numNodes; i++){
        int page_id = int(i/num_nodes_per_page);
        int offset = (i%num_nodes_per_page) * sizeof(int)/sizeof(char) * (2+(2*dimensionality)+maxCap+(maxCap*2*dimensionality));
        int int_increment=sizeof(int)/sizeof(char);
        ph = fh.PageAt(page_id);
        char* data = ph.GetData();
        int id;
        memcpy(&id, &data[offset], sizeof(int));
        std::cout << id << " check" << std::endl;
        fh.UnpinPage(page_id);
        fh.FlushPage(page_id);
    }
}


int main(int argv, char **argc){
    const char *query_file = argc[1];
    int maxCap = atoi(argc[2]);
    int dimensionality = atoi(argc[3]);
    const char *output_file = argc[4];
    FileManager fm;
    // std::cout << query_file << " " << maxCap << " " << dimensionality << " " << output_file << std::endl;

    // create rtree file
    FileHandler fh_rtree = fm.CreateFile("rtree.txt");

    // open query file
    std::ifstream query_file_handle(query_file);
    std::string line;
    std::ofstream output_file_handle(output_file);
    int root_id = -1;
    int numNodes = 0;
    if (query_file_handle.is_open()){
        while (std::getline(query_file_handle, line)){
            // std::cout << line << std::endl;
            int query_type = -1; // 0 - bulk load, 1 - insert, 2 - point query
            if (line.substr(0, 5)=="QUERY"){
                query_type = 2;
            }else if (line.substr(0, 6)=="INSERT"){
                query_type = 1;
            }else{
                query_type = 0;
            }

            // Bulkload
            if (query_type==0){
                line = line.substr(9, line.size()-9);
                // std::cout << line << std::endl;
                int delim_pos = line.find(" ");
                // std::cout<<line<<std::endl;
                // const char* bulk_load_file = ("./"+line.substr(0, delim_pos)).c_str();
                std::string temp = line.substr(0,delim_pos);
                char* bulk_load_file = const_cast<char*>(temp.c_str());
                // std::cout <<"Boom:"<< bulk_load_file << std::endl;
                int N = std::stoi(line.substr(delim_pos+1, line.size()-delim_pos-1));
                // std::cout << N << std::endl;
                // open bulk load file
                FileHandler bulk_load_file_handler = fm.OpenFile(bulk_load_file);
                // std::cout << "ok" << std::endl;
                std::tuple<FileHandler, int, int> ans = BulkLoad(bulk_load_file_handler, fh_rtree, N, maxCap, dimensionality);
                // std::cout << "ok" << std::endl;
                root_id = std::get<1>(ans);
                numNodes = std::get<2>(ans);
                fm.CloseFile(bulk_load_file_handler);
                output_file_handle << "BULKLOAD" << std::endl;
                output_file_handle << std::endl;
                output_file_handle << std::endl;
                checkTree(fh_rtree, numNodes, dimensionality, maxCap);

            }else if (query_type==1){
                //insert
                if (root_id==-1){
                    root_id = 0;
                }
                line = line.substr(7, line.size()-7);
                int *P = new int[dimensionality];
                for (int i=0; i<dimensionality; i++){
                    int space_pos = line.find(" ");
                    P[i] = std::stoi(line.substr(0, space_pos));
                    line = line.substr(space_pos+1, line.size()-space_pos-1);
                }
                std::tuple<bool, Node> ans = insert(P, root_id, dimensionality, maxCap, fh_rtree, numNodes);
                if (std::get<0>(ans)){
                    root_id = (std::get<1>(ans)).id;
                }
                // numNodes++;
                output_file_handle << "INSERT" << std::endl;
                output_file_handle << std::endl;
                output_file_handle << std::endl;
            }else{
                line = line.substr(6, line.size()-6);
                int *P = new int[dimensionality];
                for (int i=0; i<dimensionality; i++){
                    int space_pos = line.find(" ");
                    P[i] = std::stoi(line.substr(0, space_pos));
                    line = line.substr(space_pos+1, line.size()-space_pos-1);
                }
                std::cout << root_id << " root" << std::endl;
                bool ans = PointQuery(P, root_id, dimensionality, maxCap, fh_rtree);
                if (ans){
                    output_file_handle << "TRUE" << std::endl;
                }else{
                    output_file_handle << "FALSE" << std::endl;
                }
                output_file_handle << std::endl;
                output_file_handle << std::endl;
            }
        }
    }
    return 0;
}