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

bool compare(VertexId a, VertexId b) {
  return a > b;
}

int main(int argc, char ** argv) {

  std::string input_dir = argv[1];

  Graph<Empty> graph;
  graph.load_txt_undirected(input_dir);

  Bitmap * full_active = graph.alloc_vertex_bitmap();
  full_active->fill();

  VertexId * Count = graph.alloc_vertex_array<VertexId>();
  graph.fill_vertex_array(Count, 0);

  VertexId gather_degree = graph.stream_vertices<VertexId>(
    [&] (VertexId v) {
      VertexId out = graph.out_degree[v];
      write_add(&Count[out], 1);
      return out;
    },
    full_active
  );
  double avg_degree = gather_degree * 1.0 / graph.get_num_vertices();

  




  printf("avg_degree = %lf\n", avg_degree);
  printf("  = %lu\n", communities);
  printf("Q = %lf\n", Q);
  printf("Top 10 largest communities size and percentage\n");
  for (int i = 0; i < 10; i++) {
    VertexId size = q.top();
    q.pop();
    double rate = 100.0 * size / num_vertices;
    printf("%lu\t%lf\n", size, rate);
  }

}
