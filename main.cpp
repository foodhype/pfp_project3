#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>
#include <papi.h>
#include <string.h>
#include <sstream>
#include <thread>
#include <queue>
#include "BlockingQueue.h"
#include <vector>
using namespace std;


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


void relax(int current, pair<int, int> neighbor, int * dist,
        BlockingQueue<int> &q) {
    int alt = dist[current] + neighbor.second;

    if (alt < dist[neighbor.first]) {
        dist[neighbor.first] = alt;
        q.push(neighbor.first);
    }
}


int * dijkstra(csr_graph& graph, int source) {
    int * dist = new int[graph.vertices.size()];
    for (int i = 0; i < graph.vertices.size(); ++i) {
        dist[i] = numeric_limits<int>::max();
    }

    dist[source] = 0;

    BlockingQueue<int> q;
    q.push(source);

    int current;
    while (!q.empty()) {
        q.pop(current);

        int start = graph.vertices[current];
        int end = graph.vertices.size() + 1;
        if (current + 1 < end) {
            end = graph.vertices[current + 1];
        }

        vector<thread> threads;
        for (int offset = start; offset < end; ++offset) {
            threads.push_back(
                    thread(relax, current, graph.edges[offset], ref(dist), ref(q)));
        }

        for (auto& t: threads) {
            t.join();
        }
    }
    
    return dist;
}


int * profile(csr_graph& graph, int source) {
    char cache[10000000];
    memset(cache, 0, sizeof cache);
    
    long_long start_usec = PAPI_get_real_usec();
    
    int * dist = dijkstra(graph, source);
    
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

    assert(dist[0] == 0);
    assert(dist[1] == 4);
    assert(dist[2] == 12);
    assert(dist[3] == 19);
    assert(dist[4] == 21);
    assert(dist[5] == 11);
    assert(dist[6] == 9);
    assert(dist[7] == 8);
    assert(dist[8] == 14);

    graph = build_graph("USA-road-t.USA.gr");
    dist = profile(graph, 1);
}

