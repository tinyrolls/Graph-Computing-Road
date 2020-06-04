#include <stdio.h>
#include <stdlib.h>

#include <queue>

#include "core/graph.hpp"
#include "fma-common/string_formatter.h"
#include "fma-common/check_date.h"
#include "fma-common/logging.h"
#include "toolkits/config_common.h"
#include "fma-common/license.h"

#define MAX_VID 1099511627775ul

typedef double Weight;

size_t parse_edge(const char *p, const char * end, EdgeUnit<Weight> & e) {
  const char * orig = p;
  int64_t t = 0;
  size_t r = 0;
  if (*p == '#') {
    while (*p != '\n') p++;
    e.src = MAX_VID;
    e.dst = MAX_VID;
    return p - orig;
  }
  r = fma_common::TextParserUtils::ParseInt64(p, end, t);
  e.src = t;
  p += r;
  r = fma_common::TextParserUtils::ParseInt64(p, end, t);
  e.dst = t;
  p += r;
  Weight w = 1;
  //r = fma_common::TextParserUtils::ParseDigit(p, end, w);
  e.edge_data = w;
  //p += r;
  return p - orig;
}

bool filter_edge(EdgeUnit<Weight> & e) {
  return e.src != MAX_VID && e.dst != MAX_VID;
}

size_t parse_label(const char * p, const char * end, VertexUnit<VertexId> & v) {
  const char *orig = p;
  int64_t t = 0;
  p += fma_common::TextParserUtils::ParseInt64(p, end, t);
  v.vertex = t;
  p += fma_common::TextParserUtils::ParseInt64(p, end, t);
  v.vertex_data = t;
  return p - orig;
}

bool compare(VertexId a, VertexId b) {
  return a > b;
}

int main(int argc, char ** argv) {

  VertexId vertices = 0;
  VertexId communities = 0;
  std::string input_dir = argv[1];
  std::string label_dir = argv[2];

  Graph<Weight> graph;
  graph.load_txt_undirected(input_dir, vertices, parse_edge, filter_edge);

  Bitmap * full_active = graph.alloc_vertex_bitmap();
  full_active->fill();

  Weight * k = graph.alloc_vertex_array<Weight>();
  graph.fill_vertex_array(k, (Weight)0.0);

  Weight * e_tot = graph.alloc_vertex_array<Weight>();
  graph.fill_vertex_array(e_tot, (Weight)0.0);

  VertexId * Comm = graph.alloc_vertex_array<VertexId>();
  graph.load_vertex_array_txt<VertexId>(Comm, label_dir, parse_label);

  Weight m = graph.stream_vertices<Weight>(
    [&] (VertexId v) {
      record_comm->set_bit(Comm[v]);
      for (auto & e : graph.out_edges(v)) {
        k[v] += e.edge_data;
      }
      return k[v];
    },
    full_active
  ) / 2;

  graph.stream_vertices<VertexId>(
    [&] (VertexId v) {
      write_add(&e_tot[Comm[v]], k[v]);
      return 0;
    },
    full_active
  );

  Weight Q = graph.stream_vertices<Weight>(
    [&] (VertexId v) {
      Weight q = 0;
      for (auto e : graph.out_edges(v)) {
        VertexId nbr = e.neighbour;
        if (Comm[v] == Comm[nbr]) {
          q += e.edge_data;
        }
      }
      q -= 1.0 * k[v] * e_tot[Comm[v]] / (2 * m);
      return q;
    },
    full_active
  ) / (2 * m);

  VertexId * Count = graph.alloc_vertex_array<VertexId>();
  graph.fill_vertex_array(Count, (VertexId)0);
  VertexId num_vertices = graph.get_num_vertices();
  for (VertexId v = 0; v < num_vertices; v++) {
    if (Comm[v] == num_vertices) continue;
    Count[Comm[v]]++;
  }

  std::priority_queue<VertexId> q;
  for (VertexId v = 0; v < num_vertices; v++) {
    q.push(Count[v]);
    if (Count[v] > 0) communities++;
  }

  printf("m = %lf\n", m);
  printf("Communities = %lu\n", communities);
  printf("Q = %lf\n", Q);
  printf("Top 10 largest communities size and percentage\n");
  for (int i = 0; i < 10; i++) {
    VertexId size = q.top();
    q.pop();
    double rate = 100.0 * size / num_vertices;
    printf("%lu\t%lf\n", size, rate);
  }

}
