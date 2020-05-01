//
// Created by Administrator on 2/2/2020.
//

#include "query2.h"
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

bool sortFun(int a, int b)
{
    return a < b;
}

size_t writeFunction2(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}
using namespace rapidjson;
using namespace std;

extern vector<int> F1TestOriginal;
Query2::Query2() {

    struct timeval  startTime, endTime, entirStartTime, entirEndTime;
    long  prestepTime=0, step1Time=0, step2Time=0;
    long  initialization=0, networkTime=0, jsonTime=0;
    gettimeofday( &entirStartTime, NULL );

    cout << "Query 2 starting" << endl;

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
    curl_easy_setopt(curl_get, CURLOPT_URL, "http://localhost/data/2/");
//    curl_easy_setopt(curl_get, CURLOPT_TCP_NODELAY, 1);

    CURL *curl_post;
    curl_post = curl_easy_init();
    curl_easy_setopt(curl_post, CURLOPT_URL, "http://localhost/data/2/");

    //    std::string header_string;

    int sequenceNumber[1000];
    double voltage [1000];
    double currrent [1000];


    // benchmark the time
    gettimeofday( &entirEndTime, NULL );
    initialization= (1000000*(entirEndTime.tv_sec - entirStartTime.tv_sec) + (entirEndTime.tv_usec - entirStartTime.tv_usec));

    //statistics of data pattern
    long total = 0;
    long normal = 0;
    long duplication = 0;
    long delayed = 0;
    long missing = 0;
    long average_waiting_time = 0;
    long madium_waiting_time = 0;
    long minimum_waiting_time = 0;
    long maximum_waiting_time = 0;
    vector<int> waiting_time;

    int precision = 0;
    int recall = 0;

    while(true){
        //get the data
        gettimeofday( &startTime, NULL );
        std::string response_string;
        curl_easy_setopt(curl_get, CURLOPT_WRITEFUNCTION, writeFunction2);
        curl_easy_setopt(curl_get, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl_get, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_perform(curl_get);
        gettimeofday( &endTime, NULL );
        networkTime+= (1000000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec));

        gettimeofday( &startTime, NULL );

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
        gettimeofday( &endTime, NULL );
        jsonTime+= (1000000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec));



//        json j;
//        gettimeofday( &startTime, NULL );
//        j = json::parse(response_string);
//        gettimeofday( &endTime, NULL );
//        cout << (1000000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec)) << endl;
//        jsonTime+= (1000000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec));
//        j = j["records"];
//        if( j.empty())
//            break;

        int batch_left_boundary=batchCounter*1000;
        int batch_right_boundary=(batchCounter+1)*1000;
        for(int i = 0; i<1000; i++){
            sequenceNumber[i] = i + batch_left_boundary;
            voltage[i] = 2;
            currrent[i] = 2;
        }

        gettimeofday( &startTime, NULL );
        //PreStep
        //for (json::iterator it = j.begin(); it != j.end(); ++it) {
//        cout<< batch_left_boundary << "         " << j.size() << endl;
        for (SizeType i = 0; i < itr->value.Size(); i++){
            total++;
            int index = itr->value[i]["i"].GetInt();
            index -= batch_left_boundary;
            if(index >= 0 && index < 1000){
                if(voltage[index] != 2) {
                    duplication++;
                }else {
                    normal++;
                }
                sequenceNumber[index] = itr->value[i]["i"].GetInt();
                voltage[index] = itr->value[i]["voltage"].GetDouble();
                currrent[index] = itr->value[i]["current"].GetDouble();
//                cout << itr->value[i]["i"].GetInt() << "    " << itr->value[i]["voltage"].GetDouble() <<"     " << itr->value[i]["current"].GetDouble() << endl;
            }
            if(index < 0) {
                delayed++;
                average_waiting_time += (batchCounter - (itr->value[i]["i"].GetInt() / 1000));
//                if(batchCounter - (itr->value[i]["i"].GetInt() / 1000) > 20){
//                    cout<< batchCounter << "   "  << itr->value[i]["i"].GetInt() << endl;
//                }
                waiting_time.push_back((batchCounter - (itr->value[i]["i"].GetInt() / 1000)));
            }
        }
        gettimeofday( &endTime, NULL );
        prestepTime+= (1000000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec));

//        if( itr->value.Size() == 555){
////            for( int i = 0; i <  1000; i++){
////                cout << sequenceNumber[i] << "    " << voltage[i] << "     " << currrent[i] << endl;
////            }
//            for( int i = 0; i <  itr->value.Size(); i++){
//                cout << itr->value[i]["i"].GetInt() << "    " << itr->value[i]["voltage"].GetDouble() <<"     " << itr->value[i]["current"].GetDouble() << endl;
//            }
//        }

        feature_index += 1;
        gettimeofday( &startTime, NULL );
        // Compute the feature for the 1000 samples we have buffered
        Feature X_i = EventDet_Barsim.compute_input_signal(voltage, currrent, period, true);
        // append the newly computed feature point to the features streamed
        //features_streamed.push_back(X_i);
//        cout<< batch_left_boundary << "     " << X_i.active_power_P << "    " << X_i.reactive_power_Q << endl;
        gettimeofday( &endTime, NULL );
        step1Time += (1000000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec));

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
        gettimeofday( &startTime, NULL );
        Event event_interval_indices = EventDet_Barsim.predict(batchCounter, X);
        gettimeofday( &endTime, NULL );
//        cout<< endTime.tv_sec - startTime.tv_sec  <<endl;
//        cout<< endTime.tv_usec - startTime.tv_usec  <<endl;
        step2Time+= (1000000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec));

//        cout <<current_window_start << "    " << X.size() << endl;
        //change the json library
//        json result;
//        result["s"] = batchCounter;
        std::string jsonTemp =   "{\"s\":";
        jsonTemp.append(to_string (batchCounter));

        if (event_interval_indices.event_end != 0){  // if an event is returned
            cout << "Event Detected at " << current_window_start + event_interval_indices.event_start << "," <<
                 current_window_start + event_interval_indices.event_end << endl;
            // Instead of an event interval, we might be interested in an exact event point
            // Hence, we just take the mean of the interval boundaries
            int mean_event_index = (event_interval_indices.event_start + event_interval_indices.event_end) / 2;
//            cout << current_window_start << endl;
//            cout << event_interval_indices.event_start << endl;
//            cout << event_interval_indices.event_end << endl;

            // Now we create a new data window X
            // We take all datapoints that we have already receveived, beginning form the index of the event (the end index of the event interval in this case)
            // the end_index of the event
            for( int i = 0; i<event_interval_indices.event_end; i++){
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
            if (F1TestOriginal[batchCounter] == 0) {
                cout << "re   " << batchCounter << endl;
                recall++;
            }
        }else{
            // We start at the end of the previous window
            // Hence, at first, we do nothing, except the maximum window size is exceeded
            if (X.size() > MAXIMUM_WINDOW_SIZE){
                // if the maximum window size is exceeded
                X.clear();  // Reset X
                //change the json library
//                result["d"] = false;
//                result["event_s"] = -1;
            }
            jsonTemp.append(",\"d\":0,\"event_s\":");
            jsonTemp.append(to_string(-1));

            if (F1TestOriginal[batchCounter] ==  1) {
                cout << "pre   " << batchCounter << endl;
                precision++;
            }

        }
        jsonTemp.append("}");



//                cout << jsonTemp.c_str() << endl;
        std::string response_string_post;
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_post, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl_post, CURLOPT_POSTFIELDS, jsonTemp.c_str());
        curl_easy_setopt(curl_post, CURLOPT_WRITEFUNCTION, writeFunction2);
        curl_easy_setopt(curl_post, CURLOPT_WRITEDATA, &response_string_post);
        curl_easy_perform(curl_post);

        batchCounter += 1;
//        curl_easy_setopt(curl_post, CURLOPT_POSTFIELDSIZE, result.dump().length());
//        curl_easy_setopt(curl_post, CURLOPT_POST, 1);
//        curl_easy_setopt(curl_post, CURLOPT_COPYPOSTFIELDS, result.dump().c_str());
//        curl_easy_perform(curl_post);

    }
    cout << recall<< endl;
    cout << precision<< endl;
//    curl_easy_cleanup(curl_get);
//    curl_easy_cleanup(curl_post);

//    int recorder[22];
//    for(int i = 0; i < 22; i++){
//        recorder[i] = 0;
//    }
//    missing = batchCounter*1000 - duplication - delayed - normal;
//    sort(waiting_time.begin(), waiting_time.end(),sortFun);
//        for(int i = 0; i < waiting_time.size(); i++){
//            recorder[waiting_time[i]]++;
//        }
//    for(int i = 0; i < 22; i++){
//        cout<<i << ": " << recorder[i]<<endl;
//    }


    cout << "> Query 2 done!" << endl;

//    cout<< "initialization: " << initialization<<endl;
//    cout<< "networkTime: " << networkTime<<endl;
//    cout<< "jsonTime: " << jsonTime<<endl;
//    cout<< "prestepTime: " << prestepTime<<endl;
//    cout<< "step1Time: " << step1Time<<endl;
//    cout<< "step2Time: " << step2Time<<endl;
//
//    cout<< "all: " << batchCounter*1000<<endl;
//    cout<< "total: " << total<<endl;
//    cout<< "normal: " << normal<<endl;
//    cout<< "duplication: " << duplication<<endl;
//    cout<< "delayed: " << delayed<<endl;
//    cout<< "missing: " << missing<<endl;
//    cout<< "average_waiting_time: " << average_waiting_time/delayed<<endl;
//    cout<< "3/4medium_waiting_time: " << waiting_time[3*waiting_time.size()/4]<<endl;
//    cout<< "2/4medium_waiting_time: " << waiting_time[waiting_time.size()/2]<<endl;
//    cout<< "1/4medium_waiting_time: " << waiting_time[waiting_time.size()/4]<<endl;
//    cout<< "maximum_waiting_time: " << waiting_time[waiting_time.size() - 1]<<endl;
//    cout<< "minimum_waiting_time: " << waiting_time[0]<<endl;

//    cout<< "dbscanTime: " << EventDet_Barsim.dbscanTime<<endl;
//    cout<< "eventTime: " << EventDet_Barsim.eventTime<<endl;
//    cout<< "thresholdTime: " << EventDet_Barsim.thresholdTime<<endl;
//    cout<< "verifyTime: " << EventDet_Barsim.verifyTime<<endl;

}