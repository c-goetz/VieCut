/******************************************************************************
 * per_thread_problem_queue.cpp
 *
 * Source of VieCut.
 *
 ******************************************************************************
 * Copyright (C) 2018-2019 Alexander Noe <alexander.noe@univie.ac.at>
 *
 * Published under the MIT license in the LICENSE file.
 *****************************************************************************/

#pragma once

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/multicut/multicut_problem.h"
#include "common/configuration.h"

class per_thread_problem_queue {
 public:
    typedef std::shared_ptr<multicut_problem> problemPointer;

    per_thread_problem_queue(size_t threads, std::string pq_type)
        : num_threads(threads),
          pop_mutex(threads),
          sizes(threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            sizes[i].second = false;

            if (pq_type == "small_graph") {
                pq.emplace_back(small_graph);
                continue;
            }

            if (pq_type == "bound_sum") {
                pq.emplace_back(bound_sum);
                continue;
            }

            if (pq_type == "few_terminals") {
                pq.emplace_back(few_terminals);
                continue;
            }

            if (pq_type == "upper_bound") {
                pq.emplace_back(upper_bound);
                continue;
            }

            if (pq_type == "lower_bound") {
                pq.emplace_back(lower_bound);
                continue;
            }

            if (pq_type == "bigger_distance") {
                pq.emplace_back(bigger_distance);
                continue;
            }

            if (pq_type == "lower_distance") {
                pq.emplace_back(lower_distance);
                continue;
            }

            if (pq_type == "most_deleted") {
                pq.emplace_back(most_deleted);
                continue;
            }

            pq.emplace_back(lower_bound);
        }
    }
    ~per_thread_problem_queue() { }

    problemPointer pullProblem(size_t local_id) {
        problemPointer current_problem;

        // we (implicitly) add +1 to pq size
        // (as comparison function adds +1 when running is true)
        // when a thread has a running job, as we want to prefer workers
        // that don't currently run a job
        // we remove this +1 when adding a finished problem to the pq afterwards
        pop_mutex[local_id].lock();
        current_problem = pq[local_id].top();
        pq[local_id].pop();
        sizes[local_id].first -= 1;
        sizes[local_id].second = true;
        pop_mutex[local_id].unlock();

        return current_problem;
    }

    size_t addProblem(problemPointer p, size_t local_id) {
        if (sizes[local_id].second) {
            sizes[local_id].second = false;
        }

        auto min =
            std::min_element(sizes.begin(), sizes.end(),
                             [](const auto& e1, const auto& e2) {
                                 return e1.first + e1.second
                                 < e2.first + e2.second;
                             });

        size_t min_index = min - sizes.begin();
        size_t min_element = min->first;

        if (min_element == sizes[local_id].first) {
            min_index = local_id;
        }

        pop_mutex[min_index].lock();
        sizes[min_index].first += 1;
        pq[min_index].push(p);
        pop_mutex[min_index].unlock();
        return min_index;
    }

    bool empty(size_t i) {
        return sizes[i].first == 0;
    }

    bool all_empty() {
        return size() == 0;
    }

    size_t size() {
        return std::accumulate(
            sizes.begin(), sizes.end(),
            std::make_pair(0, true),
            [](const auto& e1, const auto& e2) {
                return std::make_pair(e1.first + e2.first, true);
            }).first;
    }

 private:
    constexpr static auto small_graph =
        [](const problemPointer& p1, const problemPointer& p2) {
            return p1->graph->n() > p2->graph->n();
        };

    constexpr static auto bound_sum =
        [](const problemPointer& p1, const problemPointer& p2) {
            return (p1->upper_bound + p1->lower_bound)
                   > (p2->upper_bound + p2->lower_bound);
        };

    constexpr static auto lower_bound =
        [](const problemPointer& p1, const problemPointer& p2) {
            if (p1->lower_bound == p2->lower_bound) {
                return p1->upper_bound > p2->upper_bound;
            } else {
                return p1->lower_bound > p2->lower_bound;
            }
        };

    constexpr static auto few_terminals =
        [](const problemPointer& p1, const problemPointer& p2) {
            if (p1->terminals.size() == p2->terminals.size()) {
                return lower_bound(p1, p2);
            } else {
                return p1->terminals.size() > p2->terminals.size();
            }
        };

    constexpr static auto upper_bound =
        [](const problemPointer& p1, const problemPointer& p2) {
            if (p1->upper_bound == p2->upper_bound) {
                return p1->lower_bound > p2->lower_bound;
            } else {
                return p1->upper_bound > p2->upper_bound;
            }
        };

    constexpr static auto bigger_distance =
        [](const problemPointer& p1, const problemPointer& p2) {
            return (p1->upper_bound - p1->lower_bound)
                   < (p2->upper_bound - p2->lower_bound);
        };

    constexpr static auto lower_distance =
        [](const problemPointer& p1, const problemPointer& p2) {
            return (p1->upper_bound - p1->lower_bound)
                   > (p2->upper_bound - p2->lower_bound);
        };

    constexpr static auto most_deleted =
        [](const problemPointer& p1, const problemPointer& p2) {
            return p1->deleted_weight < p2->deleted_weight;
        };

    size_t num_threads;

    std::vector<std::priority_queue<problemPointer,
                                    std::vector<problemPointer>,
                                    std::function<
                                        bool(const problemPointer&,
                                             const problemPointer&)> > > pq;

    std::vector<std::mutex> pop_mutex;
    std::vector<std::pair<std::atomic<size_t>, bool> > sizes;
};
