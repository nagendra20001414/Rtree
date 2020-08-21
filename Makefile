sampleobjects = buffer_manager.o file_manager.o sample_run.o

sample_run : $(sampleobjects)
	     g++ -std=c++11 -o sample_run $(sampleobjects)

sample_run.o : sample_run.cpp
	g++ -std=c++11 -c sample_run.cpp

buffer_manager.o : buffer_manager.cpp
	g++ -std=c++11 -c buffer_manager.cpp

file_manager.o : file_manager.cpp
	g++ -std=c++11 -c file_manager.cpp


test :
	g++ -std=c++11 -o test.out sample_run.cpp buffer_manager.cpp file_manager.cpp rtree.cpp


clean :
	rm -f *.o
	rm -f sample_run
