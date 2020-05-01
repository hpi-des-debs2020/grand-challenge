//
// Created by Administrator on 2/2/2020.
//

#ifndef CHALLENGE_EVENT_DETECTOR_H
#define CHALLENGE_EVENT_DETECTOR_H

#include <map>
#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include "DBSCAN.h"
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

extern vector<int> F1TestOriginal;

class Event_Detector {

    /*
    Args:
            dbscan_eps (float): Epsilon Parameter for the DBSCAN algorithm
            dbscan_min_pts (int): Minimum Points Parameter for the DBSCAN algorithm
            window_size_n (int): Window Size
            loss_thresh (int): treshhold fopr the loss-function
            temp_eps (float):  temporal locality epsilon
            perform_input_order_checks: check the correct input order before processing the input, as described in
            the doc-string
            debugging_mode (bool): activate if plots of the dbscan clustering shall be shown
            grid_search_mode (bool): activate to adapt the score function, if you want to perfrom grid-search
            values_per_second (int): values per second that are fed to the algorithm
            dbscan_multiprocessing (bool): default=False, if set to true multiple processes are used in the dbscan algorithm.
            If the Barsim_Sequential event detector is used within a multiprocessing environment, turning the dbscan_multiprocessing
            paramter to True, results in warnings by sklearn and the multiprocessing library, as no additional subprocesses can
            be spawned by the processes.
     */

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
    int** cluster_counter;
    int cluster_number;
    int cluster_number_more_than_2;
    int cluster0_time;
    int cluster0_size;
    int cluster1_time;
    int cluster1_size;
    int clusterX_time;
    int clusterX_size;
    clock_t startTime, dbscanTime, eventTime, thresholdTime, verifyTime;
    Event_Detector(Init_dict &initDict);
    void fit();
    Feature compute_input_signal(double voltage[1000], double current[1000], int period_length, bool single_sample_mode);
    Event predict(int batchnumber, vector<Feature> &X);
    vector<Cluster> _update_clustering(vector<Feature> &X);
    vector<Checked_Cluster>  _check_event_model_constraints(vector<Cluster> clustering_structure);
    Checked_Cluster _compute_and_evaluate_loss(vector<Checked_Cluster> checked_clusters);
    Checked_Cluster _roleback_backward_pass(bool status, vector<Cluster> backward_clustering_structure,
                                            Checked_Cluster event_cluster_combination_balanced, int i);
    //STREAMING_EventDet_Barsim_Sequential();

};


#endif //CHALLENGE_EVENT_DETECTOR_H
