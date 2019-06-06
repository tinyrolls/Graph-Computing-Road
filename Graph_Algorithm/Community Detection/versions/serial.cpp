
#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <unordered_map>

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
  while (p != end && (*p == ' ' || *p == '\t')) p++;
  r = fma_common::TextParserUtils::ParseInt64(p, end, t);
  e.dst = t;
  p += r;
  Weight w = 1;
  //r = fma_common::TextParserUtils::ParseDigit(p, end, w);
  e.edge_data = w;
  if (e.src == e.dst) e.edge_data /= 2;
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
  while (p != end && (*p == ' ' || *p == '\t')) p++;
  p += fma_common::TextParserUtils::ParseInt64(p, end, t);
  v.vertex_data = t;
  return p - orig;
}

int main(int argc, char ** argv) {
  //fma_common::license::License::ValidateThisHost("louvain", "fma.lic");

  int input_format = 1;
  std::string input_dir = "";
  VertexId vertices = 0;
  std::string output_dir = "";
  std::string label_dir = "";
  std::string license_file = "";

  fma_common::Configuration config;
  toolkits_common::config_common(config, license_file, input_format, input_dir, vertices, true, output_dir);
  config.Add(label_dir, "label_dir", true)
    .Comment("Directory where the initial-label files are stored."
             " Only initial-label files can be stored under the directory");
  config.Parse(argc, argv);
  config.ExitAfterHelp();
  config.Finalize();

  //fma_common::license::License::ValidateThisHost("louvain", license_file);

  Graph<Weight> graph;
  if (input_format == 0) {
    graph.load(input_dir, vertices, true);
  } else {
    graph.load_txt_undirected(input_dir, vertices, parse_edge, filter_edge);
  }

  double exec_time[2] = {0};
  for(int i = 0; i < 2; i++) {
    exec_time[i] -= get_time();
  }

  Bitmap * old_active = graph.alloc_vertex_bitmap();
  old_active->fill();
  Bitmap * new_active = graph.alloc_vertex_bitmap();
  new_active->clear();

  Weight * k = graph.alloc_vertex_array<Weight>();
  graph.fill_vertex_array(k, (Weight)0.0);

  Weight * e_tot = graph.alloc_vertex_array<Weight>();
  graph.fill_vertex_array(e_tot, (Weight)0.0);

  Weight Q_total = 0.0;
  Weight m = 0.0;

  VertexId * label = graph.alloc_vertex_array<VertexId>();
  graph.stream_vertices<VertexId> (
    [&](VertexId v) {
      label[v] = v;
      return 0;
    },
    old_active
  );

  if (!label_dir.empty()) {
    graph.load_vertex_array_txt<VertexId>(label, label_dir, parse_label);
  }

  m = graph.stream_vertices<Weight>(
    [&] (VertexId v) {
      for (auto & e : graph.out_edges(v)) {
        k[v] += e.edge_data;
      }
      e_tot[v] = k[v];
      return k[v];
    },
    old_active
  );
  m /= 2;

  //double * e_
  Weight initQ = graph.stream_vertices<Weight>(
    [&] (VertexId v) {
      Weight q = 0;
      for (auto e : graph.out_edges(v)) {
        VertexId nbr = e.neighbour;
        //if (label[v] == label[nbr]) q += (e.edge_data - 1.0 * k[v] * k[nbr] / (2*m));
        if (label[v] == label[nbr]) q += e.edge_data;
      }
      q -= 1.0 * k[v] * e_tot[label[v]] / (2 * m);
      return q;
    },
    old_active
  ) / (2*m);
  printf("initial Q = %lf\n", initQ);
  //exit(0);

  VertexId num_vertices = graph.get_num_vertices();
  printf("m = %f\n", m);

  LOG() << "done the initial process";

  VertexId active_vertices;
  LOG() << "";
  LOG() << "begin propagating process... ";
  for (int i = 0; i < 200; i++) {
    LOG() << "begin round " << i;
    int iters = 0;
    active_vertices = graph.get_num_vertices();
    old_active->fill();
    while(active_vertices && iters < 10) {
      iters++;
      active_vertices = 0;
      new_active->clear();
      for (unsigned int v = 0; v < graph.get_num_vertices(); v++) {
        if (graph.out_degree[v] == 0 || old_active->get_bit(v) == 0) {
          continue;
        }
        std::unordered_map<VertexId, Weight> count;
        for (auto e : graph.out_edges(v)) {
          if (v == e.neighbour) continue;
          VertexId nbr_label = label[e.neighbour];
          auto it = count.find(nbr_label);
          if (it == count.end()) {
            count[nbr_label] = e.edge_data;
          } else {
            it->second += e.edge_data;
          }
        }
        VertexId old_label = label[v];
        Weight k_in_out = 0.0;
        if (count.find(old_label) != count.end()) {
          k_in_out = count[old_label];
        }
        Weight delta_in = k[v] * (e_tot[old_label]-k[v]) - 2.0 * k_in_out * m;

        Weight delta_in_max = -delta_in;
        VertexId label_max = old_label;
        for (auto & ele : count) {
          VertexId new_label = ele.first;
          if (old_label == new_label) continue;
          Weight k_in_in = ele.second;
          Weight delta_in = 2.0 * k_in_in * m - k[v]*(e_tot[new_label]);
          if (delta_in > delta_in_max) {
            delta_in_max = delta_in;
            label_max = new_label;
          // } else if (delta_in == delta_in_max && new_label < label_max) {
          //   delta_in_max = delta_in;
          //   label_max = new_label;
          }
        }

        if (label_max != old_label) {
          e_tot[old_label] -= k[v];
          label[v] = label_max;
          e_tot[label_max] += k[v];
          active_vertices += 1;
          for (auto & e : graph.out_edges(v)) {
            new_active->set_bit(e.neighbour);
          }
        }
      }

      std::swap(old_active, new_active);
      new_active->clear();
      LOG() << "active_vertices(" << iters << ") = " << active_vertices;
    }
    LOG() << "round " << i << " ends in " << iters << " iterations";
    if (iters == 1) break;
  }

  LOG() << "done propagating process";
  exec_time[0] += get_time();
  printf("first step exec_time = %.2lf seconds\n", exec_time[0]);

  old_active->fill();
  VertexId num_community = graph.stream_vertices<VertexId>(
    [&] (VertexId v) {
      if (e_tot[v] == 0) {
        return 0;
      } else {
        return 1;
      }
    },
    old_active
  );

  Q_total = graph.stream_vertices<Weight>(
    [&] (VertexId v) {
      Weight q = 0;
      for (auto e : graph.out_edges(v)) {
        VertexId nbr = e.neighbour;
        //if (label[v] == label[nbr]) q += (e.edge_data - 1.0 * k[v] * k[nbr] / (2*m));
        if (label[v] == label[nbr]) q += e.edge_data;
      }
      q -= 1.0 * k[v] * e_tot[label[v]] / (2 * m);
      return q;
    },
    old_active
  ) / (2*m);

  VertexId old_community = num_community;
  LOG() << "number of communities is " << num_community;
  LOG() << "the final value of Q_total is " << Q_total;

  for (int iterations = 0; iterations < 100; iterations++) {
    long int * sub_index = graph.alloc_vertex_array<long int>();
    VertexId num_sub_vertices = 0;
    for (VertexId v_i = 0; v_i < num_vertices; v_i++) {
      if (e_tot[v_i] == 0) {
        sub_index[v_i] = -1;
      } else {
        sub_index[v_i] = num_sub_vertices;
        num_sub_vertices++;
      }
    }
    LOG() << "num_sub_vertices = " << num_sub_vertices;
    std::vector< std::unordered_map<VertexId, Weight> > sub_edges(num_sub_vertices);

    // auto make_sub_edges = [&] (VertexId v, VertexAdjList<Weight> & edges) ->VertexId {
    //   if (sub_index[label[v]] == -1) {
    //     return 0;
    //   }
    //   graph.lock_vertex(sub_index[label[v]]);
    //   for (auto & e: graph.out_edges(v)) {
    //     if (sub_index[label[e.neighbour]] == -1) {
    //       continue;
    //     }
    //     if (v < e.neighbour) {
    //       auto pair_it = sub_edges[sub_index[label[v]]].find(sub_index[label[e.neighbour]]);
    //       if (pair_it == sub_edges[sub_index[label[v]]].end()) {
    //         sub_edges[sub_index[label[v]]][sub_index[label[e.neighbour]]] = e.edge_data;
    //       } else {
    //         pair_it->second += e.edge_data;
    //       }
    //     }
    //   }
    //   graph.unlock_vertex(sub_index[label[v]]);
    //   return 1;
    // };

    // graph.stream_edges<VertexId>(
    //   make_sub_edges,
    //   make_sub_edges,
    //   old_active
    // );

    for (VertexId v = 0; v < graph.get_num_vertices(); v++) {
      if (sub_index[label[v]] == -1) {
        continue;
      }
      for (auto & e : graph.out_edges(v)) {
        // if (sub_index[e.neighbour] == -1) {
        //   continue;
        // }
        if (v <= e.neighbour) {
          double eWeight = e.edge_data;
          if (v == e.neighbour) {
            eWeight /= 2;
          }
          //construct_m += eWeight;
          auto pair_it = sub_edges[sub_index[label[v]]].find(sub_index[label[e.neighbour]]);
          if (pair_it == sub_edges[sub_index[label[v]]].end()) {
            sub_edges[sub_index[label[v]]][sub_index[label[e.neighbour]]] = eWeight;  //e.edge_data;
          } else {
            pair_it->second += eWeight; // e.edge_data;
          }
        }
      }
    }

    // for (VertexId v = 0; v < graph.get_num_vertices(); v++) {
    //   if (sub_index[label[v]] == -1) {
    //     continue;
    //   }
    //   for (auto & e : graph.out_edges(v)) {
    //     if (sub_index[label[e.neighbour]] == -1) {
    //       continue;
    //     }
    //     if (v < e.neighbour) {
    //       auto pair_it = sub_edges[sub_index[label[v]]].find(sub_index[label[e.neighbour]]);
    //       if (pair_it == sub_edges[sub_index[label[v]]].end()) {
    //         sub_edges[sub_index[label[v]]][sub_index[label[e.neighbour]]] = e.edge_data;
    //       } else {
    //         pair_it->second += e.edge_data;
    //       }
    //     }
    //   }
    // }

    VertexId sub_edges_num = 0;
    for (VertexId i = 0; i < sub_edges.size(); i++) {
      sub_edges_num += sub_edges[i].size();
    }

    EdgeUnit<Weight> * sub_edge_array = new EdgeUnit<Weight>[sub_edges_num];
    VertexId sub_edge_index = 0;
    for (VertexId v_i = 0; v_i < num_sub_vertices; v_i++) {
      for (auto & ele : sub_edges[v_i]) {
        assert(sub_edge_index < sub_edges_num);
        sub_edge_array[sub_edge_index].src = v_i;
        sub_edge_array[sub_edge_index].dst = ele.first;
        sub_edge_array[sub_edge_index].edge_data = ele.second;
        sub_edge_index++;
      }
    }
    sub_edges.clear();
    //delete sub_edges;

    LOG() << "make_sub_edges done";

/*
    int num_threads = omp_get_num_procs();
    fma_common::OutputFmaStream * fout = new fma_common::OutputFmaStream [num_threads];
    for (int t_i = 0; t_i < num_threads; t_i++) {
      std::string filename = fma_common::StringFormatter::Format("{}/part-r-{}", output_dir, t_i);
      fout[t_i].Open(filename, 64 << 20);
    }

    #pragma omp parallel for
    for (VertexId v_i = 0; v_i < num_sub_vertices; v_i++) {
      for (auto & ele : sub_edges[v_i]) {
        int t_i = omp_get_thread_num();
        std::string line = fma_common::StringFormatter::Format("{} {} {}\n", v_i, ele.first, ele.second);
        fout[t_i].Write(line.c_str(), line.size());
      }
    }
    for (int t_i = 0; t_i < num_threads; t_i++) {
      fout[t_i].Close();
    }

    sub_edges.clear();

    LOG() << "writing subgraph to files done";
*/

    Graph<Weight> sub_graph;
  //  sub_graph.load_txt_undirected(output_dir, 0, parse_edge);
    sub_graph.load_from_array(*sub_edge_array, num_sub_vertices, sub_edges_num, true);
    //sub_edge_array.clear();
    delete sub_edge_array;

    Bitmap * sub_old_active = sub_graph.alloc_vertex_bitmap();
    sub_old_active->fill();
    Bitmap * sub_new_active = sub_graph.alloc_vertex_bitmap();
    sub_new_active->clear();

    Weight * sub_k = sub_graph.alloc_vertex_array<Weight>();
    sub_graph.fill_vertex_array(sub_k, (Weight)0.0);

    Weight * sub_e_tot = sub_graph.alloc_vertex_array<Weight>();
    sub_graph.fill_vertex_array(sub_e_tot, (Weight)0.0);

    VertexId * sub_to_parent_label = sub_graph.alloc_vertex_array<VertexId>();
    sub_graph.fill_vertex_array(sub_to_parent_label, num_vertices);

    //Weight sub_m = m;

    graph.stream_vertices<VertexId> (
      [&] (VertexId v) {
        //if (e_tot[v] <= 0) {
        if (sub_index[v] < 0) {
          return 0;
        }
        //sub_e_tot[sub_index[v]] = e_tot[v];
        //sub_k[sub_index[v]] = e_tot[v];
        sub_to_parent_label[sub_index[v]] = v;
        return 1;
      },
      old_active
    );

    VertexId * sub_label = sub_graph.alloc_vertex_array<VertexId>();
    Weight sub_m = sub_graph.stream_vertices<Weight>(
      [&](VertexId v) {
        sub_label[v] = v;
        for (auto & e : sub_graph.out_edges(v)) {
          sub_k[v] += e.edge_data;
        }
        sub_e_tot[v] = sub_k[v];
        return sub_k[v];
      },
      sub_old_active
    ) / 2;

    VertexId sub_active_vertices;
    LOG() << "";
    printf("sub_m = %lf\n", sub_m);
    LOG() << "begin sub_propagating process " << iterations;
    for (int i = 0; i < 20; i++) {
      LOG() << "begin round " << i;
      int sub_iters = 0;
      sub_active_vertices = sub_graph.get_num_vertices();
      sub_old_active->fill();
      while(sub_active_vertices) {
        sub_iters++;
        sub_active_vertices = 0;
        sub_new_active->clear();
        for (VertexId v = 0; v < sub_graph.get_num_vertices(); v++) {
          if (sub_graph.out_degree[v] == 0 || sub_old_active->get_bit(v) == 0) {
            continue;
          }

          std::unordered_map<VertexId, Weight> sub_count;
          for (auto e : sub_graph.out_edges(v)) {
            if (v == e.neighbour) continue;
            VertexId sub_nbr_label = sub_label[e.neighbour];
            auto it = sub_count.find(sub_nbr_label);
            if (it == sub_count.end()) {
              sub_count[sub_nbr_label] = e.edge_data;
            } else {
              it->second += e.edge_data;
            }
          }
          VertexId sub_old_label = sub_label[v];
          Weight sub_k_in_out = 0.0;
          if (sub_count.find(sub_old_label) != sub_count.end()) {
            sub_k_in_out = sub_count[sub_old_label];
          }
          Weight sub_delta_in = sub_k[v] * (sub_e_tot[sub_old_label] - sub_k[v]) - 2.0 * sub_k_in_out * sub_m;

          Weight sub_delta_in_max = - sub_delta_in;
          VertexId sub_label_max = sub_old_label;
          for (auto & ele : sub_count) {
            VertexId sub_new_label = ele.first;
            if (sub_old_label == sub_new_label) continue;
            Weight sub_k_in_in = ele.second;
            Weight sub_delta_in = 2.0 * sub_k_in_in * sub_m - sub_k[v] * (sub_e_tot[sub_new_label]);
            if (sub_delta_in > sub_delta_in_max) {
              sub_delta_in_max = sub_delta_in;
              sub_label_max = sub_new_label;
            // } else if (sub_delta_in ==  sub_delta_in_max && sub_new_label < sub_label_max) {
            //   sub_delta_in_max = sub_delta_in;
            //   sub_label_max = sub_new_label;
            }
          }

          if (sub_label_max != sub_old_label) {
            sub_e_tot[sub_old_label] -= sub_k[v];
            sub_label[v] = sub_label_max;
            sub_e_tot[sub_label_max] += sub_k[v];
            sub_active_vertices += 1;
            for (auto & e : sub_graph.out_edges(v)) {
              sub_new_active->set_bit(e.neighbour);
            }
          }
        }

        std::swap(sub_old_active, sub_new_active);
        sub_new_active->clear();
        LOG() << "sub_active_vertices = " << sub_active_vertices;
      }
      LOG() << "round " << i << " ends in " << sub_iters << " iterations";
      if (sub_iters == 1) break;
    }

    LOG() << "done the sub_propagating process";

    sub_old_active->fill();
    graph.stream_vertices<VertexId> (
      [&] (VertexId v) {
        if (sub_index[label[v]] <= 0) {
          return 0;
        }
        label[v] = sub_to_parent_label[sub_label[sub_index[label[v]]]];
        return 0;
      },
      old_active
    );

    graph.fill_vertex_array(e_tot, (Weight)0.0);

    graph.stream_vertices<VertexId> (
      [&] (VertexId v) {
        write_add(&e_tot[label[v]], k[v]);
        return 0;
      },
      old_active
    );

    VertexId new_community = graph.stream_vertices<VertexId>(
      [&] (VertexId v) {
        if (e_tot[v] == 0) {
          return 0;
        } else {
          return 1;
        }
      },
      old_active
    );

    Weight new_Q = graph.stream_vertices<Weight>(
      [&](VertexId v) {
        Weight q = 0;
        for (auto e : graph.out_edges(v)) {
          VertexId nbr = e.neighbour;
          //if (label[v] == label[nbr]) q += (e.edge_data - 1.0 * k[v] * k[nbr] / (2*m));
          if (label[v] == label[nbr]) q += e.edge_data;
        }
        q -= 1.0 * k[v] * e_tot[label[v]] / (2 * m);
        return q;
      },
      old_active
    ) / (2*m);

    LOG() << "new_community_number = " << new_community;
    LOG() << "the value of Q after subprocess is " << new_Q;
    if (new_community >= old_community) {
      break;
    }
    old_community = new_community;
    Q_total = new_Q;
  }
  exec_time[1] += get_time();
  printf("Final exec_time = %.2lf seconds\n", exec_time[1]);

  LOG() << "";
  LOG() << "all propagating process done!";
  LOG() << "Q = " << Q_total;
  LOG() << "community number is " << old_community;

  int num_threads = omp_get_num_procs();
  fma_common::OutputFmaStream * fout = new fma_common::OutputFmaStream [num_threads];
  for (int t_i = 0; t_i < num_threads; t_i++) {
    std::string filename = fma_common::StringFormatter::Format("{}/part-r-{}", output_dir, t_i);
    fout[t_i].Open(filename, 64 << 20);
  }

  #pragma omp parallel for
  for (VertexId v_i = 0; v_i < num_vertices; v_i++) {
      if (graph.out_degree[v_i] > 0) {
          int t_i = omp_get_thread_num();
          std::string line = fma_common::StringFormatter::Format("{} {}\n", v_i, label[v_i]);
          fout[t_i].Write(line.c_str(), line.size());
      }
  }
  for (int t_i = 0; t_i < num_threads; t_i++) {
    fout[t_i].Close();
  }

  LOG() << "all processes done!";

  return 0;
}
