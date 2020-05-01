//
// Created by Administrator on 2/2/2020.
//

#include "Event_Detector.h"

using namespace std;
vector<int> F1TestOriginal;
Event_Detector::Event_Detector( Init_dict &initDict ){

    //We compute 50 features (data points) per second, once every 1000 samples
    int VALUES_PER_SECOND = 50;

    //Hyperparameters Dictionary for the Event Detector
    Init_dict init_dict;
    init_dict.dbscan_eps = 0.03;  //epsilon radius parameter for the dbscan algorithm
    init_dict.dbscan_min_pts = 2;  // minimum points parameter for the dbscan algorithm
    init_dict.window_size_n = 50;  // datapoints the algorithm takes one at a time, i..e here 1 second
    init_dict.values_per_second = VALUES_PER_SECOND;  // datapoints it needs from the future
    init_dict.loss_thresh = 40;  // threshold for model loss
    init_dict.temp_eps = 0.8;  // temporal epsilon parameter of the algorithm
    // debugging, yes or no - if yes detailed information is printed to console
    init_dict.debugging_mode = false;
    init_dict.network_frequency = 50;  //base frequency


    this->dbscan_eps = init_dict.dbscan_eps;
    this->dbscan_min_pts = init_dict.dbscan_min_pts;
    this->window_size_n = init_dict.window_size_n;
    this->loss_thresh = init_dict.loss_thresh;
    this->temp_eps = init_dict.temp_eps;

    this->values_per_second = init_dict.values_per_second;
    this->network_frequency = init_dict.network_frequency;

//    this->order_safety_check = None

    this->debugging_mode = init_dict.debugging_mode;
    this->dbscan_multiprocessing = true;

    cluster_number = 0;
    cluster_number_more_than_2 = 0;
    cluster0_time = 0;
    cluster0_size = 0;
    cluster1_time = 0;
    cluster1_size = 0;
    clusterX_time = 0;
    clusterX_size = 0;

    cluster_counter = new int*[20];
    for( int i=0; i<20; i++)
    {
        cluster_counter[i] = new int[101];
        for(int j = 0;j < 101;j++){
            cluster_counter[i][j] = 0;
        }
    }

}

void Event_Detector::fit(){
    this->is_fitted = true;
}

Feature Event_Detector::compute_input_signal(double voltage[1000], double current[1000], int period_length, bool single_sample_mode){
/*
Args:
    voltage (ndarray): one-dimensional voltage array
    current (ndarray: one-dimensional current array
    period_length (int): length of a period for the given dataset.
            example: sampling_rate=10kHz, 50Hz basefrequency,
            10kHz / 50 = 200 samples / period
    original_non_log (bool): default: False, if set to true, the non-logarithmized data is returned.
            single_sample_mode (bool): default: False, if set to true, the input checks are adapted for single sampels

    Returns:
    X (ndarray): feature vector with active and reactive power, with shape(window_size_n,2).
            The first component at time t is the active, the second one the reactive power.
            The active and reactive power are per values per second.
*/

    //period = 1000 bool == true
    //int compute_features_window = period * this.network_frequency/this.values_per_second //compute features averaged over this timeframe
    //bool original_non_log = false;

    Feature X;
    double active_power_P = 0;
    double apparent_power_S = 0;
    double reactive_power_Q = 0;
    double instant_voltage = 0;
    double instant_current = 0;
    double instant_power = 0;
    //Compute the active and reactive power using the Metrics class provided with this estimator
    for(int i = 0; i < period_length; i++){
        instant_voltage += voltage[i]*voltage[i];
        instant_current += current[i]*current[i];
        instant_power += voltage[i] * current[i];
    }
    active_power_P = instant_power/period_length;
    apparent_power_S = sqrt(instant_voltage/period_length)*sqrt(instant_current/period_length);
    reactive_power_Q = sqrt(apparent_power_S * apparent_power_S - active_power_P * active_power_P);

    X.active_power_P = log(active_power_P);
    X.reactive_power_Q =  log(reactive_power_Q);

    return X;
}

Event Event_Detector::predict(int batchnumber, vector<Feature> &X){

/*  Predict if the input provided contains an event or not.
    The input provided should be computed by the compute_input_signal function of this class.
    Args:
    X_overall (ndarray): Input computed by compute_input_signal function.
    Returns:
    event_interval_indices (tuple): (start_index, end_index), if no event detected None is returned
*/

    Event event;
    event.event_start = 0;
    event.event_end = 0;
    // 2. Event Detection Logic
    //2.1 Forward Pass
    bool event_detected = false; // Flag to indicate if an event was detected or not

    //# 2.1.2 Update the clustering and the clustering structure, using the DBSCAN Algorithm
    //# By doing this we get clusters C1 and C2
    startTime = clock();
    vector<Cluster> clustering_structure = _update_clustering(X);


    for(int i = 0; i < clustering_structure.size(); i++ ){
        if (clustering_structure[i].Member_Indices.size() > 0) {
            cluster_counter[clustering_structure[i].cluster_id][clustering_structure[i].Member_Indices.size()]++;
        }
    }


//    cout << "batch:  " << batchnumber<< "  window size:  " << X.size() <<  "  cluster number: " << clustering_structure.size();
//    cluster_number+=clustering_structure.size();
//    if(clustering_structure.size() > 2){
//        cluster_number_more_than_2++;
//        cout << "batch:  " << batchnumber<< "  window size:  " << X.size() <<  "  cluster number: " << clustering_structure.size();
//    for(int i = 0; i < clustering_structure.size(); i++ ){
//        if (clustering_structure[i].Member_Indices.size() > 0) {
//
//            cout << "   cluster id:" << clustering_structure[i].cluster_id << " u:" << clustering_structure[i].u
//                 << " v:" <<
//                 clustering_structure[i].v << " cluster size:" << clustering_structure[i].Member_Indices.size();
//        }
////                << "     " << clustering_structure[i].Loc;
////        for(int j = 0; j < clustering_structure[i].Member_Indices.size(); j++){
////        cout << "   " << clustering_structure[i].Member_Indices[j] ;
////        }
//        if(clustering_structure[i].cluster_id == 0){
//            cluster0_time++;
//            cluster0_size+=clustering_structure[i].Member_Indices.size();
//        } else if(clustering_structure[i].cluster_id == 1){
//            cluster1_time++;
//            cluster1_size+=clustering_structure[i].Member_Indices.size();
//        }else{
//            clusterX_time++;
//            clusterX_size+=clustering_structure[i].Member_Indices.size();
//        }
//    }
//        cout << endl;
//    }


    dbscanTime+= clock() - startTime;
//        for(int i = 0; i < clustering_structure.size(); i++ ){
//            cout<< clustering_structure[i].cluster_id - 1 << "  +  " << clustering_structure[i].u << "  +  " << clustering_structure[i].v
//            << "  +  " << clustering_structure[i].Loc;
//            for(int j = 0; j < clustering_structure[i].Member_Indices.size(); j++){
//            cout << "    " << clustering_structure[i].Member_Indices[j] ;
//            }
//            cout << endl;
//        }
//    cout << endl;

    //# Now check the mode constraints
    //# Possible intervals event_interval_t are computed in the _check_event_model_constraints() function.
    startTime = clock();
    vector<Checked_Cluster> checked_clusters = _check_event_model_constraints(clustering_structure);
    eventTime+= clock() - startTime;
    // If there are no clusters that pass the model constraint tests, the method _check_event_model_constraints()
    // returns None, else a list of triples (c1, c2, event_interval_t).
    Checked_Cluster event_cluster_combination;
    vector<Cluster> forward_clustering_structure;

    startTime = clock();
    if (checked_clusters.empty()) {
        return event;
    }
        // 2.1.3 Compute the Loss-values
    else { // at least one possible combination of two clusters fullfills the event model constraints-
        // Hence, we can proceed with step 3 of the forward pass.
        // Therefore, we compute the loss for the given cluster combination.
        // The formula for the loss, is explained in the doc-string in step 3 of the forward pass.
        event_cluster_combination = _compute_and_evaluate_loss(checked_clusters);
//        cout << X.size()<< endl;
//        for(int i =0; i < X.size(); i++){
//            cout<< X[i].active_power_P << "    " << X[i].reactive_power_Q << "  " << endl;
//        }
//        cout << "batch:  " << batchnumber<< "  window size:  " << X.size() <<  "  cluster number: " << clustering_structure.size()<<endl;
//        for(int i = 0; i < clustering_structure.size(); i++ ){
//            cout<< clustering_structure[i].cluster_id << "  u: " << clustering_structure[i].u << "   v: " << clustering_structure[i].v<< "   member: ";
////            << "     " << clustering_structure[i].Loc;
//            for(int j = 0; j < clustering_structure[i].Member_Indices.size(); j++){
//            cout << " " << clustering_structure[i].Member_Indices[j] ;
//            }
//            cout << endl;
//        }
//        for(int i = 0; i < checked_clusters.size(); i++ ){
//            cout<< checked_clusters[i].c1.cluster_id << "    " << checked_clusters[i].c2.cluster_id ;
//            for(int j = 0; j < checked_clusters[i].event_interval_t.size(); j++){
//                cout << "   " << checked_clusters[i].event_interval_t[j] ;
//            }
//            cout << endl;
//        }
//        cout<< "event_cluster_combination   " << event_cluster_combination.event_interval_t[0] << "   " <<
//        event_cluster_combination.event_interval_t[event_cluster_combination.event_interval_t.size()-1] << endl;
        forward_clustering_structure = clustering_structure; //save the forward clustering structure
        if( !event_cluster_combination.event_interval_t.empty()){
            event_detected = true;
        }else{
            return event;
        }
    }
    thresholdTime+= clock() - startTime;

    startTime = clock();
    if (event_detected == true) {
        //an event was detected in the forward pass, so the backward pass is started
        //Initialize the backward pass clustering with the forward pass clustering, in case already the
        //first sample that is removed, causes the algorithm to fail. Then the result from the forward
        //pass is the most balanced event
        vector<Cluster> backward_clustering_structure = forward_clustering_structure;
        Checked_Cluster event_cluster_combination_balanced = event_cluster_combination;
        vector<Feature> X_cut = X;

        //2.2.1. Delete the oldest sample x1 from the segment (i.e the first sample in X)
        for ( int i = 1; i < X.size(); i++){
            X_cut.erase(X_cut.begin()); //delete the first i elements, i.e. in each round the oldest sample is removed
            //# 2.2.2 Update the clustering structure
            clustering_structure = _update_clustering(X_cut); //the clustering_structure is overwritten, but the winning one
            // 2.2.3 Compute the loss-for all clusters that are detected (except the detected)
            // Hence, we need to check the event model constraints again
            checked_clusters = _check_event_model_constraints(clustering_structure);

            if (checked_clusters.empty()) { //roleback with break
                event_cluster_combination_balanced = _roleback_backward_pass(false, backward_clustering_structure, event_cluster_combination_balanced, i);
                break; //finished
            }else {
                //compute the loss
                // 2.2.4 Check the loss-values for the detected segment
                Checked_Cluster event_cluster_combination_below_loss =  _compute_and_evaluate_loss(checked_clusters);
                if (event_cluster_combination_below_loss.event_interval_t.empty()) {
                    //roleback with break
                    event_cluster_combination_balanced = _roleback_backward_pass(false, backward_clustering_structure, event_cluster_combination_balanced, i);
                    break; //finished
                }else{
                    // continue with the backward pass
                    // update the backward_clustering_structure with the latest valid one
                    // i.e. the new clustering structure
                    backward_clustering_structure = clustering_structure;
                    event_cluster_combination_balanced = event_cluster_combination_below_loss;  // same here
                    continue; //not finished, next round, fiight
                }
            }
        }
        event.event_start = event_cluster_combination_balanced.event_interval_t[0];
        event.event_end = event_cluster_combination_balanced.event_interval_t[event_cluster_combination_balanced.event_interval_t.size()-1];
    }
    verifyTime+= clock() - startTime;
    return event;
}

vector<Cluster> Event_Detector::_update_clustering(vector<Feature> &X){
/*
    Using the DBSCAN Algorithm to update the clustering structure.
    clustering_structure (dict): resulting nested clustering structure. contains the following keys
    For each cluster it contains: {"Cluster_Number" : {"Member_Indices": []"u" : int,"v" : int,"Loc" : float} }
    u and v are the smallest and biggest index of each cluster_i respectively.
            Loc is the temporal locality metric of each cluster_i.
    Args:
    X (ndarray): input window, shape=(n_samples, 2)
    Returns:
    None
*/
    // Do the clustering
    // dbscan = DBSCAN(eps=self.dbscan_eps, min_samples=self.dbscan_min_pts, n_jobs=-1).fit(X)
    DBCAN dbScan(this->dbscan_eps, this->dbscan_min_pts, X);
    dbScan.run();
    vector<vector<int>> clusterR = dbScan.getCluster(); //cluster and the size of cluster

    vector<Cluster> clustering_structure;
    //build the cluster structure, for each cluster store the indices of the points.
    for (int i = 0; i < clusterR.size(); i++ ){
        // Which datapoints (indices) belong to cluster_i
        // Determine u and v of the cluster (the timely first and last element, i.e. the min and max index)
        // compute the temporal locality of cluster_ci
        Cluster clusterT;
        clusterT.cluster_id = i;
        if(clusterR[i].size() > 0) {
            clusterT.Member_Indices = clusterR[i];
            clusterT.u = clusterR[i][0];
            clusterT.v = clusterR[i][clusterR[i].size() - 1];
            clusterT.Loc = clusterT.Member_Indices.size() / (double) (clusterT.v - clusterT.u + 1);
        }
//        cout<<clusterT.Member_Indices.size() << "         " << (clusterT.v - clusterT.u + 1) << "   " << clusterT.Loc << endl;
        // insert the structure of cluster_i into the overall clustering_structure
        clustering_structure.push_back(clusterT);

    }
//    cout<< clusterR.size()<< endl;
//    for(int i = 0; i < clusterR.size(); i++ ){
//        cout<< clusterR.size() << "    " << clusterR[i].size() << endl;
//        for(int j = 0; j < clusterR[i].size(); j++){
//            cout << clusterR[i][j] << "   ";
//        }
//    }
//    cout<<"hola------------" << endl;

    return clustering_structure;
}

vector<Checked_Cluster>  Event_Detector::_check_event_model_constraints(vector<Cluster> clustering_structure){
/*
    Checks the constraints the event model, i.e. event model 3, opposes on the input data.
            It uses the clustering_structure attribute, that is set in the _update_clustering() function.
    Returns:
    checked_clusters (list): list of triples (c1, c2, event_interval_t)
    with c1 being the identifier of the first cluster, c2 the second cluster
    in the c1 - c2 cluster-combination, that have passed the model
    checks. The event_interval_t are the indices of the datapoints in between the two
    clusters.
*/
    vector<int> event_interval_t;
    vector<Checked_Cluster> checked_clusters;
    //(1) it contains at least two clusters C1 and C2, besides the outlier cluster, and the outlier Cluster C0
    // can be non empty. (The noisy samples are given the the cluster -1 in this implementation of DBSCAN)
    if ( clustering_structure.size() - 1 < 2 ) //check (1) not passed
        return checked_clusters;

    // (2) clusters C1 and C2 have a high temporal locality, i.e. Loc(Ci) >= 1 - temp_eps
    // i.e. there are at least two, non noise, clusters with a high temporal locality
    // the first cluster is the noise
    vector<Cluster> checked_two_clusters;
    for(int i = 1; i < clustering_structure.size(); i++){
        if (clustering_structure[i].Loc >= 1 - this->temp_eps) {  //the central condition of condition (2)
            checked_two_clusters.push_back(clustering_structure[i]);
        }
    }
    if (checked_two_clusters.size() < 2)  //check (2) not passed
        return checked_clusters;

    // (3) two clusters C1 and C2 do not interleave in the time domain.
    // There is a point s in C1 for which all points n > s do not belong to C1 anymore.
    // There is also a point i in C2 for which all points n < i do not belong to C2 anymore.
    // i.e. the maximum index s of C1 has to be smaller then the minimum index of C2
    for(int i = 0; i < checked_two_clusters.size(); i ++){
        Cluster c1,c2;
        for(int j = i + 1; j < checked_two_clusters.size(); j++){
            // the cluster with the smaller u, occurs first in time, we name it C1 according to the paper terminology here
            if (checked_two_clusters[i].u < checked_two_clusters[j].u){
                c1 = checked_two_clusters[i];
                c2 = checked_two_clusters[j];
            }else{
                c1 = checked_two_clusters[j];
                c2 = checked_two_clusters[i];
            }
            // now we check if they are overlapping
            // the maximum index of C1 has to be smaller then the minimum index of C2, then no point of C2 is in C1
            // and the other way round, i.e all points in C1 have to be smaller then u of C2
            if (c1.v < c2.u){ // #no overlap detected
                // if the clusters pass this check, we can compute the possible event_interval_t (i.e. X_t)
                // for them. This interval possibly contains an event and is made up of all points between cluster c1
                // and cluster c2 that are noisy-datapoints, i.e. that are within the 0 cluster(it is the first cluster of clustering_structure).
                // THe noisy points have to lie between the upper-bound of c1 and the lower-bound of c2
//                    if ( clustering_structure[0].Member_Indices.size() < 0) {
//                        return None
//                    }
                //ASSUMPTION if there is no noise cluster, then the check is not passed.
                // No event segment is between the two steady state clusters then.
                // Any other proceeding would cause problems in all subsequent steps too.

                // check the condition, no overlap between the noise and steady state clusters allowed: u is lower, v upper bound
                for(auto index: clustering_structure[0].Member_Indices){
                    if(index > c1.v && index < c2.u){
                        event_interval_t.push_back(index);
                    }
                }
                if (!event_interval_t.empty()){
                    Checked_Cluster checkedCluster;
                    checkedCluster.c1 = c1;
                    checkedCluster.c2 = c2;
                    checkedCluster.event_interval_t = event_interval_t;
                    checked_clusters.push_back(checkedCluster);
                }
            }
        }
    }

    return checked_clusters;
}

Checked_Cluster Event_Detector::_compute_and_evaluate_loss(vector<Checked_Cluster> checked_clusters){
    /*
    Function to compute the loss values of the different cluster combinations.
            The formula for the loss, is explained in the doc-string in step 3 of the forward pass.
     Args:
    checked_clusters (list): of triples (c1, c2, event_interval_t)
    Returns:
    event_cluster_combination (tuple): triple of the winning cluster combination
    */
    int min_index;
    double event_model_loss = -1;
    for(int i =0; i < checked_clusters.size(); i++){
        int lower_event_bound_u = checked_clusters[i].event_interval_t[0] - 1;  // the interval starts at u + 1
        int upper_event_bound_v = checked_clusters[i].event_interval_t[checked_clusters[i].event_interval_t.size()-1] - 1;  // the interval ends at v -1
        //number of samples from c2 smaller than lower bound of event <u
        //number of samples from c1 greater than upper bound of event >v
        // number of samples n between u < n < v, so number of samples n in the event_interval_t that
        // belong to C1 or C2, i.e. to the stationary signal. u < v
        // c1 > u c2 < v
        int event_model_loss_temp=0;
        for(int temp: checked_clusters[i].c1.Member_Indices){
            if(temp > lower_event_bound_u){
                event_model_loss_temp++;
            }
        }
        for(int temp: checked_clusters[i].c2.Member_Indices){
            if(temp < upper_event_bound_v){
                event_model_loss_temp++;
            }
        }
        if(event_model_loss > event_model_loss_temp || event_model_loss == -1){
            event_model_loss = event_model_loss_temp;
            min_index = i;
        }
    }
    // Compare with the loss threshold, i.e. if the smallest loss is not smaller than the treshold, no other
    // loss will be in the array
    Checked_Cluster checkedCluster;
    if (event_model_loss <= this->loss_thresh) {  // if smaller than the threshold event detected
        checkedCluster = checked_clusters[min_index]; // get the winning event cluster combination
    }
    return checkedCluster;
}

Checked_Cluster Event_Detector::_roleback_backward_pass(bool status, vector<Cluster> backward_clustering_structure,
                                                        Checked_Cluster event_cluster_combination_balanced, int i){
    /*
    When the backward pass is performed, the oldest datapoint is removed in each iteration.
            After that, first the model constraints are evaluated.
            If they are violated, we roleback to the previous version by adding the oldest datapoint again and we are finished.
    Args:
    status (string): either "continue" or "break"
    i: current iteration index of the datapoint
    event_cluster_combination_balanced:
    event_cluster_combination_below_loss: NONE
    Returns:
*/

    if (status == false){
        // if the loss is above the threshold without the recently removed sample, take the previous combination and declare it as an
        // balanced event. the previous clustering and the previous event_cluster_combination are saved from the previous
        // run automatically, so there is no need to perform the clustering again. Attention: the event_interval indices are now matched to X_cut.
        //# We want them to match the original input X instead. Therefore we need to add + (i-1) to the indices, the  -1 is done because we take
        // the clustering and the state of X_cut from the previous, i.e. i-1, round. This is the last round where the loss,
        // was below the threshold, so it is still fine
        for(int ind = 0; ind < event_cluster_combination_balanced.event_interval_t.size(); ind++){
//            cout<< event_cluster_combination_balanced.event_interval_t[ind] << endl;
            event_cluster_combination_balanced.event_interval_t[ind] += i;// the event_interval_t
//            cout<< event_cluster_combination_balanced.event_interval_t[ind] << endl;
        }
        // The same is to be done for all the final cluster
        // The structure stored in self.backward_clustering_structure is valid, it is from the previous iteration
        for(int ind  = 0; ind < backward_clustering_structure.size(); ind++){
            // Only the "Loc" is not updated (stays the same, regardless of the indexing)
            backward_clustering_structure[ind].u += (i - 1);
            backward_clustering_structure[ind].v += (i - 1);
            for( int temp_ind = 0; temp_ind < backward_clustering_structure[ind].Member_Indices.size(); temp_ind++){
                backward_clustering_structure[ind].Member_Indices[temp_ind] += (i -1);
            }
        }
    }// else{
//        // continue with the backward pass
//        // update the backward_clustering_structure with the latest valid one
//        // i.e. the new clustering structure
//    backward_clustering_structure = clustering_structure;
//    event_cluster_combination_balanced = event_cluster_combination_balanced;  // same here
//    }
    return event_cluster_combination_balanced;

}