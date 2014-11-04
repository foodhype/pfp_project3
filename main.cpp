#include <fstream>
#include <iostream>
#include <limits>
#include <papi.h>
#include <string.h>
#include <sstream>
#include <queue>
#include <vector>
using namespace std;


class csr_graph {
    public:
        vector<int> _vertices;
        vector<pair<int, int> > _edges;
        csr_graph(vector<int> vertices, vector<pair<int, int> > edges);
};


csr_graph::csr_graph(vector<int> vertices, vector<pair<int, int> > edges) {
    _vertices = vertices;
    _edges = edges;
}


vector<int> dijkstra(csr_graph& graph, int source) {
    vector<int> dist;
    for (auto vertex: graph._vertices) {
        dist.push_back(numeric_limits<int>::max());
    }

    dist[source] = 0;
   
    queue<int> q; 
    q.push(source);

    while (!q.empty()) {
        int current = q.front();
        q.pop();

        int start = graph._vertices[current];
        int end = graph._vertices.size();
        if (current + 1 < end) {
            end = graph._vertices[current + 1];
        }

        for (int offset = start; offset < end; ++offset) {
            pair<int, int> neighbor = graph._edges[offset];

            int alt = dist[current] + neighbor.second;

            if (alt < dist[neighbor.first]) {
                dist[neighbor.first] = alt;
                q.push(neighbor.first);
            }
        }
    }

    return dist;
}


void profile(csr_graph& graph, int source) {
    char cache[10000000];
    memset(cache, 0, sizeof cache);
    int Events[1] = {PAPI_L1_TCM};
    long_long values[1];
    
    PAPI_start_counters(Events, 1);
    long_long start_usec = PAPI_get_real_usec();
    
    dijkstra(graph, source);
    
    long_long end_usec = PAPI_get_real_usec();
    PAPI_stop_counters(values, 1);
    long_long elapsed = end_usec - start_usec;
    
    cout << "Measurements: " <<
            elapsed << " microseconds, " << 
            values[0] << " L1 cache misses" << endl;

}


csr_graph build_graph(string gr_filename) {
    struct timespec start_time;
    struct timespec end_time;
    long time_diff;

    vector<int> out_degree;
    vector<int> current_offset;
    vector<int> vertices;
    vector<pair<int, int> > edges;

    char label;
    int V, E, u, v, w;
    string sp;
    string line;

    cout << "Building graph..." << endl;
    try {
        ifstream gr_file(gr_filename);

        while (getline(gr_file, line)) {
            istringstream iss(line);
            iss >> label;
            if (label == 'c') {
                continue;
            } else if (label == 'p') {
                iss >> sp >> V >> E;

                cout << V << endl;

                for (int i = 0; i <= V; ++i) {
                    out_degree.push_back(0);
                    vertices.push_back(0);
                    current_offset.push_back(0);
                }
                for (int i = 0; i < E; ++i) {
                    edges.push_back(make_pair(0, 0));
                }
            } else if (label == 'a') {
                iss >> u >> v >> w;
                ++out_degree[u];
            }
        }
        gr_file.close();
    } catch (const std::bad_alloc&) {
        cout << "bad alloc: probably ran out of memory." << endl;
        exit(-1);
    }

    for (int i = 1; i <= V; ++i) {
        vertices[i] = vertices[i - 1] + out_degree[i - 1];
    }

    try {
        ifstream gr_file(gr_filename);
        while (getline(gr_file, line)) {
            istringstream iss(line);
            iss >> label;
            if (label == 'c' || label == 'p') {
                continue;
            } else if (label == 'a') {
                iss >> u >> v >> w;
                edges[vertices[u] + current_offset[u]] = make_pair(v, w);
                ++current_offset[u];
            }
        }    
        gr_file.close();
    } catch (const std::bad_alloc&) {
        cout << "bad alloc: probably ran out of memory." << endl;
        exit(-1);
    }
    cout << "Finished building graph." << endl;

    return csr_graph(vertices, edges);
}


int main() {
    csr_graph graph = build_graph("USA-road-t.USA.gr");
    profile(graph, 1);
}

