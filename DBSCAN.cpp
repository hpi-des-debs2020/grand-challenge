//
// Created by Administrator on 2/6/2020.
//

#include "DBSCAN.h"
#include "Event_Detector.h"

using namespace std;

DBCAN::DBCAN(double eps, int minPts, vector<Feature> points) {
    this->eps = eps;
    this->minPts = minPts;
    this->points = points;
    this->size = (int)points.size();
    adjPoints.resize(size);
    this->clusterIdx=0;
    this->ptsCnt = new int[points.size()];
    this->clusterNum = new int[points.size()];
    for (int i = 0; i < points.size(); i++){
        ptsCnt[i] = 1;
        clusterNum[i] = NOT_CLASSIFIED;
    }
}
void DBCAN::run () {
    checkNearPoints();

    for(int i=0;i<size;i++) {
        if(clusterNum[i] != NOT_CLASSIFIED) continue; //if it is not classified go dfs

        if(isCoreObject(i)) {
            dfs(i, ++clusterIdx);
        } else {
            clusterNum[i] = NOISE;
        }
    }

    cluster.resize(clusterIdx+1);
    for(int i=0;i<size;i++) {
//        if(clusterNum[i] != NOISE) {
            cluster[clusterNum[i]].push_back(i);
//        }
    }
}

//MERGE!!!!!!!!!!!!!!!!!!
void DBCAN::dfs (int now, int c) {
    clusterNum[now] = c;
    if(!isCoreObject(now)) return; //if it is not core point then return

    for(auto&next:adjPoints[now]) {
        if(clusterNum[next] != NOT_CLASSIFIED) continue;
        dfs(next, c);
    }
}

void DBCAN::checkNearPoints() {
    for(int i=0;i<size;i++) {
        for(int j=0;j<size;j++) {
            if(i==j) continue;
//            double tem = getDis(points[i], points[j]);
//            double tem2 = eps;
//            double tem3 = points[i].active_power_P;
//            double tem4 = points[j].active_power_P;
//            double tem5 = (points[i].active_power_P - points[j].active_power_P)*(points[i].active_power_P - points[j].active_power_P);
//            double tem6 = points[i].reactive_power_Q;
//            double tem7 = points[j].reactive_power_Q;
//            double tem8 = (points[i].reactive_power_Q - points[j].reactive_power_Q)*(points[i].reactive_power_Q - points[j].reactive_power_Q);
//            double tem9 = sqrt(tem5+tem8);
            if(getDis(points[i], points[j]) <= eps) {
                ptsCnt[i]++;
                adjPoints[i].push_back(j);
            }
        }
    }
}
// is idx'th point core object?
bool DBCAN::isCoreObject(int idx) {
    return ptsCnt[idx] >= minPts;
}

vector<vector<int> > DBCAN::getCluster() {
    return cluster;
}

double DBCAN::getDis(Feature a, Feature b) {

    return sqrt((a.active_power_P-b.active_power_P)*(a.active_power_P-b.active_power_P)+
                (a.reactive_power_Q-b.reactive_power_Q)*(a.reactive_power_Q-b.reactive_power_Q));
}