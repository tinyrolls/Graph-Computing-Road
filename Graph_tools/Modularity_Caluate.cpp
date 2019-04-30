
#include <stdio.h>
#include <stdlib.h>

#include "core/graph.hpp"
#include "fma-common/string_formatter.h"
#include "fma-common/check_date.h"
#include "fma-common/logging.h"
#include "toolkits/config_common.h"
#include "fma-common/license.h"

#define MAX_VID 1099511627775ul

typedef float Weight;

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

int main(int argc, char ** argv) {

  std::string edge_dir = argv[1];
  std::string comm_dir = argv[2];
  VertexId vertices = 0;

  Graph<Weight> graph;
  graph.load_txt_undirected(edge_dir, vertices, parse_edge, filter_edge);

  Bitmap * full_active = graph.alloc_vertex_bitmap();
  full_active->fill();

  VertexId * Comm = graph.alloc_vertex_array<VertexId>();
  graph.load_vertex_array_txt<VertexId>(Comm, comm_dir, parse_label);

  Weight * k = graph.alloc_vertex_array<Weight>();
  graph.fill_vertex_array(k, (Weight)0.0);

  Weight m = graph.stream_vertices<Weight>(
    [&] (VertexId v) {
      for (auto & e : graph.out_edges(v)) {
        k[v] += e.edge_data;
      }
      return k[v];
    },
    full_active
  );
  m /= 2;

  Weight Q_total = graph.stream_vertices<Weight>(
    [&] (VertexId v) {
      Weight q = 0;
      for (auto e : graph.out_edges(v)) {
        VertexId nbr = e.neighbour;
        if (Comm[v] == Comm[nbr]) {
          q += (e.edge_data - 1.0 * k[v] * k[nbr] / (2 * m));
        }
      }
      return q;
    },
    full_active
  ) / (2 * m);
  LOG() << "Q_total value : " << Q_total;
  return 0;
}
