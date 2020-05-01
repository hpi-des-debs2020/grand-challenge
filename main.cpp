#include <iostream>
#include <string>
#include "query1.h"
#include "query2.h"
#include "query1_Parallel.h"
#include "query1_NonParallel.h"
#include "query2_Parallel.h"
#include "query2_NonParallel.h"
#include <sys/time.h>

using namespace std;



int main(int argc, char* argv[]) {

//    for(int i=0;i<argc;i++)
//        cout<<argv[i]<<endl;

    string QUERY1_ENDPOINT = "/data/1/";
    string QUERY2_ENDPOINT = "/data/2/";

    string host = "BENCHMARK_SYSTEM_URL";
    if ( host.empty() ) {
        host.assign("localhost");
        cout << "Warning: Benchmark system url undefined. Using localhost!" << endl;
    }

    struct timeval  startTime, endTime;
//    startTime = clock();//
//    gettimeofday( &startTime, NULL );
//    Query1 query1;
//    gettimeofday( &endTime, NULL );
//    cout << 1000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec)/1000 << endl;
//    endTime = clock();
//    gettimeofday( &startTime, NULL );
//    Query2 query2;
//    gettimeofday( &endTime, NULL );
//    cout << 1000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec)/1000 << endl;
    gettimeofday( &startTime, NULL );
//    Query1_Parallel query1_Parallel("http://grader/data/1/");
//    Query1_Parallel query1_Parallel("http://localhost/data/1/");
//    Query1_NonParallel query1_NonParallel("http://localhost/data/1/");
//    Query1_NonParallel query1_NonParallel("http://grader/data/1/");
    Query1 query1;
//    gettimeofday( &endTime, NULL );
//    cout << "total:   " << 1000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec)/1000 << endl;
//    gettimeofday( &startTime, NULL );
//    Query2_Parallel query2_Parallel("http://grader/data/2/");
//    Query2_Parallel query2_Parallel("http://localhost/data/2/");
//    Query2_NonParallel query2_NonParallel("http://localhost/data/2/");
//    Query2_NonParallel query2_NonParallel("http://grader/data/2/");
    Query2 query2;
    gettimeofday( &endTime, NULL );
    cout << "total:   " << 1000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec)/1000 << endl;


    cout << "Solution Done" << endl;

    return 0;
}