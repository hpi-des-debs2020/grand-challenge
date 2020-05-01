//
// Created by Administrator on 2/20/2020.
//
#include "query2_NonParallel.h"
#include "Incremental_Detector_Disorder.h"

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
#include <condition_variable>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <curl/curl.h>
#include <unistd.h>
#include <list>

using namespace rapidjson;
using namespace std;

size_t writeFunction_Parallel2(void *ptr, size_t size, size_t nmemb, std::string* data) {
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
int NETWORK_FREQUENCY_Query2 = 50;
int SAMPLERATE_Query2 = 50000;  //Sampling Rate the raw Dataset
//Compute the period size of the dataset: i.e. number of raw data points per period
int period_Query2 = SAMPLERATE_Query2 / NETWORK_FREQUENCY_Query2;
int MAXIMUM_WINDOW_SIZE_Query2 = 100;

//to predict the event
int Radius_Query2 =7;
//the queue to store input tuple
//the size of queue
const int queueSize_Query2 = 30;
//maximum size of buffer
int maxThrehold_Query2 = 1000;
//maximum waiting time for late tuples
int lateTime_Query2 = 20;
extern vector<int> F1Test;

Query2_NonParallel::Query2_NonParallel(string address) {
    cout << "Query 2 starting" << endl;

//    struct timeval  startTime1, endTime1;
    long taskTime = 0;
//    gettimeofday( &startTime1, NULL );
    Init_dict init_dict;
    Incremental_Detector_Disorder incremental_Detector_Disorder(init_dict);
    vector<Tuple> inputBatch_Query2[queueSize_Query2];
    list<int> waitBatch;


    CURL *curl_get;
    curl_get = curl_easy_init();
    curl_easy_setopt(curl_get, CURLOPT_URL, address.c_str());

    CURL *curl_post;
    curl_post = curl_easy_init();
    curl_easy_setopt(curl_post, CURLOPT_URL, address.c_str());

    int batchNumber = 0;
    int precision = 0;
    int recall = 0;
    int disorderCount = 0;
    while(true){
        //get the data
        std::string response_string;
        curl_easy_setopt(curl_get, CURLOPT_WRITEFUNCTION, writeFunction_Parallel2);
        curl_easy_setopt(curl_get, CURLOPT_WRITEDATA, &response_string);
        curl_easy_perform(curl_get);

        if(response_string.empty()){
            usleep(1000000);
//            gettimeofday( &startTime1, NULL );
            continue;
        }

//        cout<< response_string.size() << endl;
        //the size of last one is 16 the size of error is 37
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
//        cout<<batchNumber << "  "  << F1Test[batchNumber]<<endl;
//        int batchNumber = itr->value[0]["i"].GetInt() / 1000;
        for (int i = 0; i < itr->value.Size(); i++) {
            struct Tuple tuple;
            tuple.sequenceNumber = itr->value[i]["i"].GetInt();
            tuple.voltage = itr->value[i]["voltage"].GetDouble();
            tuple.current = itr->value[i]["current"].GetDouble();

//            if( batchNumber - (tuple.sequenceNumber / 1000)  <= 20 ) {
                inputBatch_Query2[(tuple.sequenceNumber / 1000) % queueSize_Query2].push_back(tuple);
//                disorderCount++;
//            }
//            cout << tuple.sequenceNumber << "   " << tuple.sequenceNumber / 1000  << "   " << (tuple.sequenceNumber / 1000) % 20 << endl;
//            cout << (tuple.sequenceNumber / 1000) % 20 << "  " << inputBatch[ (tuple.sequenceNumber / 1000) % 20 ].size() << endl;
        }

        for (int i = 0; i < queueSize_Query2; i++) {
            //            the queue is not empty
            if (!inputBatch_Query2[i].empty()) {
                int batchSequence = inputBatch_Query2[i].back().sequenceNumber / 1000;
//                if (inputBatch_Query2[i].size() == maxThrehold_Query2 || (batchNumber - batchSequence ) >= 20) {
                if (inputBatch_Query2[i].size() == maxThrehold_Query2 || (batchNumber - batchSequence ) >= lateTime_Query2) {
//                if (inputBatch_Query2[i].size() >= 1000 || (batchNumber - batchSequence ) >= 20) {
                    double voltage[1000];
                    double current[1000];
                    //initialization
                    for (int j = 0; j < 1000; j++) {
                        voltage[j] = 2;
                        current[j] = 2;
                    }
                    int tempQueueSize = inputBatch_Query2[i].size();
                    for (int j = 0; j < tempQueueSize; j++) {
                        Tuple tuple = inputBatch_Query2[i][j];
                        voltage[j] = tuple.voltage;
                        current[j] = tuple.current;
                    }
                    inputBatch_Query2[i].clear();
                    Feature X = incremental_Detector_Disorder.compute_input_signal(voltage, current, period_Query2,true);
                    X.sequenceNumber = batchSequence;
                    //for badpoint
                    if(tempQueueSize >= 900) {
                        X.badPoint = -1;
                    }
                    else{
                        X.badPoint = 1;
                    }
//                        cout<< X.sequenceNumber << "  " << X.active_power_P << "  " << X.reactive_power_Q << "   " << X.badPoint << endl;

                    //if we find a feature
                    vector<Result> resultBucket;
                    //because the predict would refresh the window of predictEvent, put the predictevent before predict
                    int eventPredict = incremental_Detector_Disorder.predictEvent(X,Radius_Query2);
                    vector<Event> event_interval_indices_vector = incremental_Detector_Disorder.predict(X.sequenceNumber, X);
                    //maaybe there is an event
//                    eventPredict = 1; //**************************************
                    if(eventPredict == 1){
                        waitBatch.push_back(X.sequenceNumber);
                    }
                        //surely it is not an event
                    else{
                        Result result_temp;
                        result_temp.s = X.sequenceNumber;
                        result_temp.d = 0;
                        result_temp.event_s = -1;
                        resultBucket.push_back(result_temp);
//                      cout<<"1:   "<<result_temp.s << "  " << result_temp.d << "  " << result_temp.event_s << endl;
                    }
                    //**************************************
                    waitBatch.sort();

                    if(!event_interval_indices_vector.empty()) {
                        //the event_interval_indices_vector is ordered since all the events are pushed back
                        for (int eventId = 0; eventId < event_interval_indices_vector.size(); eventId++) {
                            Event event_interval_indices = event_interval_indices_vector[eventId];
                            if (event_interval_indices.event_end != 0) {  // if an event is returned
                                cout << "batchNumber" <<  X.sequenceNumber << "  Event Detected at   " << event_interval_indices.event_start << "," <<
                                     event_interval_indices.event_end << endl;

//                        double mean_event_index = (event_interval_indices.event_start + event_interval_indices.event_end) / 2.0 + 1;
                                int mean_event_index = (event_interval_indices.event_start + event_interval_indices.event_end) / 2 + 1;

                                //send the result with event
                                Result result_temp;
                                result_temp.s = event_interval_indices.sequenceNumber;
                                result_temp.d = 1;
                                result_temp.event_s = mean_event_index;
                                resultBucket.push_back(result_temp);
//                               cout<<"2:   "<<result_temp.s << "  " << result_temp.d << "  " << result_temp.event_s << endl;
                                waitBatch.remove(event_interval_indices.sequenceNumber);
                            }
                        }
                    }else{
                        incremental_Detector_Disorder.refresh_window(MAXIMUM_WINDOW_SIZE_Query2);
                    }

                    //clear the waitBatch
                    int tempWaitSize = waitBatch.size();
                    for(int i = 0; i< tempWaitSize; i++){
                        if(waitBatch.front() <= incremental_Detector_Disorder.mapFeatureBlockedPos){
                            Result result_temp;
                            result_temp.s = waitBatch.front();
                            result_temp.d = 0;
                            result_temp.event_s = -1;
                            resultBucket.push_back(result_temp);
                            waitBatch.pop_front();
//                            cout<<"3:   "<<result_temp.s << "  " << result_temp.d << "  " << result_temp.event_s << endl;
                        }
                    }
                    event_interval_indices_vector.clear();

                    for(int i = 0; i< resultBucket.size(); i++){
                        std::string jsonTemp =   "{\"s\":";
                        jsonTemp.append(to_string (resultBucket[i].s));
                        if(resultBucket[i].d == 0){
                            jsonTemp.append(",\"d\":0,\"event_s\":");
                        }else{
                            jsonTemp.append(",\"d\":1,\"event_s\":");
                        }
                        if(resultBucket[i].event_s == -1){
                            jsonTemp.append(to_string(-1));
                        }else {
                            jsonTemp.append(to_string((int) resultBucket[i].event_s));
                        }
                        jsonTemp.append("}");

//                        cout << jsonTemp.c_str() << endl;

                        //********************************************
                        if(F1Test[resultBucket[i].s] != -1){
                            if (resultBucket[i].d == 1) {
                                if (F1Test[resultBucket[i].s] == 0) {
                                    cout << "re   " << resultBucket[i].s << endl;
                                    recall++;
                                }
                            } else {
                                if (F1Test[resultBucket[i].s] == 1) {
                                    cout << "pre   " << resultBucket[i].s << endl;
                                    precision++;
                                }
                            }
                            F1Test[resultBucket[i].s] = -1;
                        }



                        std::string response_string_post;
                        struct curl_slist *headers = NULL;
                        headers = curl_slist_append(headers, "Content-Type: application/json");
                        curl_easy_setopt(curl_post, CURLOPT_HTTPHEADER, headers);
                        curl_easy_setopt(curl_post, CURLOPT_POSTFIELDS, jsonTemp.c_str());
                        curl_easy_setopt(curl_post, CURLOPT_WRITEFUNCTION, writeFunction_Parallel2);
                        curl_easy_setopt(curl_post, CURLOPT_WRITEDATA, &response_string_post);
                        curl_easy_perform(curl_post);
                    }
                    resultBucket.clear();

                }
            }
        }
        //**************************************
        batchNumber++;

    }
//    gettimeofday( &endTime1, NULL );
//    taskTime+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));
    cout << "> Query 2 done!" << endl;
//    cout << disorderCount<< endl;
    cout << recall<< endl;
    cout << precision<< endl;

}