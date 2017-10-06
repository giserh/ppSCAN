//
// Created by yche on 10/3/17.
//

#include "AnySCANGraphSerial.h"

#include <cmath>
#include <chrono>
#include <queue>

using namespace std;
using namespace chrono;
using namespace yche;

AnySCANGraph::AnySCANGraph(const char *dir_string, const char *eps_s, int min_u) {
    io_helper_ptr = yche::make_unique<InputOutput>(dir_string);
    io_helper_ptr->ReadGraph();

    auto tmp_start = high_resolution_clock::now();

    // 1st: parameter
    std::tie(eps_a2, eps_b2) = io_helper_ptr->ParseEps(eps_s);
    this->min_u = min_u;

    // 2nd: graph
    // csr representation
    n = static_cast<ui>(io_helper_ptr->n);
    out_edge_start = std::move(io_helper_ptr->offset_out_edges);
    out_edges = std::move(io_helper_ptr->out_edges);

    // vertex properties
    degree = std::move(io_helper_ptr->degree);
    vertex_status = vector<char>(n, anySCAN::UN_TOUCHED);
    eps_neighbor_num = vector<int>(n, 0);
    eps_neighborhood = vector<vector<int>>(n);

    disjoint_set_ptr = yche::make_unique<DisjointSet>(n);

    alpha_block_size = 32768;
    beta_block_size = 32768;
    checking_lst.reserve(n);
    auto all_end = high_resolution_clock::now();
    cout << "other construct time:" << duration_cast<milliseconds>(all_end - tmp_start).count()
         << " ms\n";
}

int AnySCANGraph::ComputeCnLowerBound(int du, int dv) {
    auto c = (int) (sqrtl((((long double) du) * ((long double) dv) * eps_a2) / eps_b2));
    if (((long long) c) * ((long long) c) * eps_b2 < ((long long) du) * ((long long) dv) * eps_a2) { ++c; }
    return c;
}

int AnySCANGraph::IntersectNeighborSets(int u, int v, int min_cn_num) {
    int cn = 2; // count for self and v, count for self and u
    int du = out_edge_start[u + 1] - out_edge_start[u] + 2, dv =
            out_edge_start[v + 1] - out_edge_start[v] + 2; // count for self and v, count for self and u

    // correctness guaranteed by two pruning previously in computing min_cn
    for (auto offset_nei_u = out_edge_start[u], offset_nei_v = out_edge_start[v];
         offset_nei_u < out_edge_start[u + 1] && offset_nei_v < out_edge_start[v + 1] &&
         cn < min_cn_num && du >= min_cn_num && dv >= min_cn_num;) {
        if (out_edges[offset_nei_u] < out_edges[offset_nei_v]) {
            --du;
            ++offset_nei_u;
        } else if (out_edges[offset_nei_u] > out_edges[offset_nei_v]) {
            --dv;
            ++offset_nei_v;
        } else {
            ++cn;
            ++offset_nei_u;
            ++offset_nei_v;
        }
    }
    return cn >= min_cn_num ? SIMILAR : NOT_SIMILAR;
}

int AnySCANGraph::EvalSimilarity(int u, ui edge_idx) {
    int v = out_edges[edge_idx];
    int deg_a = degree[u], deg_b = degree[v];
    if (deg_a > deg_b) { swap(deg_a, deg_b); }
    if (((long long) deg_a) * eps_b2 < ((long long) deg_b) * eps_a2) {
        return NOT_SIMILAR;
    } else {
        int c = ComputeCnLowerBound(deg_a, deg_b);
        return c <= 2 ? SIMILAR : IntersectNeighborSets(u, v, c);
    }
}

bool AnySCANGraph::IsDefiniteCore(int u) {
    return u == anySCAN::UNPROCESSED_CORE || u == anySCAN::PROCESSED_CORE;
}

// get complete core-induced clusters, but requiring further merging. further expansion obeys to connectivity
void AnySCANGraph::Summarize() {
    for (auto v_beg = 0, v_cur = 0, untouched_num = 0; v_cur < n; v_cur++) {
        if (vertex_status[v_cur] == untouched_num) {
            untouched_num++;
        }
        if (untouched_num >= alpha_block_size || v_cur == n - 1) {
            // 1.1 examining all untouched vertices
            for (auto u = v_beg; u < v_cur + 1; u++) {
                // compute eps-neighborhood for u
                auto sd = 0;
                for (auto j = out_edge_start[u]; j < out_edge_start[u + 1]; j++) {
                    auto &eps_neighborhood_vec = eps_neighborhood[u];
                    if (EvalSimilarity(u, j) == SIMILAR) {
                        eps_neighborhood_vec.emplace_back(out_edges[j]);
                        sd++;
                    }
                }
                // check core status from sd
                if (sd >= min_u) {
                    vertex_status[u] = anySCAN::PROCESSED_CORE; // definite core, eps-neighborhood known
                } else {
                    vertex_status[u] = anySCAN::PROCESSED_NOISE; // definite non-core, eps-neighborhood known
                }
            }

            // 1.2 update eps-neighborhood property, update vertex_status
            for (auto u = v_beg; u < v_cur + 1; u++) {
                if (vertex_status[u] == anySCAN::PROCESSED_CORE) {
                    for (auto w: eps_neighborhood[u]) {
                        eps_neighbor_num[w]++;
                    }
                }
            }
            for (auto u = v_beg; u < v_cur + 1; u++) {
                if (vertex_status[u] == anySCAN::PROCESSED_CORE) {
                    for (auto w: eps_neighborhood[u]) {
                        if (vertex_status[w] == anySCAN::PROCESSED_NOISE) {
                            vertex_status[w] = anySCAN::PROCESSED_BORDER;
                        } else if (vertex_status[w] != anySCAN::PROCESSED_CORE) {

                        } else if (vertex_status[w] == anySCAN::UN_TOUCHED) {
                            if (eps_neighbor_num[w] >= min_u) {
                                vertex_status[w] = anySCAN::UNPROCESSED_CORE;
                            }
                        }
                    }
                }
            }

            // 1.3 merge core-induced clusters related to vertices in the current block
            for (auto u = v_beg; u < v_cur + 1; u++) {
                // u is either anySCAN::PROCESSED_CORE or PROCESSED_NOISE
                if (vertex_status[u] == anySCAN::PROCESSED_CORE) {
                    for (auto w: eps_neighborhood[u]) {
                        if (IsDefiniteCore(w)) {
                            disjoint_set_ptr->Union(u, w);
                        }
                    }
                }
            }

            v_beg = v_cur + 1;
        }
    }
}

void AnySCANGraph::MergeStronglyRelatedCluster() {

}

void AnySCANGraph::MergeWeaklyRelatedCluster() {
    checking_lst.clear();
}

// eval similarity for unprocessed core/non-core, to produce complete results for non-cores, i.e, assign labels
void AnySCANGraph::PostProcessing() {

}

void AnySCANGraph::anySCAN() {
    Summarize();
    MergeStronglyRelatedCluster();
    MergeWeaklyRelatedCluster();
    PostProcessing();
}

void AnySCANGraph::Output(const char *eps_s, const char *miu) {
//    io_helper_ptr->Output(eps_s, miu, noncore_cluster, is_core_lst, cluster_dict, *disjoint_set_ptr);
}