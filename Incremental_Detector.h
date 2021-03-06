//
// Created by Administrator on 3/11/2020.
//

#ifndef GRAND_CHALLENGE_INCREMENTAL_DETECTOR_H
#define GRAND_CHALLENGE_INCREMENTAL_DETECTOR_H

#include <map>
#include <set>
#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <sys/time.h>

using namespace std;
struct Init_dict
{
    //Hyperparameters Dictionary for the Event Detector
    double dbscan_eps;  //epsilon radius parameter for the dbscan algorithm
    int dbscan_min_pts;  // minimum points parameter for the dbscan algorithm
    int window_size_n;  // datapoints the algorithm takes one at a time, i..e here 1 second
    int values_per_second;  // datapoints it needs from the future
    int loss_thresh;  // threshold for model loss
    float temp_eps;  // temporal epsilon parameter of the algorithm
    // debugging, yes or no - if yes detailed information is printed to console
    bool debugging_mode;
    int network_frequency;  //base frequency
};

struct Cluster{
    vector<int> Member_Indices;
    int u;
    int v;
    int cluster_id;
    double Loc;
};

struct Checked_Cluster{
    Cluster c1;
    Cluster c2;
    vector<int> event_interval_t;
};

struct Event
{
    int event_start;
    int event_end;
};

struct Feature
{
    double active_power_P;
    double reactive_power_Q;
    int sequenceNumber;
    int badPoint;
    int clusterId;
};

extern vector<int> F1Test;

class Incremental_Detector {
private:
    double dbscan_eps;
    int dbscan_min_pts;
    int window_size_n;
    int loss_thresh;
    float temp_eps;
    int values_per_second;
    int network_frequency;
    //self.order_safety_check = None
    bool debugging_mode;
    bool dbscan_multiprocessing;
    bool is_fitted;

public:
    map<int, Feature> mapFeature;
    Cluster cluster0;
    vector<Cluster> mapCluster;
    int predictBatchNumber;
    int predictCounter;
    Incremental_Detector(Init_dict &initDict);
    Feature compute_input_signal(double voltage[1000], double current[1000], int period_length, bool single_sample_mode);
    Event predict(int batchnumber, Feature X);
    void Incremental_DBSCAN(Feature X);
    void Decremental_DBSCAN(Feature X);
    vector<int> Check_Cluster(Feature X);
    Cluster Check_Cluster0(Feature X);
    double getDis(Feature a, Feature b);
    Checked_Cluster check_event();
    void refresh_window(int MAXIMUM_WINDOW_SIZE);
    void addPoint(Feature X);

};


#endif //GRAND_CHALLENGE_INCREMENTAL_DETECTOR_H
