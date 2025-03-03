/******************************************************************************
 * mincut_algo_test.h
 *
 * Source of VieCut.
 *
 ******************************************************************************
 * Copyright (C) 2018 Alexander Noe <alexander.noe@univie.ac.at>
 *
 * Published under the MIT license in the LICENSE file.
 *****************************************************************************/

#include <type_traits>
#ifdef PARALLEL
#include "algorithms/global_mincut/viecut.h"
#include "parallel/algorithm/exact_parallel_minimum_cut.h"
#include "parallel/algorithm/parallel_cactus.h"
#else
#include "algorithms/global_mincut/cactus/cactus_mincut.h"
#include "algorithms/global_mincut/ks_minimum_cut.h"
#include "algorithms/global_mincut/matula_approx.h"
#include "algorithms/global_mincut/noi_minimum_cut.h"
#include "algorithms/global_mincut/padberg_rinaldi.h"
#include "algorithms/global_mincut/stoer_wagner_minimum_cut.h"
#include "algorithms/global_mincut/viecut.h"
#endif
#include "gtest/gtest.h"
#include "io/graph_io.h"

TEST(CactusCutTest, UnweightedGraphFromFile) {
    configuration::getConfig()->save_cut = true;
    for (size_t i = 0; i < 1; ++i) {
        random_functions::setSeed(time(NULL) + i * 623412);
        std::shared_ptr<graph_access> G = graph_io::readGraphWeighted(
            std::string(VIECUT_PATH) + "/graphs/small.metis");
#ifdef PARALLEL
        parallel_cactus mc;
#else
        cactus_mincut mc;
#endif
        auto [cut, mg] = mc.findAllMincuts(G);

        ASSERT_EQ(cut, 2);

        std::vector<size_t> sizes(6, 0);
        std::vector<size_t> desired_sizes = { 0, 0, 0, 0, 2, 0 };
        for (NodeID n : mg->nodes()) {
            sizes[mg->containedVertices(n).size()]++;
        }
        ASSERT_EQ(sizes, desired_sizes);
    }
}

TEST(CactusCutTest, WeightedGraphFromFile) {
    configuration::getConfig()->save_cut = true;
    for (size_t i = 0; i < 1; ++i) {
        random_functions::setSeed(time(NULL) + i * 623412);
        std::shared_ptr<graph_access> G = graph_io::readGraphWeighted(
            std::string(VIECUT_PATH) + "/graphs/small-wgt.metis");
#ifdef PARALLEL
        parallel_cactus mc;
#else
        cactus_mincut mc;
#endif

        auto [cut, mg] = mc.findAllMincuts(G);
        ASSERT_EQ(cut, 3);

        std::vector<size_t> sizes(10, 0);
        std::vector<size_t> desired_sizes = { 0, 0, 0, 0, 2, 0, 0, 0, 0, 0 };
        for (NodeID n : mg->nodes()) {
            sizes[mg->containedVertices(n).size()]++;
        }
        ASSERT_EQ(sizes, desired_sizes);
    }
}

TEST(CactusCutTest, SmallClique) {
    configuration::getConfig()->save_cut = true;
    for (size_t i = 0; i < 1; ++i) {
        random_functions::setSeed(time(NULL) + (i * 623412));
        std::shared_ptr<graph_access> G = std::make_shared<graph_access>();

        G->start_construction(4, 16);

        for (NodeID i = 0; i < 4; ++i) {
            for (NodeID j = 0; j < 4; ++j) {
                G->new_edge(i, j);
            }
        }

        G->finish_construction();

#ifdef PARALLEL
        parallel_cactus mc;
#else
        cactus_mincut mc;
#endif

        auto [cut, mg] = mc.findAllMincuts(G);
        ASSERT_EQ(cut, 3);
        ASSERT_EQ(mg->number_of_nodes(), 5);

        std::vector<size_t> sizes(3, 0);
        std::vector<size_t> desired_sizes = { 1, 4, 0 };
        for (NodeID n : mg->nodes()) {
            sizes[mg->containedVertices(n).size()]++;
        }
        ASSERT_EQ(sizes, desired_sizes);
    }
}

TEST(CactusCutTest, RingOfVerySmallCliques) {
    configuration::getConfig()->save_cut = true;
    for (size_t i = 0; i < 1; ++i) {
        random_functions::setSeed(time(NULL) + i * 623412);
        std::shared_ptr<graph_access> G = std::make_shared<graph_access>();

        NodeID num_cliques = 4;

        G->start_construction(num_cliques * 3, num_cliques * 20);

        for (NodeID k = 0; k < num_cliques; ++k) {
            if (k > 0) {
                G->new_edge(3 * k, 3 * (k - 1));
            } else {
                G->new_edge(0, (num_cliques - 1) * 3);
            }

            if (k < (num_cliques - 1)) {
                G->new_edge(3 * k, 3 * (k + 1));
            } else {
                G->new_edge(3 * k, 0);
            }

            for (NodeID i = 0; i < 3; ++i) {
                for (NodeID j = 0; j < 3; ++j) {
                    G->new_edge(i + (3 * k), j + (3 * k));
                }
            }
        }

        G->finish_construction();

#ifdef PARALLEL
        parallel_cactus mc;
#else
        cactus_mincut mc;
#endif

        auto [cut, mg] = mc.findAllMincuts(G);
        ASSERT_EQ(cut, 2);
        ASSERT_EQ(mg->number_of_nodes(), num_cliques * 3);

        std::vector<size_t> sizes(3, 0);
        std::vector<size_t> desired_sizes = { 0, num_cliques*3, 0 };
        for (NodeID n : mg->nodes()) {
            sizes[mg->containedVertices(n).size()]++;
        }
        ASSERT_EQ(sizes, desired_sizes);
    }
}

TEST(CactusCutTest, SimplePath) {
    configuration::getConfig()->save_cut = true;
    std::vector<size_t> weights = { 1, 10, 1000 };
    for (auto wgt : weights) {
        std::shared_ptr<graph_access> G = std::make_shared<graph_access>();
        size_t length = 10;
        G->start_construction(length, 2 * length);
        for (size_t i = 0; i < length - 1; ++i) {
            EdgeID e = G->new_edge(i, i + 1);
            G->setEdgeWeight(e, wgt);
            EdgeID e2 = G->new_edge(i + 1, i);
            G->setEdgeWeight(e2, wgt);
        }
        G->finish_construction();
#ifdef PARALLEL
        parallel_cactus mc;
#else
        cactus_mincut mc;
#endif

        auto [cut, mg] = mc.findAllMincuts(G);
        ASSERT_EQ(cut, wgt);
        ASSERT_EQ(mg->number_of_nodes(), length);
    }
}

TEST(CactusCutTest, RingOfSmallCliques) {
    configuration::getConfig()->save_cut = true;
    for (size_t i = 0; i < 1; ++i) {
        random_functions::setSeed(time(NULL) + i * 623412);
        std::shared_ptr<graph_access> G = std::make_shared<graph_access>();

        NodeID num_cliques = 3;

        G->start_construction(num_cliques * 4, num_cliques * 20);

        for (NodeID k = 0; k < num_cliques; ++k) {
            if (k > 0) {
                G->new_edge(4 * k, 4 * (k - 1));
            } else {
                G->new_edge(0, (num_cliques - 1) * 4);
            }

            if (k < (num_cliques - 1)) {
                G->new_edge(4 * k, 4 * (k + 1));
            } else {
                G->new_edge(4 * k, 0);
            }

            for (NodeID i = 0; i < 4; ++i) {
                for (NodeID j = 0; j < 4; ++j) {
                    G->new_edge(i + (4 * k), j + (4 * k));
                }
            }
        }

        G->finish_construction();

#ifdef PARALLEL
        parallel_cactus mc;
#else
        cactus_mincut mc;
#endif

        auto [cut, mg] = mc.findAllMincuts(G);
        ASSERT_EQ(cut, 2);
        ASSERT_EQ(mg->number_of_nodes(), num_cliques);

        std::vector<size_t> sizes(6, 0);
        std::vector<size_t> desired_sizes = { 0, 0, 0, 0, num_cliques, 0 };
        for (NodeID n : mg->nodes()) {
            sizes[mg->containedVertices(n).size()]++;
        }
        ASSERT_EQ(sizes, desired_sizes);
    }
}

TEST(CactusCutTest, MultipleMincuts) {
    configuration::getConfig()->save_cut = true;
    for (size_t i = 0; i < 1; ++i) {
        random_functions::setSeed(time(NULL) + i * 623412);
        std::shared_ptr<graph_access> G = graph_io::readGraphWeighted(
            std::string(VIECUT_PATH) + "/graphs/small.metis");

        G->setEdgeWeight(3, 2);
        G->setEdgeWeight(13, 2);
        G->setEdgeWeight(14, 2);
        G->setEdgeWeight(24, 2);

#ifdef PARALLEL
        parallel_cactus mc;
#else
        cactus_mincut mc;
#endif

        auto [cut, mg] = mc.findAllMincuts(G);

        ASSERT_EQ(cut, 3);
        ASSERT_EQ(mg->number_of_nodes(), 5);

        std::vector<size_t> sizes(6, 0);
        std::vector<size_t> desired_sizes = { 0, 4, 0, 0, 1, 0 };
        for (NodeID n : mg->nodes()) {
            sizes[mg->containedVertices(n).size()]++;
        }
        ASSERT_EQ(sizes, desired_sizes);
    }
}

TEST(CactusCutTest, LargeClique) {
    configuration::getConfig()->save_cut = true;
    for (size_t i = 0; i < 1; ++i) {
        random_functions::setSeed(time(NULL) + i * 623412);
        std::shared_ptr<graph_access> G = std::make_shared<graph_access>();

        G->start_construction(10, 100);

        for (NodeID i = 0; i < 10; ++i) {
            for (NodeID j = 0; j < 10; ++j) {
                G->new_edge(i, j);
            }
        }

        G->finish_construction();

#ifdef PARALLEL
        parallel_cactus mc;
#else
        cactus_mincut mc;
#endif

        auto [cut, mg] = mc.findAllMincuts(G);

        ASSERT_EQ(cut, 9);
        ASSERT_EQ(mg->number_of_nodes(), 11);

        std::vector<size_t> sizes(3, 0);
        std::vector<size_t> desired_sizes = { 1, 10, 0 };
        for (NodeID n : mg->nodes()) {
            sizes[mg->containedVertices(n).size()]++;
        }
        ASSERT_EQ(sizes, desired_sizes);
    }
}

TEST(CactusCutTest, GraphFromNKPaper) {
    configuration::getConfig()->save_cut = true;
    for (size_t i = 0; i < 1; ++i) {
        random_functions::setSeed(time(NULL) + i * 623412);
        // Example graph from H. Nagamochi, T. Kameda
        // - Constructing Cactus Representation for all Minimum Cuts
        // - in an Undirected Network
        std::shared_ptr<graph_access> G = std::make_shared<graph_access>();
        G->start_construction(6, 20);

        G->new_edge(0, 1, 3);
        G->new_edge(0, 4, 1);
        G->new_edge(0, 5, 1);

        G->new_edge(1, 0, 3);
        G->new_edge(1, 2, 1);
        G->new_edge(1, 3, 1);

        G->new_edge(2, 1, 1);
        G->new_edge(2, 3, 2);
        G->new_edge(2, 5, 1);

        G->new_edge(3, 1, 1);
        G->new_edge(3, 2, 2);
        G->new_edge(3, 4, 1);

        G->new_edge(4, 0, 1);
        G->new_edge(4, 3, 1);
        G->new_edge(4, 5, 2);

        G->new_edge(5, 0, 1);
        G->new_edge(5, 2, 1);
        G->new_edge(5, 4, 2);

        G->finish_construction();

#ifdef PARALLEL
        parallel_cactus mc;
#else
        cactus_mincut mc;
#endif

        auto [cut, mg] = mc.findAllMincuts(G);
        ASSERT_EQ(cut, 4);
        ASSERT_EQ(mg->number_of_nodes(), 7);
        ASSERT_EQ(mg->number_of_edges(), 18);

        std::vector<size_t> sizes(3, 0);
        std::vector<size_t> desired_sizes = { 2, 4, 1 };
        for (NodeID n : mg->nodes()) {
            sizes[mg->containedVertices(n).size()]++;
        }
        ASSERT_EQ(sizes, desired_sizes);
    }
}

TEST(CactusCutTest, GraphFromNNIPaper) {
    configuration::getConfig()->save_cut = true;
    // Example graph from H. Nagamochi, Y. Nakao, T. Ibaraki
    // - A Fast Algorithm for Cactus Representation of Minimum Cuts
    for (size_t i = 0; i < 3; ++i) {
        random_functions::setSeed(time(NULL) + i * 623412);

        std::shared_ptr<graph_access> G = std::make_shared<graph_access>();
        G->start_construction(19, 100);

        G->new_node();
        G->new_edge(0, 1, 1);
        G->new_edge(0, 4, 1);
        G->new_edge(0, 9, 1);
        G->new_edge(0, 18, 1);

        G->new_node();
        G->new_edge(1, 0, 1);
        G->new_edge(1, 2, 2);
        G->new_edge(1, 4, 1);

        G->new_node();
        G->new_edge(2, 1, 2);
        G->new_edge(2, 3, 1);
        G->new_edge(2, 4, 1);

        G->new_node();
        G->new_edge(3, 2, 1);
        G->new_edge(3, 5, 3);

        G->new_node();
        G->new_edge(4, 0, 1);
        G->new_edge(4, 1, 1);
        G->new_edge(4, 2, 1);
        G->new_edge(4, 5, 1);

        G->new_node();
        G->new_edge(5, 3, 3);
        G->new_edge(5, 4, 1);
        G->new_edge(5, 6, 1);
        G->new_edge(5, 7, 1);

        G->new_node();
        G->new_edge(6, 5, 1);
        G->new_edge(6, 7, 2);
        G->new_edge(6, 8, 1);

        G->new_node();
        G->new_edge(7, 5, 1);
        G->new_edge(7, 6, 2);
        G->new_edge(7, 8, 1);

        G->new_node();
        G->new_edge(8, 6, 1);
        G->new_edge(8, 7, 1);
        G->new_edge(8, 9, 1);
        G->new_edge(8, 10, 2);

        G->new_node();
        G->new_edge(9, 0, 1);
        G->new_edge(9, 8, 1);
        G->new_edge(9, 10, 2);

        G->new_node();
        G->new_edge(10, 8, 2);
        G->new_edge(10, 9, 2);
        G->new_edge(10, 11, 2);
        G->new_edge(10, 12, 1);

        G->new_node();
        G->new_edge(11, 10, 2);
        G->new_edge(11, 12, 1);
        G->new_edge(11, 18, 1);

        G->new_node();
        G->new_edge(12, 10, 1);
        G->new_edge(12, 11, 1);
        G->new_edge(12, 13, 1);
        G->new_edge(12, 14, 1);

        G->new_node();
        G->new_edge(13, 12, 1);
        G->new_edge(13, 14, 1);
        G->new_edge(13, 15, 1);
        G->new_edge(13, 17, 1);
        G->new_edge(13, 18, 1);

        G->new_node();
        G->new_edge(14, 12, 1);
        G->new_edge(14, 13, 1);
        G->new_edge(14, 15, 2);

        G->new_node();
        G->new_edge(15, 13, 1);
        G->new_edge(15, 14, 2);
        G->new_edge(15, 16, 2);

        G->new_node();
        G->new_edge(16, 15, 2);
        G->new_edge(16, 17, 2);

        G->new_node();
        G->new_edge(17, 13, 1);
        G->new_edge(17, 16, 2);
        G->new_edge(17, 18, 1);

        G->new_node();
        G->new_edge(18, 0, 1);
        G->new_edge(18, 11, 1);
        G->new_edge(18, 13, 1);
        G->new_edge(18, 17, 1);

        G->finish_construction();

#ifdef PARALLEL
        parallel_cactus mc;
#else
        cactus_mincut mc;
#endif

        auto [cut, mg] = mc.findAllMincuts(G);
        ASSERT_EQ(cut, 4);
        ASSERT_EQ(mg->number_of_nodes(), 21);
        ASSERT_EQ(mg->number_of_edges(), 54);

        std::vector<size_t> sizes(4, 0);
        std::vector<size_t> desired_sizes = { 4, 15, 2, 0 };
        for (NodeID n : mg->nodes()) {
            sizes[mg->containedVertices(n).size()]++;
        }
        ASSERT_EQ(sizes, desired_sizes);
    }
}
