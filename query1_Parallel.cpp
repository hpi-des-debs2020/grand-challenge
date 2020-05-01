////
//// Created by Administrator on 2/20/2020.
////
//#include "query1_Parallel.h"
//#include "Incremental_Detector.h"
//
//#include <iostream>
//#include <vector>
//#include <deque>
//#include <string>
//#include <array>
//#include <map>
//#include <thread>
//#include <mutex>
//#include <chrono>
//#include <future>
//#include <list>
//#include <condition_variable>
//#include "rapidjson/document.h"
//#include "rapidjson/writer.h"
//#include "rapidjson/stringbuffer.h"
//#include <curl/curl.h>
//#include <unistd.h>
//
//using namespace rapidjson;
//using namespace std;
//
//size_t writeFunction_Parallel(void *ptr, size_t size, size_t nmemb, std::string* data) {
//    data->append((char*) ptr, size * nmemb);
//    return size * nmemb;
//}
//
//struct Tuple{
//    int sequenceNumber;
//    double voltage;
//    double current;
//};
//
//struct Result{
//    int s;
//    int d;
//    int event_s;
//};
//
////Base electrical network frequency of the region where the dataset was recorded
//int NETWORK_FREQUENCY_Query1 = 50;
//int SAMPLERATE_Query1 = 50000;  //Sampling Rate the raw Dataset
////Compute the period size of the dataset: i.e. number of raw data points per period
//int period_Query1 = SAMPLERATE_Query1 / NETWORK_FREQUENCY_Query1;
//int MAXIMUM_WINDOW_SIZE_Query1 = 100;
//
////the buffer for network batch queue which can avoid resource competition of inputBatch.
//list<vector<Tuple>> dataBuffer_Query1;
////reorder the data
//list<Feature> featureBufer_Query1;
////share the data between sent thread and find thread
//list<vector<Result>> resultBuffer_Query1;
//
//// the lock between fetch thread and merge thread
//mutex networkAndJsonMutex_Query1;
////for condition variable to wake up the merge thread
//condition_variable mergeCond_Query1;
//// the lock between find thread and merge thread
//mutex findMutex_Query1;
////for condition variable to wake up the merge thread
//condition_variable findCond_Query1;
//// the lock between find thread and merge thread
//mutex sendMutex_Query1;
////for condition variable to wake up the merge thread
//condition_variable sendCond_Query1;
//
////flag notify all threads to stop
//bool endFlag_Query1 = false;
//bool mergeNotifyFlag_Query1 = false;
//bool findNotifyFlag_Query1 = false;
//bool sendNotifyFlag_Query1 = false;
//
//
//
///*
// The program would create a thread based on that function to fetch data from the http sever which support asynchronization .
// And it will put the input batch that include 1000 tuples into the deque.
// */
//long networkTask_Query1(int taskId, string address){
//    cout << "entry network task: " << taskId << endl;
//    struct timeval  startTime1, endTime1;
//    long taskTime = 0;
//    gettimeofday( &startTime1, NULL );
//
//    CURL *curl_get;
//    curl_get = curl_easy_init();
////    curl_easy_setopt(curl_get, CURLOPT_INTERFACE, "eth0");
//    curl_easy_setopt(curl_get, CURLOPT_URL, address.c_str());
//
//    while(true){
//        //get the data
//        std::string response_string;
//        curl_easy_setopt(curl_get, CURLOPT_WRITEFUNCTION, writeFunction_Parallel);
//        curl_easy_setopt(curl_get, CURLOPT_WRITEDATA, &response_string);
//        CURLcode res = curl_easy_perform(curl_get);
////        fprintf(stderr, "curl_easy_perform() failed: %s\n",
////                curl_easy_strerror(res));
//
//        if(response_string.empty()){
//            usleep(1000000);
//            gettimeofday( &startTime1, NULL );
//            continue;
//        }
//
//        if(response_string.size() == 16) {
//            cout<< response_string << endl;
//            continue;
//        }
//
//        Document document;
//        document.Parse(response_string.c_str());
//        Value::ConstMemberIterator itr = document.FindMember("records");
//        if (itr == document.MemberEnd() || endFlag_Query1) {
////            cout << endFlag << endl;
//            cout <<taskId<<"   "<< response_string << endl;
//            usleep(1000000);
//            endFlag_Query1 = true;
//            break;
//        }
////        cout <<taskId<<"   "<< response_string << endl;
////        batchCounter.fetch_add(1);
//
////        for (json::iterator it = j.begin(); it != j.end(); ++it) {
//        vector<Tuple> tupleBucket;
//        for (int i = 0; i < itr->value.Size(); i++) {
//            struct Tuple tuple;
//            tuple.sequenceNumber = itr->value[i]["i"].GetInt();
//            tuple.voltage = itr->value[i]["voltage"].GetDouble();
//            tuple.current = itr->value[i]["current"].GetDouble();
////            cout << tuple.sequenceNumber << "   " << tuple.sequenceNumber / 1000  << "   " << (tuple.sequenceNumber / 1000) % 20 << endl;
////            cout << (tuple.sequenceNumber / 1000) % 20 << "  " << inputBatch[ (tuple.sequenceNumber / 1000) % 20 ].size() << endl;
//            tupleBucket.push_back(tuple);
//        }
//        {
//            std::unique_lock<std::mutex> locker(networkAndJsonMutex_Query1);
//            dataBuffer_Query1.push_front(move(tupleBucket));
//            mergeNotifyFlag_Query1 = true;
//            mergeCond_Query1.notify_one();
////                mergeCond.notify_all();
//        }
//
//    }
//
//    gettimeofday( &endTime1, NULL );
//    taskTime+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));
//    return taskTime;
//}
//
//
///*
// That function is to parse the json from batch and output an array to vector.
// It is based on the rapidjson which is the fastest json library.
// */
//long mergeTask_Query1(int taskId){
//    cout << "entry merge task: " << taskId << endl;
//    struct timeval  startTime1, endTime1;
//    long taskTime = 0;
//    gettimeofday( &startTime1, NULL );
//
//    Init_dict init_dict;
//    Incremental_Detector incremental_Detector(init_dict);
//    vector<Tuple> tupleBucket;
//    while(true) {
//        tupleBucket.clear();
//            if(endFlag_Query1)
//                break;
//
//        {
//            std::unique_lock <std::mutex> locker(networkAndJsonMutex_Query1);
//            mergeCond_Query1.wait_for(locker, std::chrono::milliseconds(20), []{ return !dataBuffer_Query1.empty() || mergeNotifyFlag_Query1;} );
//            mergeNotifyFlag_Query1 = false;
////            taskTime++;
//            gettimeofday( &startTime1, NULL );
//            if (!dataBuffer_Query1.empty()) {
//                tupleBucket = move(dataBuffer_Query1.back());
//                dataBuffer_Query1.pop_back();
//            }
//        }
//
//        //that is for query2 to reorganise the data set
//        if(!tupleBucket.empty()) {
//            double voltage[1000];
//            double current[1000];
//            int temp = tupleBucket.back().sequenceNumber / 1000;
//            for (int i = 0; i < tupleBucket.size(); i++) {
//                    Tuple tuple = tupleBucket[i];
//                    voltage[i] = tuple.voltage;
//                    current[i] = tuple.current;
//            }
//            tupleBucket.clear();
//            Feature X_i = incremental_Detector.compute_input_signal(voltage, current, period_Query1, true);
//            X_i.sequenceNumber = temp;
////            cout<<featureBufer_Query1.size() <<"        "<< X_i.sequenceNumber << "  " << X_i.reactive_power_Q << " "  << X_i.active_power_P <<endl;
//            {
//                std::unique_lock<std::mutex> locker(findMutex_Query1);
//                featureBufer_Query1.push_back(move(X_i));
//                findNotifyFlag_Query1 = true;
//                findCond_Query1.notify_one();
//            }
//        }
//        gettimeofday( &endTime1, NULL );
//        taskTime+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));
//    }
//    return taskTime;
//}
//
//
//long findTask_Query1(int taskId){
//    cout << "entry find task: " << taskId << endl;
//    struct timeval  startTime1, endTime1;
//    long taskTime = 0;
//
//    gettimeofday( &startTime1, NULL );
//
//    Init_dict init_dict;
//    Incremental_Detector incremental_Detector(init_dict);
//    vector<Feature> vecFeature;
//
//    while(true){
////        int batchNumber = batchCounter.load();
//        int batchNumber = 0;
//        if(endFlag_Query1) {
//            break;
//        }
//
//        {
//            std::unique_lock <std::mutex> locker(findMutex_Query1);
//            findCond_Query1.wait_for(locker, std::chrono::milliseconds(20), []{ return !featureBufer_Query1.empty() || findNotifyFlag_Query1;} );
//            findNotifyFlag_Query1 = false;
////            taskTime++;
//            gettimeofday( &startTime1, NULL );
//            if (!featureBufer_Query1.empty()) {
////                cout<< featureBufer.size() << endl;
//                int queueSize = featureBufer_Query1.size();
//                for(int i = 0; i<queueSize; i++){
//                    Feature X_temp = move(featureBufer_Query1.back());
////                    cout<<featureBufer_Query1.size() <<"     "<< X_temp.sequenceNumber << "  " << X_temp.reactive_power_Q << " "  << X_temp.active_power_P <<endl;
//                    featureBufer_Query1.pop_back();
//                    vecFeature.push_back(X_temp);
//                }
//            }
////            featureBufer_Query1.clear();
//        }
//        if(vecFeature.empty())
//            continue;
//
//        vector<Result> resultBucket;
//        for(int ind = 0; ind < vecFeature.size(); ind++) {
//            batchNumber = vecFeature[ind].sequenceNumber;
////            cout<< vecFeature[ind].sequenceNumber << "  " << vecFeature[ind].reactive_power_Q << " "  << vecFeature[ind].active_power_P <<endl;
//            Event event_interval_indices = incremental_Detector.predict(batchNumber, vecFeature[ind]);
//
//            if (event_interval_indices.event_end != 0) {  // if an event is returned
//                cout << "batchNumber " <<  vecFeature[ind].sequenceNumber << "  Event Detected at   " << event_interval_indices.event_start << "," <<
//                     event_interval_indices.event_end << endl;
//
////                double mean_event_index = (event_interval_indices.event_start + event_interval_indices.event_end) / 2.0 + 1;
//                int mean_event_index = (event_interval_indices.event_start + event_interval_indices.event_end) / 2 + 1;
//
//                Result result_temp;
//                result_temp.s = vecFeature[ind].sequenceNumber;
//                result_temp.d = 1;
//                result_temp.event_s = mean_event_index;
//                resultBucket.push_back(result_temp);
//
//            } else {
//                Result result_temp;
//                result_temp.s = vecFeature[ind].sequenceNumber;
//                result_temp.d = 0;
//                result_temp.event_s = -1;
//                resultBucket.push_back(result_temp);
//                incremental_Detector.refresh_window(MAXIMUM_WINDOW_SIZE_Query1);
//            }
//
//            {
//                std::unique_lock<std::mutex> locker(sendMutex_Query1);
//                resultBuffer_Query1.push_front(move(resultBucket));
//                sendNotifyFlag_Query1 = true;
//                sendCond_Query1.notify_one();
//            }
//
//        }
//        vecFeature.clear();
//        gettimeofday( &endTime1, NULL );
//        taskTime+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));
//    }
//
//        return taskTime;
//}
//
//
//long sendTask_Query1(int taskId, string address){
//    cout << "entry send task: " << taskId << endl;
//    struct timeval  startTime1, endTime1;
//    long taskTime = 0;
//    gettimeofday( &startTime1, NULL );
//    vector<Result> resultBucket;
//
//    CURL *curl_post;
//    curl_post = curl_easy_init();
//    curl_easy_setopt(curl_post, CURLOPT_URL, address.c_str());
//
//    while(true) {
//        resultBucket.clear();
//        if(endFlag_Query1)
//            break;
//
//        {
//            std::unique_lock <std::mutex> locker(sendMutex_Query1);
////            mergeCond.wait_for(locker,std::chrono::milliseconds(20) );
//            sendCond_Query1.wait_for(locker, std::chrono::milliseconds(20), []{ return !resultBuffer_Query1.empty() || sendNotifyFlag_Query1;} );
//            sendNotifyFlag_Query1 = false;
////            taskTime++;
//            gettimeofday( &startTime1, NULL );
//            if (!resultBuffer_Query1.empty()) {
//                resultBucket = move(resultBuffer_Query1.back());
//                resultBuffer_Query1.pop_back();
//            }
////            resultBuffer_Query1.clear();
//        }
//        //that is for query2 to reorganise the data set
//        if(resultBucket.empty()) {
//            continue;
//        }
//        if(!resultBucket.empty()) {
//
//            for(int i = 0; i< resultBucket.size(); i++){
////        cout<<resultBucket[i].s << "  " << resultBucket[i].d << "  " << resultBucket[i].event_s << endl;
////        rapidjson::StringBuffer buf;
////        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
////        writer.StartObject();
////        writer.Key("s"); writer.Int(1000);
////        writer.Key("d"); writer.Bool(false);
////        writer.EndObject();
////        const char* json_content = buf.GetString();
//
//                std::string jsonTemp =   "{\"s\":";
//                jsonTemp.append(to_string (resultBucket[i].s));
//                if(resultBucket[i].d == 0){
//                    jsonTemp.append(",\"d\":0,\"event_s\":");
//                }else{
//                    jsonTemp.append(",\"d\":1,\"event_s\":");
//                }
//                if(resultBucket[i].event_s == -1){
//                    jsonTemp.append(to_string(-1));
//                }else {
//                    jsonTemp.append(to_string(resultBucket[i].event_s));
//                }
//                jsonTemp.append("}");
////                jsonTemp = "{\"s\":317,\"d\":1,\"event_s\":314.5 }";
//
////                cout << jsonTemp.c_str() << endl;
////
//                std::string response_string_post;
//                struct curl_slist *headers = NULL;
//                headers = curl_slist_append(headers, "Content-Type: application/json");
//                curl_easy_setopt(curl_post, CURLOPT_HTTPHEADER, headers);
//                curl_easy_setopt(curl_post, CURLOPT_POSTFIELDS, jsonTemp.c_str());
//                curl_easy_setopt(curl_post, CURLOPT_WRITEFUNCTION, writeFunction_Parallel);
//                curl_easy_setopt(curl_post, CURLOPT_WRITEDATA, &response_string_post);
//                curl_easy_perform(curl_post);
////                CURLcode res =  curl_easy_perform(curl_post);
////                fprintf(stderr, "curl_easy_perform() failed: %s\n",
////                        curl_easy_strerror(res));
//            }
//        }
//        gettimeofday( &endTime1, NULL );
//        taskTime+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));
//    }
//
//
//    return taskTime;
//}
//
//
//Query1_Parallel::Query1_Parallel(string address) {
//
//    cout << "Query 1 starting" << endl;
//
//    //for benchmark
////    struct timeval  startTime1, endTime1;//, entirStartTime, entirEndTime;
////    struct timeval  startTime2, endTime2;//, entirStartTime, entirEndTime;
////    long  prestepTime=0, step1Time=0, step2Time=0;
////    long  initialization=0, networkTime=0, jsonTime=0;
//
//    //if to wait for the late data or the miss data
//    int parallelNetwork = 1;
//    int parallelSending = 1;
//    int parallelJsonParse = 1;
//
//
//    cout << "Getting data in batches.." << endl;
//
//    vector<future<long>> taskTime;
//    //    std::string header_string;
//    for(int i = 0; i < parallelNetwork; i++){
//        taskTime.push_back(async(launch::async, networkTask_Query1, i+1, address));
//    }
//    taskTime.push_back(async(launch::async, mergeTask_Query1, parallelNetwork + 1));
//    taskTime.push_back(async(launch::async, findTask_Query1, parallelNetwork + 2));
//    for(int i = 0; i < parallelSending; i++){
//        taskTime.push_back(async(launch::async, sendTask_Query1, parallelNetwork + 2 + i, address));
//    }
//    for(int i = 0; i < ( parallelNetwork + parallelSending + 2); i++){
//        cout << taskTime[i].get() << '\n';
//    }
//
////    for(int i = 0; i < queueSize; i++){
////        cout << inputBatch[i].size() << endl;
////    }
//
////    curl_easy_cleanup(curl_get);
////    curl_easy_cleanup(curl_post);
//    cout << "> Query 1 done!" << endl;
//
//}