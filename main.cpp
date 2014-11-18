#include "LIFOScheduler.h"
#include "FIFOScheduler.h"
#include "PriorityScheduler.h"
#include <atomic>
#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>
#include <papi.h>
#include <string.h>
#include <sstream>
#include <thread>
#include <queue>
#include <vector>
using namespace std;


#define THREAD_COUNT 4


mutex mtx;
atomic<bool> done(false);

class csr_graph {
    public:
        vector<int> vertices;
        vector<pair<int, int> > edges;
        csr_graph(vector<int> vertices, vector<pair<int, int> > edges);
};


csr_graph::csr_graph(vector<int> vertices, vector<pair<int, int> > edges) :
    vertices(vertices),
    edges(edges) {
}


void relaxLIFO(int current, pair<int, int> neighbor, int dist[],
        LIFOScheduler<int> &q) {
    int alt = dist[current] + neighbor.second;

    if (alt < dist[neighbor.first]) {
        dist[neighbor.first] = alt;
        q.push(neighbor.first);
    }
}


void relaxFIFO(int current, pair<int, int> neighbor, int dist[],
        FIFOScheduler<int> &q) {
    int alt = dist[current] + neighbor.second;

    if (alt < dist[neighbor.first]) {
        dist[neighbor.first] = alt;
        q.push(neighbor.first);
    }
}


void relax(int current, pair<int, int> neighbor, int dist[],
        PriorityScheduler<pair<int, int> > &q) {
    int alt = dist[current] + neighbor.second;

    if (alt < dist[neighbor.first]) {
        dist[neighbor.first] = alt;
        q.push(make_pair(dist[neighbor.first], neighbor.first));
    }
}


int * dijkstra(csr_graph& graph, int source, int thread_count) {
    int * dist = new int[graph.vertices.size()];
    for (int i = 0; i < graph.vertices.size(); ++i) {
        dist[i] = numeric_limits<int>::max();
    }

    dist[source] = 0;

    PriorityScheduler<pair<int, int> > q;
    q.push(make_pair(dist[source], source));

    cout << "Priority Queue:" << endl;

    int current;
    pair<int, int> popped;
    while (!q.empty()) {
        q.pop(popped);

        if (dist[popped.second] != popped.first) {
            continue;
        }
        current = popped.second;

        cout << current << endl;

        int start = graph.vertices[current];
        int end = graph.vertices.size() + 1;
        if (current + 1 < end) {
            end = graph.vertices[current + 1];
        }

        vector<thread> threads;
        for (int offset = start; offset < end; ++offset) {
              threads.push_back(
                    thread(relax, current, graph.edges[offset], dist, ref(q)));
        }

        for (auto& t: threads) {
            t.join();
        }
    }
    
    return dist;
}


int * dijkstra_LIFO(csr_graph& graph, int source, int thread_count) {
    int * dist = new int[graph.vertices.size()];
    for (int i = 0; i < graph.vertices.size(); ++i) {
        dist[i] = numeric_limits<int>::max();
    }

    dist[source] = 0;

    LIFOScheduler<int> q;
    q.push(source);

    cout << "LIFO:" << endl;

    int current;
    while (!q.empty()) {
        q.pop(current);

        cout << current << endl;

        int start = graph.vertices[current];
        int end = graph.vertices.size() + 1;
        if (current + 1 < end) {
            end = graph.vertices[current + 1];
        }


        vector<thread> threads;
        for (int offset = start; offset < end; ++offset) {
              threads.push_back(
                    thread(relaxLIFO, current, graph.edges[offset], dist, ref(q)));
        }

        for (auto& t: threads) {
            t.join();
        }
    }
    
    return dist;
}

int * dijkstra_FIFO(csr_graph& graph, int source, int thread_count) {
    int * dist = new int[graph.vertices.size()];
    for (int i = 0; i < graph.vertices.size(); ++i) {
        dist[i] = numeric_limits<int>::max();
    }

    dist[source] = 0;

    FIFOScheduler<int> q;
    q.push(source);

    cout << "FIFO:" << endl;

    int current;
    while (!q.empty()) {
        q.pop(current);

        cout << "v: " << current << endl;

        int start = graph.vertices[current];
        int end = graph.vertices.size() + 1;
        if (current + 1 < end) {
            end = graph.vertices[current + 1];
        }

        vector<thread> threads;
        for (int offset = start; offset < end; ++offset) {
              threads.push_back(
                    thread(relaxFIFO, current, graph.edges[offset], dist, ref(q)));
        }

        for (auto& t: threads) {
            t.join();
        }
    }
    
    return dist;
}


void relax_work_stealing(int current, pair<int, int> neighbor, int dist[],
        PriorityScheduler<pair<int, int> > &q) {
    int alt = dist[current] + neighbor.second;

    if (alt < dist[neighbor.first]) {
        dist[neighbor.first] = alt;
        q.push(make_pair(dist[neighbor.first], neighbor.first));
    }
}


void work_stealing(csr_graph& graph, PriorityScheduler<pair<int, int> > schedulers[],
            int tid, int dist[], int thread_count) {
    int next = 0;
    int current;
    pair<int, int> popped;
    while (!done) {
        unique_lock<mutex> unq_lock(mtx);
        if (schedulers[tid].empty()) {
            // work stealing
            /*
            int max_thread = tid;
            int max_workload = 1;
            for (int index = 0; index < thread_count; ++index) {
                if (schedulers[index].size() > max_workload) {
                    max_workload = schedulers[index].size();
                    max_thread = index;
                }
            }
            schedulers[max_thread].pop(popped);
            if (schedulers[max_thread].is_complete()) {
                break;
            }

            schedulers[tid].push(popped);
            */
        } else {
            // normal dijkstras
            schedulers[tid].pop(popped);
            if (dist[popped.second] != popped.first) {
                continue;
            }
            current = popped.second;            

            int start = graph.vertices[current];
            int end = graph.vertices.size() + 1;
            if (current + 1 < end) {
                end = graph.vertices[current + 1];
            }

            for (int offset = start; offset < end; ++offset) {
                next = (next + 1) % thread_count;
                relax_work_stealing(current, graph.edges[offset], dist, ref(schedulers[next]));
            }
        }

        bool is_done = true;
        for (int index = 0; index < thread_count; ++index) {
            if (!schedulers[index].empty()) {
                is_done = false;
            }
        }

        if (is_done) {
            done = true;
        }

        unq_lock.unlock();
    }

    done = true;
    cout << "done. tid: " << tid << " done: "<< done << endl;

    for (int index = 0; index < thread_count; ++index) {
        schedulers[index].complete();
    }

}


int * dijkstra_work_stealing(csr_graph& graph, int source, int thread_count) {
    int * dist = new int[graph.vertices.size()];
    for (int i = 0; i < graph.vertices.size(); ++i) {
        dist[i] = numeric_limits<int>::max();
    }

    dist[source] = 0;

    PriorityScheduler<pair<int, int> > * schedulers = new PriorityScheduler<pair<int, int> >[thread_count];
    schedulers[0].push(make_pair(dist[source], source));

    vector<thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.push_back(thread(work_stealing, ref(graph), schedulers, i, dist, thread_count));
    }

    for (auto& t: threads) {
        t.join();
    }
  
    return dist;
}


int * profile(csr_graph& graph, int source) {
    char cache[10000000];
    memset(cache, 0, sizeof cache);
   
    long_long start_usec = PAPI_get_real_usec();
    
    int * dist = dijkstra_FIFO(graph, source, THREAD_COUNT);
    done = false;

    long_long end_usec = PAPI_get_real_usec();
    long_long elapsed = end_usec - start_usec;
    cout << "sssp: " << elapsed << " usec elapsed " << endl;

    return dist;
}


csr_graph build_graph(string gr_filename) {
    struct timespec starttime;
    struct timespec endtime;
    long timediff;

    vector<int> outdegree;
    vector<int> currentoffset;
    vector<int> vertices;
    vector<pair<int, int> > edges;

    char label;
    int V, E, u, v, w;
    string sp;
    string line;

    cout << "Building graph..." << endl;
    try {
        ifstream grfile(gr_filename);

        while (getline(grfile, line)) {
            istringstream iss(line);
            iss >> label;
            if (label == 'c') {
                continue;
            } else if (label == 'p') {
                iss >> sp >> V >> E;

                for (int i = 0; i <= V; ++i) {
                    outdegree.push_back(0);
                    vertices.push_back(0);
                    currentoffset.push_back(0);
                }
                for (int i = 0; i < E; ++i) {
                    edges.push_back(make_pair(0, 0));
                }
            } else if (label == 'a') {
                iss >> u >> v >> w;
                ++outdegree[u];
            }
        }
        grfile.close();
    } catch (const std::bad_alloc&) {
        cout << "bad alloc: probably ran out of memory." << endl;
        exit(-1);
    }

    for (int i = 1; i <= V; ++i) {
        vertices[i] = vertices[i - 1] + outdegree[i - 1];
    }

    try {
        ifstream grfile(gr_filename);
        while (getline(grfile, line)) {
            istringstream iss(line);
            iss >> label;
            if (label == 'c' || label == 'p') {
                continue;
            } else if (label == 'a') {
                iss >> u >> v >> w;
                edges[vertices[u] + currentoffset[u]] = make_pair(v, w);
                ++currentoffset[u];
            }
        }    
        grfile.close();
    } catch (const std::bad_alloc&) {
        cout << "bad alloc: probably ran out of memory." << endl;
        exit(-1);
    }
    cout << "Finished building graph." << endl;

    return csr_graph(vertices, edges);
}


int main() {
    csr_graph graph = build_graph("test.gr");
    int * dist = profile(graph, 0);

    for (int i = 0; i < 9; ++i) {
        cout << "dist[" << i << "] = " << dist[i] << endl;
    }

    assert(dist[0] == 0);
    assert(dist[1] == 4);
    assert(dist[2] == 12);
    assert(dist[3] == 19);
    assert(dist[4] == 21);
    assert(dist[5] == 11);
    assert(dist[6] == 9);
    assert(dist[7] == 8);
    assert(dist[8] == 14);

/*    graph = build_graph("USA-road-t.USA.gr");

    dist = profile(graph, 1); */
}

