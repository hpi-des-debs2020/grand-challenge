////
//// Created by Administrator on 2/20/2020.
////
//#include "query2_Parallel.h"
//#include "Incremental_Detector_Disorder.h"
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
//#include <condition_variable>
//#include "rapidjson/document.h"
//#include "rapidjson/writer.h"
//#include "rapidjson/stringbuffer.h"
//#include <curl/curl.h>
//#include <unistd.h>
//#include <list>
//
//using namespace rapidjson;
//using namespace std;
//
//size_t writeFunction_Parallel2(void *ptr, size_t size, size_t nmemb, std::string* data) {
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
//int NETWORK_FREQUENCY_Query2 = 50;
//int SAMPLERATE_Query2 = 50000;  //Sampling Rate the raw Dataset
////Compute the period size of the dataset: i.e. number of raw data points per period
//int period_Query2 = SAMPLERATE_Query2 / NETWORK_FREQUENCY_Query2;
//int MAXIMUM_WINDOW_SIZE_Query2 = 100;
//
////to predict the event
//int Radius_Query2 =7;
////the queue to store input tuple
////the size of queue
//const int queueSize_Query2 = 30;
////maximum size of buffer
//int maxThrehold_Query2 = 1000;
////maximum waiting time for late tuples
//int lateTime_Query2 = 20;
//
////the buffer for network batch queue which can avoid resource competition of inputBatch.
//list<vector<Tuple>> dataBuffer_Query2;
////reorder the data
//list<Feature> featureBufer_Query2;
////share the data between sent thread and find thread
//list<vector<Result>> resultBuffer_Query2;
//
//// the lock between fetch thread and merge thread
//mutex networkAndJsonMutex_Query2;
////for condition variable to wake up the merge thread
//condition_variable mergeCond_Query2;
//// the lock between find thread and merge thread
//mutex findMutex_Query2;
////for condition variable to wake up the merge thread
//condition_variable findCond_Query2;
//// the lock between find thread and merge thread
//mutex sendMutex_Query2;
////for condition variable to wake up the merge thread
//condition_variable sendCond_Query2;
////flag notify all threads to stop
//bool endFlag_Query2 = false;
//bool mergeNotifyFlag_Query2 = false;
//bool findNotifyFlag_Query2 = false;
//bool sendNotifyFlag_Query2 = false;
//
//
///*
//The program would create a thread based on that function to fetch data from the http sever which support asynchronization .
//And it will put the input batch that include 1000 tuples into the deque.
//*/
//long  networkTask_Query2(int taskId, string address){
//    cout << "entry network task: " << taskId << endl;
//    struct timeval  startTime1, endTime1;
//    long taskTime = 0;
//    gettimeofday( &startTime1, NULL );
//
//    CURL *curl_get;
//    curl_get = curl_easy_init();
//    curl_easy_setopt(curl_get, CURLOPT_URL, address.c_str());
//
//    while(true){
//        //get the data
//        std::string response_string;
//        curl_easy_setopt(curl_get, CURLOPT_WRITEFUNCTION, writeFunction_Parallel2);
//        curl_easy_setopt(curl_get, CURLOPT_WRITEDATA, &response_string);
//        curl_easy_perform(curl_get);
//
//        if(response_string.empty()){
//            usleep(1000000);
//            gettimeofday( &startTime1, NULL );
//            continue;
//        }
//
////        cout<< response_string.size() << endl;
//        //the size of last one is 16 the size of error is 37
//        if(response_string.size() == 16) {
//            cout<< response_string << endl;
//            continue;
//        }
//
//        Document document;
//        document.Parse(response_string.c_str());
//        Value::ConstMemberIterator itr = document.FindMember("records");
//        if (itr == document.MemberEnd() || endFlag_Query2) {
////            cout << endFlag << endl;
//            cout <<taskId<<"   "<< response_string << endl;
//            usleep(1000000);
//            endFlag_Query2 = true;
//            break;
//        }
////        batchCounter.fetch_add(1);
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
//            std::unique_lock<std::mutex> locker(networkAndJsonMutex_Query2);
//            dataBuffer_Query2.push_front(move(tupleBucket));
//            mergeNotifyFlag_Query2 = true;
//            mergeCond_Query2.notify_one();
////                mergeCond.notify_all();
//        }
//    }
//    gettimeofday( &endTime1, NULL );
//    taskTime+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));
//    return taskTime;
//}
//
///*
// That function is to parse the json from batch and output an array to vector.
// It is based on the rapidjson which is the fastest json library.
// */
//long mergeTask_Query2(int taskId){
//    cout << "entry merge task: " << taskId << endl;
//    struct timeval  startTime1, endTime1;
//    long taskTime = 0;
//    Init_dict init_dict;
//    Incremental_Detector_Disorder incremental_Detector_Disorder(init_dict);
//    vector<Tuple> inputBatch_Query2[queueSize_Query2];
//
//    gettimeofday( &startTime1, NULL );
//    vector<Tuple> tupleBucket;
//    while(true) {
//        tupleBucket.clear();
////        int batchNumber = batchCounter.load();
//        if(endFlag_Query2)
//            break;
//
//        {
//            std::unique_lock <std::mutex> locker(networkAndJsonMutex_Query2);
////            mergeCond.wait_for(locker,std::chrono::milliseconds(20) );
//            mergeCond_Query2.wait_for(locker, std::chrono::milliseconds(20), []{ return !dataBuffer_Query2.empty() || mergeNotifyFlag_Query2;} );
//            mergeNotifyFlag_Query2 = false;
////            taskTime++;
//            gettimeofday( &startTime1, NULL );
//            if (!dataBuffer_Query2.empty()) {
//                tupleBucket = move(dataBuffer_Query2.back());
//                dataBuffer_Query2.pop_back();
//            }
//        }
//        //that is for query2 to reorganise the data set
//        if(tupleBucket.empty()) {
//            continue;
//        }
//
//        if(!tupleBucket.empty()) {
//            int batchNumber = tupleBucket[0].sequenceNumber / 1000;
//            for (int i = 0; i < tupleBucket.size(); i++) {
//                Tuple tupleTemp;
//                tupleTemp.sequenceNumber = tupleBucket[i].sequenceNumber;
//                tupleTemp.current = tupleBucket[i].current;
//                tupleTemp.voltage = tupleBucket[i].voltage;
//
//                // -------------------- For TEST--------------------
////                if(tupleBucket[i].sequenceNumber/1000 ==  tupleBucket[0].sequenceNumber / 1000)
//                inputBatch_Query2[(tupleBucket[i].sequenceNumber / 1000) % queueSize_Query2].push_back(tupleTemp);
//            }
//            tupleBucket.clear();
////        cout << tupleBucket.size() <<endl;
////        for(int i = 0; i < queueSize; i++){
////            cout << i << " :  " <<inputBatch[i].size() << "   ";
////        }
////        cout<<endl;
//            for (int i = 0; i < queueSize_Query2; i++) {
//                //            the queue is not empty
//                if (!inputBatch_Query2[i].empty()) {
////                    cout <<inputBatch[i].size() << "   " ;
//                    int batchSequence = inputBatch_Query2[i].back().sequenceNumber / 1000;
//                    //when the queue size more than 900 or we fetch 20 batches, we start to merge
//                    //data and compute the reactive power and active power
//                    // for query2 to filter data queue
//
//                    //-------------------------For TEST------------------------------------------------------
//                    if (inputBatch_Query2[i].size() == maxThrehold_Query2 || (batchNumber - batchSequence ) >= lateTime_Query2) {
////                    if (inputBatch_Query2[i].size() != 0) {
//                        double voltage[1000];
//                        double current[1000];
//                        //initialization
//                        for (int j = 0; j < 1000; j++) {
//                            voltage[j] = 2;
//                            current[j] = 2;
//                        }
//                        int tempSize = inputBatch_Query2[i].size();
//                        for (int j = 0; j < tempSize; j++) {
//                            Tuple tuple = inputBatch_Query2[i][j];
////                                cout<< tuple.sequenceNumber << "   " << tuple.voltage <<"   " << tuple.current <<endl;
////                                inputBatch_Query2[i].pop_back();
//                            //                        sequenceNumber[i] = tuple.sequenceNumber;
//                            voltage[j] = tuple.voltage;
//                            current[j] = tuple.current;
//                        }
//                        inputBatch_Query2[i].clear();
//                        Feature X_i = incremental_Detector_Disorder.compute_input_signal(voltage, current, period_Query2,
//                                                                                         true);
//                        X_i.sequenceNumber = batchSequence;
////                            if(batchNumber - batchSequence  < lateTime) {
//                        if(tempSize > 900) {
//                            X_i.badPoint = -1;
//                        }
//                        else{
//                            X_i.badPoint = 1;
//                        }
////                        cout<< X_i.sequenceNumber << "  " << X_i.active_power_P << "  " << X_i.reactive_power_Q << "   " << X_i.badPoint << endl;
////                        cout<< featureBufer_Query2.size() << endl;
//                        {
//                            std::unique_lock<std::mutex> locker(findMutex_Query2);
//                            featureBufer_Query2.push_front(move(X_i));
//                            findNotifyFlag_Query2 = true;
//                            findCond_Query2.notify_one();
//                        }
//                    }
//                }
//            }
//        }
//        gettimeofday( &endTime1, NULL );
//        taskTime+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));
//    }
//    return taskTime;
//}
//
////the findtask thread is to find event form dataset
//long findTask_Query2(int taskId){
//    cout << "entry find task: " << taskId << endl;
//    struct timeval  startTime1, endTime1;
//    Init_dict init_dict;
//    Incremental_Detector_Disorder incremental_Detector_Disorder(init_dict);
//    long taskTime = 0;
//    list<int> waitBatch;
//
////    map<int, Feature> mapFeature;
//    vector<Feature> vecFeature;
//    gettimeofday( &startTime1, NULL );
//
//
//    while(true){
//        int batchNumber = 0;
//        if(endFlag_Query2) {
//            break;
//        }
//        {
//            std::unique_lock <std::mutex> locker(findMutex_Query2);
//            findCond_Query2.wait_for(locker, std::chrono::milliseconds(20), []{ return !featureBufer_Query2.empty() || findNotifyFlag_Query2;} );
//            findNotifyFlag_Query2 = false;
////            taskTime++;
//            gettimeofday( &startTime1, NULL );
//            if (!featureBufer_Query2.empty()) {
////                cout<< featureBufer.size() << endl;
//                int queueSize = featureBufer_Query2.size();
//                for(int i = 0; i<queueSize; i++){
//                    Feature X_temp = move(featureBufer_Query2.back());
//                    featureBufer_Query2.pop_back();
////                    cout<< featureBufer_Query2.size() << "  " << X_temp.badPoint << "  " << X_temp.sequenceNumber <<"  " << X_temp.active_power_P <<"  " << X_temp.reactive_power_Q <<endl;
//                    vecFeature.push_back(X_temp);
////                    mapFeature.insert(pair<int, Feature>(X_temp.sequenceNumber, X_temp));
//                }
////                featureBufer.clear();
//            }
//        }
//        if(vecFeature.empty())
//            continue;
//
//        vector<Result> resultBucket;
//        for(int ind = 0; ind < vecFeature.size(); ind++) {
//            batchNumber = vecFeature[ind].sequenceNumber;
//            //because the predict would refresh the window of predictEvent, put the predictevent before predict
//            int eventPredict = incremental_Detector_Disorder.predictEvent(vecFeature[ind],Radius_Query2);
//            vector<Event> event_interval_indices_vector = incremental_Detector_Disorder.predict(batchNumber, vecFeature[ind]);
//            //maaybe there is an event
//            if(eventPredict == 1){
//                waitBatch.push_back(vecFeature[ind].sequenceNumber);
////                cout<<vecFeature[ind].sequenceNumber<<endl;
//            }
//                //surely it is not an event
//            else{
//                Result result_temp;
//                result_temp.s = vecFeature[ind].sequenceNumber;
//                result_temp.d = 0;
//                result_temp.event_s = -1;
//                resultBucket.push_back(result_temp);
////                cout<<"1:   "<<result_temp.s << "  " << result_temp.d << "  " << result_temp.event_s << endl;
//            }
//            waitBatch.sort();
//
//            if(!event_interval_indices_vector.empty()) {
//                //the event_interval_indices_vector is ordered since all the events are pushed back
//                for (int eventId = 0; eventId < event_interval_indices_vector.size(); eventId++) {
//                    Event event_interval_indices = event_interval_indices_vector[eventId];
//                    if (event_interval_indices.event_end != 0) {  // if an event is returned
//                        cout << "batchNumber" <<  vecFeature[ind].sequenceNumber << "  Event Detected at   " << event_interval_indices.event_start << "," <<
//                             event_interval_indices.event_end << endl;
//
////                        double mean_event_index = (event_interval_indices.event_start + event_interval_indices.event_end) / 2.0 + 1;
//                        int mean_event_index = (event_interval_indices.event_start + event_interval_indices.event_end) / 2 + 1;
//
//                        //send the result with event
//                        Result result_temp;
//                        result_temp.s = event_interval_indices.sequenceNumber;
//                        result_temp.d = 1;
//                        result_temp.event_s = mean_event_index;
//                        resultBucket.push_back(result_temp);
////                        cout<<"2:   "<<result_temp.s << "  " << result_temp.d << "  " << result_temp.event_s << endl;
//                        waitBatch.remove(event_interval_indices.sequenceNumber);
//
//                    }
//                }
//            }else{
//                incremental_Detector_Disorder.refresh_window(MAXIMUM_WINDOW_SIZE_Query2);
//            }
//
//            //clear the waitBatch
//            int tempSize = waitBatch.size();
//            for(int i = 0; i< tempSize; i++){
//                if(waitBatch.front() <= incremental_Detector_Disorder.mapFeatureBlockedPos){
//                    Result result_temp;
//                    result_temp.s = waitBatch.front();
//                    result_temp.d = 0;
//                    result_temp.event_s = -1;
//                    resultBucket.push_back(result_temp);
//                    waitBatch.pop_front();
////                    cout<<"3:   "<<result_temp.s << "  " << result_temp.d << "  " << result_temp.event_s << endl;
//                }
//            }
//            event_interval_indices_vector.clear();
//            //send the result
//            {
//                std::unique_lock<std::mutex> locker(sendMutex_Query2);
//                resultBuffer_Query2.push_front(move(resultBucket));
//                sendNotifyFlag_Query2 = true;
//                sendCond_Query2.notify_one();
//            }
//        }
//        vecFeature.clear();
//        gettimeofday( &endTime1, NULL );
//        taskTime+= (1000000*(endTime1.tv_sec - startTime1.tv_sec) + (endTime1.tv_usec - startTime1.tv_usec));
//    }
//    return taskTime;
//}
//
//
///*
//That threads is to send result to server
// */
//long sendTask_Query2(int taskId, string address){
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
//        if(endFlag_Query2)
//            break;
//
//        {
//            std::unique_lock <std::mutex> locker(sendMutex_Query2);
////            mergeCond.wait_for(locker,std::chrono::milliseconds(20) );
//            sendCond_Query2.wait_for(locker, std::chrono::milliseconds(20), []{ return !resultBuffer_Query2.empty() || sendNotifyFlag_Query2;} );
//            sendNotifyFlag_Query2 = false;
////            taskTime++;
//            gettimeofday( &startTime1, NULL );
//            if (!resultBuffer_Query2.empty()) {
//                resultBucket = move(resultBuffer_Query2.back());
//                resultBuffer_Query2.pop_back();
//            }
//        }
//        //that is for query2 to reorganise the data set
//        if(resultBucket.empty()) {
//            continue;
//        }
//        if(!resultBucket.empty()) {
//
//
//            for(int i = 0; i< resultBucket.size(); i++){
//
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
//                    jsonTemp.append(to_string((int) resultBucket[i].event_s));
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
//                curl_easy_setopt(curl_post, CURLOPT_WRITEFUNCTION, writeFunction_Parallel2);
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
//    return taskTime;
//}
//
//Query2_Parallel::Query2_Parallel(string address) {
//    cout << "Query 2 starting" << endl;
//
//    int parallelNetwork = 1;
//    int parallelSending = 1;
//    int parallelJsonParse = 1;
//
////    EventDet_Barsim.fit();
//    int MAXIMUM_WINDOW_SIZE = 100;  //2 seconds
//
//
//    cout << "Getting data in batches.." << endl;
//
//    vector<future<long>> taskTime;
//    //    std::string header_string;
//    for(int i = 0; i < parallelNetwork; i++){
//        taskTime.push_back(async(launch::async, networkTask_Query2, i+1, address));
//    }
//    taskTime.push_back(async(launch::async, mergeTask_Query2, parallelNetwork + 1));
//    taskTime.push_back(async(launch::async, findTask_Query2, parallelNetwork + 2));
//    for(int i = 0; i < parallelSending; i++){
//        taskTime.push_back(async(launch::async, sendTask_Query2, parallelNetwork + 2 + i, address));
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
//    cout << "> Query 2 done!" << endl;
//
//}