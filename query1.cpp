//
// Created by Administrator on 2/2/2020.
//

#include "query1.h"
#include "Event_Detector.h"
#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <curl/curl.h>
#include <vector>
#include <thread>
#include <unistd.h>
#include<algorithm>

size_t writeFunction1(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

double getDis(Feature a, Feature b) {

    return sqrt((a.active_power_P-b.active_power_P)*(a.active_power_P-b.active_power_P)+
                (a.reactive_power_Q-b.reactive_power_Q)*(a.reactive_power_Q-b.reactive_power_Q));
}

using namespace rapidjson;
using namespace std;

extern vector<int> F1TestOriginal;
Query1::Query1() {

    int refrsh = 0;
    int average_of_window_size = 0;
    int event = 0;
    int gap = 0;

    cout << "Query 1 starting" << endl;

    //Base electrical network frequency of the region where the dataset was recorded
    int NETWORK_FREQUENCY = 50;
    //We compute 50 features (data points) per second, once every 1000 samples
    int VALUES_PER_SECOND = 50;
    int SAMPLERATE = 50000;  //Sampling Rate the raw Dataset

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

    //Compute some relevant window sizes etc. for the "streaming"
    int window_size_seconds = init_dict.window_size_n / VALUES_PER_SECOND;

    //Compute the period size of the dataset: i.e. number of raw data points per period
    int period = SAMPLERATE / NETWORK_FREQUENCY;

    Event_Detector EventDet_Barsim(init_dict);
    //ed.STREAMING_EventDet_Barsim_Sequential(&init_dict);  //i.e. values are unpacked into the parameters
    //Call the fit() method to further initialize the algorithm (required by the sklearn API)

//    EventDet_Barsim.fit();


    int current_window_start = 0;
    int current_window_end = 0;

    int MAXIMUM_WINDOW_SIZE = 100;  //2 seconds

    // the to the feature domain converted data that we have already receievd from the stream
    //    vector<Feature> features_streamed;
    // the data (feature domain) that is used for the prediction, i.e. our current window
    vector<Feature> X;


    cout << "Getting data in batches.." << endl;

    // Here is a script to get the data in batches and give back the results
    // Recieved data is in JSON format, with attributes {'i':,'voltage':,'current':}
    // For each batch, you produce a result with format {'ts':,'detected':,'event_ts':}
    int batchCounter = 0;
    int feature_index = 0;

    CURL *curl_get;
    curl_get = curl_easy_init();
    curl_easy_setopt(curl_get, CURLOPT_URL, "http://localhost/data/1/");

    CURL *curl_post;
    curl_post = curl_easy_init();
    curl_easy_setopt(curl_post, CURLOPT_URL, "http://localhost/data/1/");

    //    std::string header_string;

    int sequenceNumber[1000];
    double voltage [1000];
    double currrent [1000];

    while(true){
        //get the data
        std::string response_string;
        curl_easy_setopt(curl_get, CURLOPT_WRITEFUNCTION, writeFunction1);
        curl_easy_setopt(curl_get, CURLOPT_WRITEDATA, &response_string);
        curl_easy_perform(curl_get);

        if(response_string.empty()){
            usleep(1000000);
            continue;
        }

        if(response_string.size() == 16) {
            cout<< response_string << endl;
            continue;
        }

        Document document;
        document.Parse(response_string.c_str());
        Value::ConstMemberIterator itr = document.FindMember("records");
        if (itr == document.MemberEnd()) {
//            cout << endFlag << endl;
            cout << response_string << endl;
            usleep(1000000);
            break;
        }

        //for (json::iterator it = j.begin(); it != j.end(); ++it) {
        for(int i = 0; i<1000; i++){
            sequenceNumber[i] = itr->value[i]["i"].GetInt();
            voltage[i] = itr->value[i]["voltage"].GetDouble();
            currrent[i] = itr->value[i]["current"].GetDouble();
        }

        feature_index += 1;

        // Compute the feature for the 1000 samples we have buffered
        Feature X_i = EventDet_Barsim.compute_input_signal(voltage, currrent, period, true);
        // append the newly computed feature point to the features streamed
        //features_streamed.push_back(X_i);
        average_of_window_size+=X.size();
        if(X.size()>=1){
            Feature X_temp = X.back();
            if (getDis(X_temp, X_i) > init_dict.dbscan_eps ){
//                cout << "batch: " << batchCounter << "  there is a gap  "<< endl;
                gap++;
            }
        }

        int window_start_index;
        int current_window_start;
        int current_window_end;
        if (X.empty()) {
            X.push_back(X_i); //if it is the first point in our window
            window_start_index = feature_index;  // the index the window is starting with
            current_window_start = feature_index;
            current_window_end = feature_index;
        }
        else {
            // add new feature point to window
            X.push_back(X_i);
            current_window_end = current_window_end + 1;
        }

        Event event_interval_indices = EventDet_Barsim.predict(batchCounter, X);
//        cout <<current_window_start << "    " << X.size() << endl;
        //change the json library
//        json result;
//        result["s`"] = batchCounter;
        std::string jsonTemp =   "{\"s\":";
        jsonTemp.append(to_string (batchCounter));


        if (event_interval_indices.event_end != 0){  // if an event is returned
            cout << "Event Detected at                    " << current_window_start + event_interval_indices.event_start<< "," <<
                 current_window_start + event_interval_indices.event_end<< endl;
            event++;
            // Instead of an event interval, we might be interested in an exact event point
            // Hence, we just take the mean of the interval boundaries
            int mean_event_index = (event_interval_indices.event_start + event_interval_indices.event_end) / 2;
//            cout << current_window_start << endl;
//            cout << event_interval_indices.event_start << endl;
//            cout << event_interval_indices.event_end << endl;

            // Now we create a new data window X
            // We take all datapoints that we have already receveived, beginning form the index of the event (the end index of the event interval in this case)
            // the end_index of the event
            for( int i = 0; i<= event_interval_indices.event_end; i++){
                X.erase(X.begin());
            }

            // the index the window is starting with
            window_start_index = window_start_index + event_interval_indices.event_end;

            //change the json library
//            result["d"] = true;
//            result["event_s"] = current_window_start+mean_event_index;
            jsonTemp.append(",\"d\":1,\"event_s\":");
            jsonTemp.append(to_string(current_window_start+mean_event_index));

            current_window_start = window_start_index;
            F1TestOriginal.push_back(1);
        }else{
            // We start at the end of the previous window
            // Hence, at first, we do nothing, except the maximum window size is exceeded
            if (X.size() > MAXIMUM_WINDOW_SIZE){
                // if the maximum window size is exceeded
                X.clear();  // Reset X
                refrsh++;
//                cout << "batch: " << batchCounter << " -------------------------- refresh window  "<< endl;
                //change the json library
//                result["d"] = false;
//                result["event_s"] = -1;
            }
            jsonTemp.append(",\"d\":0,\"event_s\":");
            jsonTemp.append(to_string(-1));
            F1TestOriginal.push_back(0);
        }


        jsonTemp.append("}");

//                cout << jsonTemp.c_str() << endl;
        std::string response_string_post;
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_post, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl_post, CURLOPT_POSTFIELDS, jsonTemp.c_str());
        curl_easy_setopt(curl_post, CURLOPT_WRITEFUNCTION, writeFunction1);
        curl_easy_setopt(curl_post, CURLOPT_WRITEDATA, &response_string_post);
        curl_easy_perform(curl_post);

        batchCounter += 1;
//        curl_easy_setopt(curl_post, CURLOPT_POSTFIELDS, result.dump().c_str());
//        curl_easy_perform(curl_post);



    }
//    curl_easy_cleanup(curl_get);
//    curl_easy_cleanup(curl_post);
//    cout << "> Query 1 done!" << endl;
//
//    for(int i = 0; i < 20; i++){
//        cout<< "------cluster id:  " << i <<endl;
//        for(int j = 0; j<101;j++){
//            cout << j << ":  "<< EventDet_Barsim.cluster_counter[i][j] << endl;
//        }
//    }

//cout<< "refresh: " << refrsh << endl;
//cout<< "average of window size: " << average_of_window_size/batchCounter<< endl;
//cout<< "gap: " << gap<< endl;
//cout<< "event: " << event<< endl;
//cout<< "cluster amount: " << EventDet_Barsim.cluster_number<< endl;
//cout<< "cluster more than 2: " << EventDet_Barsim.cluster_number_more_than_2<< endl;
//cout<< "average size of cluster0: " << EventDet_Barsim.cluster0_size/EventDet_Barsim.cluster0_time<< endl;
//cout<< "average size of cluster1: " << EventDet_Barsim.cluster1_size/EventDet_Barsim.cluster1_time<< endl;
//cout<< "average size of clusterX: " << EventDet_Barsim.clusterX_size/EventDet_Barsim.clusterX_time<< endl;



}