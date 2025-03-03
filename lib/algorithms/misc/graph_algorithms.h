/******************************************************************************
 * graph_algorithms.h
 *
 * Source of VieCut
 *
 ******************************************************************************
 * Copyright (C) 2018 Alexander Noe <alexander.noe@univie.ac.at>
 *
 * Published under the MIT license in the LICENSE file.
 *****************************************************************************/

#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common/definitions.h"
#include "data_structure/graph_access.h"
#include "data_structure/mutable_graph.h"
#pragma once

class graph_algorithms {
 public:
    static std::vector<NodeID> top_k_degrees(
        std::shared_ptr<graph_access> G, size_t k) {
        std::vector<std::pair<NodeID, EdgeWeight> > all_degrees;
        for (NodeID n : G->nodes()) {
            all_degrees.emplace_back(n, G->getNodeDegree(n));
        }

        return find_top_k(all_degrees, k);
    }

    static std::vector<NodeID> top_k_degrees(
        std::shared_ptr<mutable_graph> G, size_t k) {
        std::vector<std::pair<NodeID, EdgeWeight> > all_degrees;
        for (NodeID n : G->nodes()) {
            all_degrees.emplace_back(n, G->getWeightedNodeDegree(n));
        }

        return find_top_k(all_degrees, k);
    }

    static std::vector<NodeID> weighted_top_k_degrees(
        std::shared_ptr<graph_access> G, size_t k) {
        std::vector<std::pair<NodeID, EdgeWeight> > all_degrees;
        for (NodeID n : G->nodes()) {
            all_degrees.emplace_back(n, G->getWeightedNodeDegree(n));
        }

        return find_top_k(all_degrees, k);
    }

    static void checkGraphValidity(std::shared_ptr<mutable_graph> G) {
        size_t edges = 0;
        for (NodeID n : G->nodes()) {
            for (NodeID v : G->containedVertices(n)) {
                if (G->getCurrentPosition(v) != n) {
                    LOG1 << "ERROR: Vertex pos of " << v << " is not " << n;
                    exit(1);
                }
            }

            EdgeWeight weight = 0;
            std::unordered_set<NodeID> targets;
            edges += G->get_first_invalid_edge(n);
            for (EdgeID e : G->edges_of(n)) {
                NodeID tgt = G->getEdgeTarget(n, e);
                NodeID rev = G->getReverseEdge(n, e);

                weight += G->getEdgeWeight(n, e);

                if (targets.count(tgt) > 0) {
                    LOG1 << "ERROR: Double edge from " << n << " to " << tgt;
                    exit(1);
                } else {
                    targets.insert(tgt);
                }

                if (tgt == n) {
                    LOG1 << "ERROR: Self edge from " << n << " to " << tgt;
                    exit(1);
                }

                if (tgt >= G->n()) {
                    LOG1 << "ERROR: Edge " << e << " of vertex "
                         << n << " points to " << tgt
                         << ", graph only has " << G->n() << "nodes!";
                    exit(1);
                }

                if (G->getEdgeTarget(tgt, rev) != n) {
                    LOG1 << "ERROR: Graph invalid: " << n
                         << "-" << e << " is not the correct edge target to "
                         << tgt << "-" << rev;
                    exit(1);
                }

                if (G->getEdgeWeight(tgt, rev) != G->getEdgeWeight(n, e)) {
                    LOG1 << "ERROR: Graph invalid: " << n << "-" << e
                         << " weight (" << G->getEdgeWeight(n, e)
                         << ") not equal to " << tgt
                         << "-" << rev << " ("
                         << G->getEdgeWeight(tgt, rev) << ")";
                    exit(1);
                }

                if (G->getReverseEdge(tgt, rev) != e) {
                    LOG1 << "ERROR: Graph invalid: " << n << "-"
                         << e << " is not the reverse edge to "
                         << tgt << "-" << rev;
                    exit(1);
                }
            }

            if (weight != G->getWeightedNodeDegree(n)) {
                LOG1 << "ERROR: Node weight in node " << n
                     << " invalid :" << weight
                     << " != " << G->getWeightedNodeDegree(n);
                exit(1);
            }
        }

        if (edges != G->m()) {
            LOG1 << "Error: Invalid number of edges!";
            exit(1);
        }
        LOG0 << "Graph is valid!";
    }

 private:
    static std::vector<NodeID> find_top_k(
        std::vector<std::pair<NodeID, EdgeWeight> > in, size_t k) {
        std::nth_element(in.begin(), in.end() - k, in.end(),
                         [](auto a1, auto a2) {
                             return a1.second < a2.second;
                         });

        std::vector<NodeID> out;
        for (size_t i = in.size() - k; i < in.size(); ++i) {
            out.emplace_back(in[i].first);
        }
        return out;
    }
};
