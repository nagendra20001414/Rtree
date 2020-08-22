//Sample file for students to get their code running

#include<iostream>
#include "file_manager.h"
#include "errors.h"
#include<cstring>
#include "rtree.h"

using namespace std;


int main() {
	FileManager fm;

	// Create a brand new file
	FileHandler fh = fm.CreateFile("temp.txt");
	cout << "File created " << endl;

	// Create a new page
	PageHandler ph = fh.NewPage ();
	cout << ph.GetPageNum() << endl;
	char *data = ph.GetData ();
	// Node root_node = Node(0,2,2);
	// cout<< root_node.id<<" "<<root_node.parent_id<<endl;
	// storeNode(0,fh,2,2,root_node);
	// cout<<"Node stored."<<endl;
	// Store an integer at the very first location
	int num = 5;
	memcpy (&data[0], &num, sizeof(int));

	// Store an integer at the second location
	num = 1000;
	memcpy (&data[4], &num, sizeof(int));
	fh.MarkDirty(ph.GetPageNum());
	fh.UnpinPage(ph.GetPageNum());
	fh.FlushPage(ph.GetPageNum());

	ph = fh.NewPage();
	cout << ph.GetPageNum() << endl;
	data = ph.GetData();
	num = 501;
	memcpy (&data[0], &num, sizeof(int));

	// Flush the page
	fh.FlushPages ();
	cout << "Data written and flushed" << endl;

	// Close the file
	fm.CloseFile(fh);

	// Reopen the same file, but for reading this time
	fh = fm.OpenFile ("temp.txt");
	cout << "File opened" << endl;
	
	// Get the very first page and its data
	ph = fh.FirstPage ();
	data = ph.GetData ();
	// memcpy(&num,&data[0],sizeof(int));
	// cout<<"id: "<<num<<endl;

	// Node n = getNode(0,2,2,fh);
	// cout<<"Node loaded."<<endl;
	// cout<<"Node: "<<n.id<<" "<<n.parent_id<<endl;;
	// Output the first integer
	memcpy (&num, &data[0], sizeof(int));
	cout << "First number: " << num << endl;

	// Output the second integer
	memcpy (&num, &data[4], sizeof(int));
	cout << "Second number: " << num << endl;;

	fh.UnpinPage(ph.GetPageNum());
	ph = fh.PageAt(1);
	data = ph.GetData();
	memcpy(&num, &data[0], sizeof(int));
	cout << "Third number: " << num << endl;
	// Close the file and destory it
	fm.CloseFile (fh);
	fm.DestroyFile ("temp.txt");
}
