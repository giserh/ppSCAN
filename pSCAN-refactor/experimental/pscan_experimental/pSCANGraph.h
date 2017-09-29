#ifndef _GRAPH_H_
#define _GRAPH_H_

#include "Utility.h"

using namespace std;

class pSCANGraph {
private:
    string dir;
    ui n, m;

    int eps_a2, eps_b2, miu; // eps_a2/eps_b2 = eps^2

    ui *pstart; //offset of neighbors of nodes
    int *edges; //adjacent ids of edges

    ui *reverse_link; //the position of reverse edge in edges
    int *min_cn; //minimum common neighbor: -2 means not similar; -1 means similar; 0 means not sure; > 0 means the minimum common neighbor

    int *pa;
    int *rank; //pa and rank are used for the disjoint-set data structure

    int *cid; //cluster id

    int *degree;
    int *similar_degree; //number of adjacent edges with similarity no less than epsilon
    int *effective_degree; //number of adjacent edges not pruned by similarity

    vector<pair<int, int> > noncore_cluster;

    // statistics
#ifdef STATISTICS
    long all_cmp0 = 0;
    long all_cmp1 = 0;
    long all_cmp2 = 0;
    long intersection_times = 0;
    int portion = 0;
#endif
public:
    explicit pSCANGraph(const char *_dir);

    ~pSCANGraph();

    void read_graph();

    void pSCAN(const char *eps_s, int miu);

    //eps_s and miu are the parameters (epsilon, miu) for the SCAN algorithm
    void cluster_noncore_vertices(int eps_a2, int eps_b2, int mu);

    void output(const char *eps_s, const char *miu);

private:
    ui binary_search(const int *array, ui b, ui e, int val);

    int similar_check_OP(int u, ui idx, int eps_a, int eps_b);

    int check_common_neighbor(int u, int v, int c);

    int compute_common_neighbor_lowerbound(int u, int v, int eps_a2, int eps_b2);

    void prune_and_cross_link(int eps_a2, int eps_b2, int miu, int *cores, int &cores_e);

    int find_root(int u);

    void my_union(int u, int v);

    void get_eps(const char *eps_s);
};

#endif
