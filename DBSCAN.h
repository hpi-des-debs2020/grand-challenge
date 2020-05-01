//
// Created by Administrator on 2/6/2020.
//
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <map>

#ifndef GRAND_CHALLENGE_DBSCAN_H
#define GRAND_CHALLENGE_DBSCAN_H

struct Feature
{
    double active_power_P;
    double reactive_power_Q;
    int sequenceNumber;
};

using namespace std;
const int NOISE = 0;
const int NOT_CLASSIFIED = -1;


class DBCAN {
public:
    int minPts;
    double eps;
    vector<Feature> points;
    int size;
    vector<vector<int> > adjPoints;
    vector<vector<int> > cluster;
    int clusterIdx;
    int *ptsCnt;
    int *clusterNum;

    DBCAN(double eps, int minPts, vector<Feature> points);
    void run ();
    void dfs (int now, int c);
    void checkNearPoints();
    // is idx'th point core object?
    bool isCoreObject(int idx);
    vector<vector<int> > getCluster();
    double getDis(Feature a, Feature b);
};

#endif //GRAND_CHALLENGE_DBSCAN_H
