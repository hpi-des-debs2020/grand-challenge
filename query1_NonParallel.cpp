//
// Created by Administrator on 2/20/2020.
//
#include "query1_NonParallel.h"
#include "Incremental_Detector.h"

#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <array>
#include <map>
#include <thread>
#include <mutex>
#include <chrono>
#include <future>
#include <list>
#include <condition_variable>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <curl/curl.h>
#include <unistd.h>

using namespace rapidjson;
using namespace std;

size_t writeFunction_Parallel(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

struct Tuple{
    int sequenceNumber;
    double voltage;
    double current;
};

struct Result{
    int s;
    int d;
    int event_s;
};

//Base electrical network frequency of the region where the dataset was recorded
int NETWORK_FREQUENCY_Query1 = 50;
int SAMPLERATE_Query1 = 50000;  //Sampling Rate the raw Dataset
//Compute the period size of the dataset: i.e. number of raw data points per period
int period_Query1 = SAMPLERATE_Query1 / NETWORK_FREQUENCY_Query1;
int MAXIMUM_WINDOW_SIZE_Query1 = 100;
extern vector<int> F1Test;

Query1_NonParallel::Query1_NonParallel(string address) {

    cout << "Query 1 starting" << endl;


//    struct timeval  startTime1, endTime1;
//    long step1 = 0, step2 = 0, step3 = 0, step4 = 0;
    long taskTime = 0;
//    gettimeofday( &startTime1, NULL );


    Init_dict init_dict;
    Incremental_Detector incremental_Detector(init_dict);

    CURL *curl_get;
    curl_get = curl_easy_init();
//    curl_easy_setopt(curl_get, CURLOPT_INTERFACE, "eth0");
    curl_easy_setopt(curl_get, CURLOPT_URL, address.c_str());

    CURL *curl_post;
    curl_post = curl_easy_init();
    curl_easy_setopt(curl_post, CURLOPT_URL, address.c_str());

    while(true){
        //get the data
        std::string response_string;
        curl_easy_setopt(curl_get, CURLOPT_WRITEFUNCTION, writeFunction_Parallel);
        curl_easy_setopt(curl_get, CURLOPT_WRITEDATA, &response_string);

//        gettimeofday( &startTime1, NULL );
        curl_easy_perform(curl_get);
//        fprintf(stderr, "curl_easy_perform() failed: %s\n",
//                curl_easy_strerror(res));

        if(response_string.empty()){
            usleep(1000000);
//            gettimeofday( &startTime1, NULL );
            continue;
        }

        if(response_string.size() == 16) {
            cout<< response_string << endl;
            continue;
        }
//        gettimeofday( &endTime1, NULL );
//        step1+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));

//        gettimeofday( &startTime1, NULL );

        Document document;
        document.Parse(response_string.c_str());
        Value::ConstMemberIterator itr = document.FindMember("records");
        if (itr == document.MemberEnd()) {
//            cout << endFlag << endl;
            cout << response_string << endl;
            usleep(1000000);
            break;
        }
//        cout <<taskId<<"   "<< response_string << endl;
//        batchCounter.fetch_add(1);

//        for (json::iterator it = j.begin(); it != j.end(); ++it) {
//        vector<Tuple> tupleBucket;
        double voltage[1000];
        double current[1000];
        int batchNumber = itr->value[0]["i"].GetInt() / 1000;
        for (int i = 0; i < itr->value.Size(); i++) {
//            struct Tuple tuple;
            voltage[i] = itr->value[i]["voltage"].GetDouble();
            current[i] = itr->value[i]["current"].GetDouble();
//            cout << tuple.sequenceNumber << "   " << tuple.sequenceNumber / 1000  << "   " << (tuple.sequenceNumber / 1000) % 20 << endl;
//            cout << (tuple.sequenceNumber / 1000) % 20 << "  " << inputBatch[ (tuple.sequenceNumber / 1000) % 20 ].size() << endl;
        }
        Feature X = incremental_Detector.compute_input_signal(voltage, current, period_Query1, true);
        X.sequenceNumber = batchNumber;
//        gettimeofday( &endTime1, NULL );
//        step2+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));
//        cout<< X.sequenceNumber << "  " << X.reactive_power_Q << " "  << X.active_power_P <<endl;

//        gettimeofday( &startTime1, NULL );
        Event event_interval_indices = incremental_Detector.predict(batchNumber, X);
//        gettimeofday( &endTime1, NULL );
//        step3+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));

//        gettimeofday( &startTime1, NULL );
        Result result_temp;
        result_temp.s = X.sequenceNumber;
        if (event_interval_indices.event_end != 0) {  // if an event is returned
            cout << "batchNumber " <<  X.sequenceNumber << "  Event Detected at   " << event_interval_indices.event_start << "," <<
                 event_interval_indices.event_end << endl;

//                double mean_event_index = (event_interval_indices.event_start + event_interval_indices.event_end) / 2.0 + 1;
            int mean_event_index = (event_interval_indices.event_start + event_interval_indices.event_end) / 2 + 1;

//            result_temp.s = X.sequenceNumber;
            result_temp.d = 1;
            result_temp.event_s = mean_event_index;
//            resultBucket.push_back(result_temp);
            F1Test.push_back(1);

        } else {
//            result_temp.s = X.sequenceNumber;
            result_temp.d = 0;
            result_temp.event_s = -1;
            incremental_Detector.refresh_window(MAXIMUM_WINDOW_SIZE_Query1);

            F1Test.push_back(0);
        }
//        cout<< result_temp.s << "  " << result_temp.d << " "  << result_temp.event_s <<endl;

        std::string jsonTemp =   "{\"s\":";
        jsonTemp.append(to_string (result_temp.s));
        if(result_temp.d == 0){
            jsonTemp.append(",\"d\":0,\"event_s\":");
        }else{
            jsonTemp.append(",\"d\":1,\"event_s\":");
        }
        if(result_temp.event_s == -1){
            jsonTemp.append(to_string(-1));
        }else {
            jsonTemp.append(to_string(result_temp.event_s));
        }
        jsonTemp.append("}");

//                cout << jsonTemp.c_str() << endl;
        std::string response_string_post;
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_post, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl_post, CURLOPT_POSTFIELDS, jsonTemp.c_str());
        curl_easy_setopt(curl_post, CURLOPT_WRITEFUNCTION, writeFunction_Parallel);
        curl_easy_setopt(curl_post, CURLOPT_WRITEDATA, &response_string_post);
        curl_easy_perform(curl_post);
//        gettimeofday( &endTime1, NULL );
//        step4+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));
//                CURLcode res =  curl_easy_perform(curl_post);
//                fprintf(stderr, "curl_easy_perform() failed: %s\n",
//                        curl_easy_strerror(res));


    }


//    gettimeofday( &endTime1, NULL );
//    taskTime+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));
    cout << taskTime << endl;
    cout << "> Query 1 done!" << endl;

//    cout << step1<< endl;
//    cout << step2<< endl;
//    cout << step3<< endl;
//    cout << step4<< endl;

}