
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
  while(p != end && (*p == ' ' || *p == '\t')) p++;
  p += fma_common::TextParserUtils::ParseInt64(p, end, t);
  v.vertex_data = t;
  return p - orig;
}

class LouvainGraph {
  Graph<double> * graph;
  VertexId * label;
  Bitmap * active;
  double * k;
  double * e_tot;
  double m;
  VertexId real_nodes;
  VertexId active_threshold;
  VertexId num_community;
  double Q;

public:

  LouvainGraph(Graph<double> * myGraph, long int threshold = 1) {
    graph = myGraph;
    if (threshold <= 0) {
      active_threshold = graph->get_num_vertices() / 1000;
    } else {
      active_threshold = threshold;
    }
  }

  void init() {
    if (!graph) {
      exit(0);
    }
    label = graph->alloc_vertex_array<VertexId>();
    active = graph->alloc_vertex_bitmap();
    active->fill();
    k = graph->alloc_vertex_array<double>();
    e_tot = graph->alloc_vertex_array<double>();
    graph->fill_vertex_array(k, (double)0.0);
    graph->fill_vertex_array(e_tot, (double)0.0);

    real_nodes = graph->stream_vertices<VertexId> (
      [&] (VertexId v) {
        label[v] = v;
        if (graph->out_degree[v] > 0) {
          return 1;
        }
        return 0;
      },
      active
    );

    m = graph->stream_vertices<double> (
      [&] (VertexId v) {
        for (auto e : graph->out_edges(v)) {
          k[v] += e.edge_data;
        }
        e_tot[v] = k[v];
        return k[v];
      },
      active
    ) / 2;

    //LOG() << "m = " << m;
  }

  void init_from_parent(Graph<double> * pGraph) {
  }

  VertexId update_num_community() {
    num_community = graph->stream_vertices<VertexId> (
      [&] (VertexId v) {
        if (e_tot[v] == 0) {
          return 0;
        } else {
          return 1;
        }
      },
      active
    );
    LOG() << "number of communities is " << num_community;
    return num_community;
  }

  VertexId get_num_community() {
    return num_community;
  }

  void update_e_tot() {
    graph->fill_vertex_array(e_tot, (double)0.0);
    graph->stream_vertices<VertexId>(
      [&] (VertexId v) {
        write_add(&e_tot[label[v]], k[v]);
        return 0;
      },
      active
    );
  }

  double update_Q() {
    Q = graph->stream_vertices<double> (
      [&] (VertexId v) {
        double q = 0.0;
        for (auto e : graph->out_edges(v)) {
          VertexId nbr = e.neighbour;
          if (label[v] == label[nbr]) q += e.edge_data;
        }
        q -= 1.0 * k[v] * e_tot[label[v]] / (2 * m);
        return q;
      },
      active
    ) / (2.0 * m);
    LOG() << "Q = " << Q;
    return Q;
  }

  double get_Q() {
    return Q;
  }

  void update_all() {
    update_e_tot();
    update_num_community();
    update_Q();
  }

  VertexId async_louvain () {
    VertexId active_vertices = graph->stream_vertices<VertexId> (
      [&] (VertexId v) {
        std::unordered_map<VertexId, Weight> count;
        for (auto e : graph->out_edges(v)) {
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
          } else if (delta_in == delta_in_max && new_label < label_max) {
            delta_in_max = delta_in;
            label_max = new_label;
          }
        }

        if (label_max != old_label) {
          write_add(&e_tot[old_label], -k[v]);
          graph->lock_vertex(v);
          label[v] = label_max;
          write_add(&e_tot[label_max], k[v]);
          graph->unlock_vertex(v);
          return 1;
        }
        return 0;
      },
      active
    );
    return active_vertices;
  }

  VertexId series_louvain () {
    VertexId active_vertices = 0;
    for (VertexId v = 0; v < graph->get_num_vertices(); v++) {
      if (graph->out_degree[v] == 0) {
        continue;
      }
      std::unordered_map<VertexId, double> count;
      for(auto e : graph->out_edges(v)) {
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
      double k_in_out = 0.0;
      if (count.find(old_label) != count.end()) {
        k_in_out = count[old_label];
      }
      double delta_out = k[v] * (e_tot[old_label] - k[v]) - 2.0 * k_in_out * m;
      double delta_in_max = -delta_out;
      VertexId label_max = old_label;
      for (auto & ele : count) {
        VertexId new_label = ele.first;
        if (old_label == new_label) continue;
        double k_in_in = ele.second;
        double delta_in = 2.0 * k_in_in * m - k[v] * (e_tot[new_label]);
        if (delta_in > delta_in_max) {
          delta_in_max = delta_in;
          label_max = new_label;
        } else if (delta_in == delta_in_max && new_label < label_max) {
          delta_in_max = delta_in;
          label_max = new_label;
        }
      }
      if (label_max != old_label) {
        e_tot[old_label] -= k[v];
        label[v] = label_max;
        e_tot[label_max] += k[v];
        active_vertices += 1;
      }
    }
    return active_vertices;
  }

  bool louvain_propagate() {
    int iters = 0;
    int recur_iters = 0;
    VertexId active_vertices = graph->get_num_vertices();
    VertexId old_active_vertices = active_vertices;

    while (active_vertices >= active_threshold) {
      old_active_vertices = active_vertices;
      active_vertices = async_louvain();
      LOG() << "active_vertices(" << iters << ") = " << active_vertices;
      iters++;
      if (old_active_vertices == active_vertices) {
        recur_iters++;
      } else {
        recur_iters = 0;
      }
      if (recur_iters >= 5) {
        return false;
      }
    }
    update_num_community();
    update_Q();
    return true;
  }

  void update_by_subgraph() {
    long int * sub_index = graph->alloc_vertex_array<long int>();
    VertexId num_sub_vertices = 0;
    for (VertexId v_i = 0; v_i < graph->get_num_vertices(); v_i++) {
      if (e_tot[v_i] == 0) {
        sub_index[v_i] = -1;
      } else {
        sub_index[v_i] = num_sub_vertices;
        num_sub_vertices++;
      }
    }
    //LOG() << "num_sub_vertices = " << num_sub_vertices;
    std::vector< std::unordered_map<VertexId, double> > sub_edges(num_sub_vertices);

    for (VertexId v = 0; v < graph->get_num_vertices(); v++) {
      if (sub_index[label[v]] == -1) {
        continue;
      }
      for (auto e : graph->out_edges(v)) {
        if (v <= e.neighbour) {
          double eWeight = e.edge_data;
          if (v == e.neighbour) {
            eWeight /= 2;
          }
          auto pair_it = sub_edges[sub_index[label[v]]].find(sub_index[label[e.neighbour]]);
          if (pair_it == sub_edges[sub_index[label[v]]].end()) {
            sub_edges[sub_index[label[v]]][sub_index[label[e.neighbour]]] = eWeight;
          } else {
            pair_it->second += eWeight;
          }
        }
      }
    }

    VertexId sub_edges_num = 0;
    for (VertexId i = 0; i < sub_edges.size(); i++) {
      sub_edges_num += sub_edges[i].size();
    }
    EdgeUnit<double> * sub_edge_array = new EdgeUnit<double>[sub_edges_num];
    VertexId sub_edge_index = 0;
    for (VertexId v_i = 0; v_i < num_sub_vertices; v_i++) {
      for (auto ele : sub_edges[v_i]) {
        assert(sub_edge_index < sub_edges_num);
        sub_edge_array[sub_edge_index].src = v_i;
        sub_edge_array[sub_edge_index].dst = ele.first;
        sub_edge_array[sub_edge_index].edge_data = ele.second;
        sub_edge_index++;
      }
    }
    sub_edges.clear();

    //LOG() << "make_sub_edges done";

    LOG() << "";
    Graph<double> * sub_graph;
    sub_graph = new Graph<double>();
    sub_graph->load_from_array(*sub_edge_array, num_sub_vertices, sub_edges_num, true);
    delete sub_edge_array;

    VertexId * sub_to_parent_label = sub_graph->alloc_vertex_array<VertexId>();
    sub_graph->fill_vertex_array(sub_to_parent_label, graph->get_num_vertices());
    graph->stream_vertices<VertexId> (
      [&] (VertexId v) {
        if (sub_index[v] < 0) {
          return 0;
        }
        sub_to_parent_label[sub_index[v]] = v;
        return 1;
      },
      active
    );

    //cal_num_community();
    //cal_Q();

    LouvainGraph * sub_louvain_graph = new LouvainGraph(sub_graph);
    sub_louvain_graph->init();
    //sub_louvain_graph->async_louvain();
    sub_louvain_graph->louvain_propagate();

    if (sub_louvain_graph->get_num_community() == sub_graph->get_num_vertices()) {
      return;
    } else {
      sub_louvain_graph->update_by_subgraph();
    }

    graph->stream_vertices<VertexId> (
      [&] (VertexId v) {
        if (sub_index[label[v]] <= 0) {
          return 0;
        }
        VertexId sub_index_v_comm = sub_louvain_graph->label[sub_index[label[v]]];
        label[v] = sub_to_parent_label[sub_index_v_comm];
        return 0;
      },
      active
    );
  }


  void min_label() {
    VertexId * min_label = graph->alloc_vertex_array<VertexId>();
    graph->fill_vertex_array(min_label, graph->get_num_vertices());
    graph->stream_vertices<VertexId> (
      [&] (VertexId v) {
        write_min(&min_label[label[v]], v);
        return 0;
      },
      active
    );
    graph->stream_vertices<VertexId> (
      [&] (VertexId v) {
        label[v] = min_label[label[v]];
        return 0;
      },
      active
    );
  }

  VertexId check_large_comm(Bitmap * active_split, VertexId comm_threshold) {
    VertexId * comm_size = graph->alloc_vertex_array<VertexId>();
    graph->fill_vertex_array(comm_size, (VertexId)0);
    graph->stream_vertices<VertexId>(
      [&] (VertexId v) {
        write_add(&comm_size[label[v]], (VertexId)1);
        return 0;
      },
      active
    );

    active_split->clear();

    VertexId large_comm_nodes = graph->stream_vertices<VertexId> (
      [&] (VertexId v) {
        if (comm_size[label[v]] >= comm_threshold) {
          active_split->set_bit(v);
          return 1;
        }
        return 0;
      },
      active
    );

    return large_comm_nodes;
  }

  void split_large_comm(VertexId comm_threshold) {
    LOG() << "";
    LOG() << "begin split large communities recursivelly with threshold = " << comm_threshold;
    //VertexId comm_threshold = real_nodes * split_threshold;
    VertexId * split_label = graph->alloc_vertex_array<VertexId>();

    Bitmap * active_split = graph->alloc_vertex_bitmap();
    //VertexId * comm_size = graph->alloc_vertex_array<VertexId>();
    double * split_k = graph->alloc_vertex_array<double>();
    double * split_e_tot = graph->alloc_vertex_array<double>();
    double * split_m = graph->alloc_vertex_array<double>();

    int split_iters = 0;
    while (split_iters < 100) {
      LOG() << " ";
      LOG() << "begin split process " << split_iters;
      split_iters++;
      min_label();
      VertexId large_comm_nodes = check_large_comm(active_split, comm_threshold);
      if(large_comm_nodes == 0) {
        break;
      }

      graph->fill_vertex_array(split_k, 0.0);
      graph->fill_vertex_array(split_e_tot, 0.0);
      graph->fill_vertex_array(split_m, 0.0);
      VertexId large_comm_edges = graph->stream_vertices<VertexId> (
        [&] (VertexId v) {
          if (active_split->get_bit(v)) {
            split_label[v] = v;
            double local_k = 0.0;
            VertexId local_count = 0;
            for (auto e : graph->out_edges(v)) {
              VertexId nbr = e.neighbour;
              if (label[v] == label[nbr]) {
                local_k += e.edge_data;
                local_count += 1;
              }
            }
            split_k[v] = local_k;
            split_e_tot[v] = local_k;
            split_m[label[v]] += 0.5 * local_k;
            return local_count;
          } else {
            split_label[v] = label[v];
            return (VertexId)0;
          }
        },
        active
      ) / 2;

      LOG() << "large_comm_nodes = " << large_comm_nodes << ", rate of large_comm_nodes is " << (1.0 * large_comm_nodes / real_nodes);
      LOG() << "large_comm_edges = " << large_comm_edges << ", rate of large_comm_edges is " << (1.0 * large_comm_edges / graph->get_num_edges());

      int inner_split_iters = 0;
      VertexId active_vertices = large_comm_nodes;
      //VertexId old_active_vertices;
      int recur_iters = 0;
      while (active_vertices > 0) {
        VertexId old_active_vertices = active_vertices;
        active_vertices = graph->stream_vertices<VertexId> (
          [&] (VertexId v) {
            std::unordered_map<VertexId, double> count;
            for (auto e : graph->out_edges(v)) {
              if (v == e.neighbour || label[v] != label[e.neighbour]) continue;
              VertexId nbr_label = split_label[e.neighbour];
              auto it = count.find(nbr_label);
              if (it == count.end()) {
                count[nbr_label] = e.edge_data;
              } else {
                it->second += e.edge_data;
              }
            }

            VertexId old_label = split_label[v];
            double k_in_out = 0.0;
            double local_m = split_m[label[v]];
            if (count.find(old_label) != count.end()) {
              k_in_out = count[old_label];
            }
            double delta_in = split_k[v] * (split_e_tot[old_label] - split_k[v]) - 2.0 * k_in_out * local_m;

            double delta_in_max = -delta_in;
            VertexId label_max = old_label;
            for (auto & ele : count) {
              VertexId new_label = ele.first;
              if (old_label == new_label) continue;
              double k_in_in = ele.second;
              double delta_in = 2.0 * k_in_in * local_m - split_k[v] * split_e_tot[new_label];
              if (delta_in > delta_in_max) {
                delta_in_max= delta_in;
                label_max = new_label;
              }
            }

            if (label_max != old_label) {
              write_add(&split_e_tot[old_label], -split_k[v]);
              split_label[v] = label_max;
              write_add(&split_e_tot[label_max], split_k[v]);
              return 1;
            } else {
              return 0;
            }
          },
          active_split
        );

        LOG() << "split_active(" << inner_split_iters++ << ") = " << active_vertices;
        if (old_active_vertices == active_vertices) {
          recur_iters++;
        } else {
          recur_iters = 0;
        }
        if (recur_iters >= 5) {
          break;
        }
      }
      //std::swap(label, split_label);

      long int * sub_index = graph->alloc_vertex_array<long int>();
      VertexId num_sub_vertices = 0;
      for (VertexId v_i = 0; v_i < graph->get_num_vertices(); v_i++) {
        if (!active_split->get_bit(v_i) || split_e_tot[v_i] == 0) {
          sub_index[v_i] = -1;
        } else {
          sub_index[v_i] = num_sub_vertices;
          num_sub_vertices++;
        }
      }
      LOG() << "num_sub_vertices = " << num_sub_vertices;
      std::vector< std::unordered_map<VertexId, double> > sub_edges(num_sub_vertices);

      for (VertexId v = 0; v < graph->get_num_vertices(); v++) {
        if (sub_index[split_label[v]] == -1) {
          continue;
        }
        for (auto e : graph->out_edges(v)) {
          if (v <= e.neighbour && label[v] == label[e.neighbour]) {
            double eWeight = e.edge_data;
            if (v == e.neighbour) {
              eWeight /= 2;
            }
            auto pair_it = sub_edges[sub_index[split_label[v]]].find(sub_index[split_label[e.neighbour]]);
            if (pair_it == sub_edges[sub_index[split_label[v]]].end()) {
              sub_edges[sub_index[split_label[v]]][sub_index[split_label[e.neighbour]]] = eWeight;
            } else {
              pair_it->second += eWeight;
            }
          }
        }
      }
      std::swap(label, split_label);

      VertexId sub_edges_num = 0;
      for (VertexId i = 0; i < sub_edges.size(); i++) {
        sub_edges_num += sub_edges[i].size();
      }
      EdgeUnit<double> * sub_edge_array = new EdgeUnit<double>[sub_edges_num];
      VertexId sub_edge_index = 0;
      for (VertexId v_i = 0; v_i < num_sub_vertices; v_i++) {
        for (auto ele : sub_edges[v_i]) {
          assert(sub_edge_index < sub_edges_num);
          sub_edge_array[sub_edge_index].src = v_i;
          sub_edge_array[sub_edge_index].dst = ele.first;
          sub_edge_array[sub_edge_index].edge_data = ele.second;
          sub_edge_index++;
        }
      }
      sub_edges.clear();

      Graph<double> * sub_graph;
      sub_graph = new Graph<double>();
      sub_graph->load_from_array(*sub_edge_array, num_sub_vertices, sub_edges_num, true);
      delete sub_edge_array;

      VertexId * sub_to_parent_label = sub_graph->alloc_vertex_array<VertexId>();
      sub_graph->fill_vertex_array(sub_to_parent_label, graph->get_num_vertices());
      graph->stream_vertices<VertexId>(
        [&] (VertexId v) {
          if (sub_index[v] < 0) {
            return 0;
          }
          sub_to_parent_label[sub_index[v]] = v;
          return 1;
        },
        active
      );

      //cal_num_community();
      //cal_Q();

      LouvainGraph * sub_louvain_graph = new LouvainGraph(sub_graph);
      sub_louvain_graph->init();
      //sub_louvain_graph->async_louvain();
      sub_louvain_graph->louvain_propagate();

      if (sub_louvain_graph->get_num_community() == sub_graph->get_num_vertices()) {
        return;
      } else {
        sub_louvain_graph->update_by_subgraph();
      }

      graph->stream_vertices<VertexId> (
        [&] (VertexId v) {
          if (sub_index[label[v]] <= 0) {
            return 0;
          }
          VertexId sub_index_v_comm = sub_louvain_graph->label[sub_index[label[v]]];
          label[v] = sub_to_parent_label[sub_index_v_comm];
          return 0;
        },
        active_split
      );
    }
    LOG() << "large community split process done";
  }

  void output(std::string output_dir) {
    int num_threads = omp_get_num_procs();
    fma_common::OutputFmaStream * fout = new fma_common::OutputFmaStream [num_threads];
    for (int t_i = 0; t_i < num_threads; t_i++) {
      std::string filename = fma_common::StringFormatter::Format("{}/part-r-{}", output_dir, t_i);
      fout[t_i].Open(filename, 64 << 20);
    }

    #pragma omp parallel for
    for (VertexId v_i = 0; v_i < graph->get_num_vertices(); v_i++) {
        if (graph->out_degree[v_i] > 0) {
            int t_i = omp_get_thread_num();
            std::string line = fma_common::StringFormatter::Format("{} {}\n", v_i, label[v_i]);
            fout[t_i].Write(line.c_str(), line.size());
        }
    }
    for (int t_i = 0; t_i < num_threads; t_i++) {
      fout[t_i].Close();
    }
    LOG() << "writing to files done!";
  }
};

int main(int argc, char ** argv) {
  int input_format = 1;
  std::string input_dir = "";
  VertexId vertices = 0;
  std::string output_dir = "";
  std::string license_file = "";
  VertexId active_threshold = 0;
  //int use_series = 0;
  //int series_iterations = 20;
  VertexId comm_threshold = 0;

  fma_common::Configuration config;
  toolkits_common::config_common(config, license_file, input_format, input_dir, vertices, true, output_dir);

  config.Add(active_threshold, "active_threshold", true)
    .Comment("Threshold of active_vertices in original graph."
     " Louvain process will stop when active vertices is smaller than active_threshold. "
     " If you want louvain process executed thoroughly until no active vertices, you can set this parameter to 1."
    " When this parameter is smaller than or equal to 0, threshold will be one thousandth of the vertices of original graph");
  /*
  config.Add(use_series, "use_series", true)
    .Comment("If to use series process after parallel process has reached threshold");
  config.Add(series_iterations, "series_iterations", true)
    .Comment("Iterations of optimized series function.");
    */
  config.Add(comm_threshold, "comm_threshold", true)
    .Comment("Community having larger proportion than this value will be recursivelly divided using louvain method."
            " When value is set to zero, this process will noe be executed.");
  config.Parse(argc, argv);
  config.ExitAfterHelp();
  config.Finalize();

  //fma_common::license::License::ValidateThisHost("louvain", license_file);

  if (comm_threshold < 0) {
    LOG() << "error: comm_threshold should be larger or equal to zero";
    exit(0);
  }

  Graph<Weight> * graph;
  graph = new Graph<double>();
  if (input_format == 0) {
    graph->load(input_dir, vertices, true);
  } else {
    graph->load_txt_undirected(input_dir, vertices, parse_edge, filter_edge);
  }

  double exec_time = 0;
  exec_time -= get_time();

  LOG() << "";
  //LOG() << "begin louvain propagation";

  LouvainGraph * louvain_graph = new LouvainGraph(graph, active_threshold);
  louvain_graph->init();
  louvain_graph->louvain_propagate();
  louvain_graph->update_by_subgraph();

  LOG() << "";
  LOG() << "the main louvain process is done!";
  //louvain_graph->update_tot();
  louvain_graph->update_all();
  //louvain_graph->cal_Q();


  //louvain_graph->split_large_comm(comm_threshold);
  //louvain_graph->update_tot();
  //louvain_graph->cal_Q();

  louvain_graph->output(output_dir);

  LOG() << "all processes done!";

  return 0;
} 
