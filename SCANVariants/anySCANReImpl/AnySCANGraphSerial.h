//
// Created by yche on 10/3/17.
//

#ifndef PPSCAN_ANYSCANGRAPH_H
#define PPSCAN_ANYSCANGRAPH_H

#include <memory>

#define STATISTICS

#include "InputOutput.h"
#include "DisjointSet.h"

class AnySCANGraph {
private:
    unique_ptr<InputOutput> io_helper_ptr;
    // parameter1: e.g eps: 0.13, eps_a:13, eps_b:100;
    // parameter2: min_u: 5, 5 nearest neighbor as threshold
    int eps_a2, eps_b2, min_u;
    int alpha_block_size, beta_block_size;

    // compressed spare row graph
    ui n;
    vector<ui> out_edge_start;
    vector<int> out_edges;

    // vertex property
    vector<int> degree;
    vector<char> vertex_status;
    vector<int> eps_neighbor_num;

    // edge property
    vector<int> min_cn;

    // clusters: core and non-core(hubs)
    vector<int> cluster_dict;


    // disjoint-sets represent core-induced clusters, eps_neighborhood has one-hop information
    unique_ptr<DisjointSet> disjoint_set_ptr;
    vector<vector<int>> eps_neighborhood;
    vector<vector<int>> candidate_super_nodes;

    // first: cluster id(min core-vertex id in cluster), second: non-core vertex id
    vector<pair<int, int>> noncore_cluster;

    // used for computation data initialization
    vector<int> checking_lst;

    // statistics
#ifdef STATISTICS
    long intersection_times = 0;
#endif
private:
    int ComputeCnLowerBound(int du, int dv);

    int IntersectNeighborSets(int u, int v, int min_cn_num);

    int EvalSimilarity(int u, ui edge_idx);

    ui BinarySearch(vector<int> &array, ui offset_beg, ui offset_end, int val);

private:
    void Summarize();

    void MergeStronglyRelatedCluster();

    void MergeWeaklyRelatedCluster();

    void MarkClusterMinEleAsId();

    void ClusterNonCore();

    void PostProcessing();

public:
    explicit AnySCANGraph(const char *dir_string, const char *eps_s, int min_u);

    void anySCAN();

    void Output(const char *eps_s, const char *miu);
};


#endif //PPSCAN_ANYSCANGRAPH_H
