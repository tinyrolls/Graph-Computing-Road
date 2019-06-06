
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

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

// inline void LOG(ostream fout) {
//   if (partition_id == 1) {
//       FMA_LOG() << fout;
//   }
// }

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

  int partitions;
  int partition_id;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &partition_id);
  MPI_Comm_size(MPI_COMM_WORLD, &partitions);

  int input_format = 1;
  std::string input_dir = "";
  VertexId vertices = 0;
  double threshold = 0.0001;
  std::string output_dir = "";
  std::string label_dir = "";
  std::string license_file = "";

  fma_common::Configuration config;
  toolkits_common::config_common(config, license_file, input_format, input_dir, vertices, true, output_dir);
  config.Add(threshold, "threshold", true)
    .Comment("Double type to control the calculation accuracy. ");
  config.Add(label_dir, "label_dir", true)
    .Comment("Directory where the initial-label files are stored."
             " Only initial-label files can be stored under the directory");
  config.Parse(argc, argv);
  config.ExitAfterHelp();
  config.Finalize();

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

  VertexId num_vertices = graph.get_num_vertices();
  VertexId part_num_vertices = num_vertices / partitions;

  int num_array[partitions] = {0};
  int offset_array[partitions] = {0};
  for (int i = 0; i < partitions; i++) {
    if (i == partitions - 1) {
      num_array[i] = num_vertices - part_num_vertices*(partitions-1);
    } else {
      num_array[i] = part_num_vertices;
    }
    offset_array[i] = i * part_num_vertices;
    assert(offset_array[i] + num_array[i] <= (int)num_vertices);
  }

  Weight * k = graph.alloc_vertex_array<Weight>();
  Weight * e_tot = graph.alloc_vertex_array<Weight>();
  Weight * Update_e_tot = graph.alloc_vertex_array<Weight>();
  VertexId * Comm = graph.alloc_vertex_array<VertexId>();
  VertexId * Update_Comm = graph.alloc_vertex_array<VertexId>();

  Weight Q_final = 0.0;
  Weight m = 0.0;
  VertexId Communities_final = graph.get_num_vertices();

  Bitmap * part_active = graph.alloc_vertex_bitmap();
  Bitmap * full_active = graph.alloc_vertex_bitmap();
  Bitmap * old_active = graph.alloc_vertex_bitmap();
  Bitmap * new_active = graph.alloc_vertex_bitmap();


  full_active->fill();
  part_active->clear();
  old_active->clear();
  // new_active->clear();

  VertexId test_part_num = graph.stream_vertices<VertexId>(
    [&] (VertexId v) {
      if ((int)v >= offset_array[partition_id] && (int)v < offset_array[partition_id] + num_array[partition_id]) {
        part_active->set_bit(v);
        old_active->set_bit(v);
        return 1;
      }
      return 0;
    },
    full_active
  );
  assert((int)test_part_num == num_array[partition_id]);

  // for (int i = offset_array[partition_id]; i < offset_array[partition_id] + num_array[partition_id]; i++) {
  //   assert(i < num_vertices);
  //   part_active->set_bit(i);
  //   old_active->set_bit(i);
  // }

  if (!label_dir.empty()) {
    graph.load_vertex_array_txt<VertexId>(Comm, label_dir, parse_label);
  }


  graph.fill_vertex_array(k, (Weight)0.0);
  graph.fill_vertex_array(e_tot, (Weight)0.0);
  // graph.fill_vertex_array(Update_e_tot, (Weight)0.0);
  graph.fill_vertex_array(Update_Comm, (VertexId)0);

  m = graph.stream_vertices<Weight>(
    [&] (VertexId v) {
      Comm[v] = v;
      Update_Comm[v] = v;
      for (auto & e : graph.out_edges(v)) {
        k[v] += e.edge_data;
      }
      e_tot[v] = k[v];
      // Update_e_tot[v] = k[v];
      return k[v];
    },
    full_active
  ) / 2;
  printf("m = %f\n", m);

  LOG() << "done the initial process";

  // ----------------- First Step ----------------------

  VertexId active_vertices = graph.get_num_vertices();
  VertexId part_active_vertices = part_num_vertices;
  VertexId old_active_num = active_vertices;
  LOG() << "";
  LOG() << "begin propagating process... ";
  int iters = 0;
  while(1) {
    iters++;
    part_active_vertices = 0;
    new_active->clear();
    graph.fill_vertex_array(Update_e_tot, (Weight)0.0);

    part_active_vertices = graph.stream_vertices<VertexId> (
      [&] (VertexId v) {
        if (graph.out_degree[v] == 0 || old_active->get_bit(v) == 0) {
          return 0;
        } else {
          assert(part_active->get_bit(v));
          std::unordered_map<VertexId, Weight> count;
          for (auto e : graph.out_edges(v)) {
            if (v == e.neighbour) continue;
            VertexId nbr_comm = Comm[e.neighbour];
            auto it = count.find(nbr_comm);
            if (it == count.end()) {
              count[nbr_comm] = e.edge_data;
            } else {
              it -> second += e.edge_data;
            }
          }
          VertexId old_comm = Comm[v];
          Weight k_in_out = 0.0;
          if (count.find(old_comm) != count.end()) {
            k_in_out = count[old_comm];
          }
          Weight delta_in = k[v] * (e_tot[old_comm] - k[v]) - 2 * k_in_out * m;
          Weight delta_in_max = -delta_in;
          VertexId comm_min = old_comm;

          for (auto & ele : count) {
            VertexId new_comm = ele.first;
            Weight k_in_in = ele.second;
            Weight delta_in = 2 * k_in_in * m - k[v]*(e_tot[new_comm]);
            if (delta_in > delta_in_max) {
                delta_in_max = delta_in;
                comm_min = new_comm;
            } else if (delta_in == delta_in_max) {
              if (new_comm < comm_min) {
                comm_min = new_comm;
              }
            }
          }

          if (comm_min != old_comm) {
            if (iters == 1 && comm_min < old_comm) {
              write_sub(&Update_e_tot[old_comm], k[v]);
              write_add(&Update_e_tot[comm_min], k[v]);
              Update_Comm[v] = comm_min;
              for (auto & e : graph.out_edges(v)) {
                if (part_active->get_bit(e.neighbour) != 0) {
                  new_active->set_bit(e.neighbour);
                }
              }
              return 1;
            } else if (iters != 1) {
              write_sub(&Update_e_tot[old_comm], k[v]);
              write_add(&Update_e_tot[comm_min], k[v]);
              Update_Comm[v] = comm_min;
              for (auto & e : graph.out_edges(v)) {
                if (part_active->get_bit(e.neighbour) != 0) {
                  new_active->set_bit(e.neighbour);
                }
              }
              return 1;
            }
          }
        }
        return 0;
      },
      old_active
    );

    std::swap(old_active, new_active);
    new_active->clear();

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Allreduce(&part_active_vertices, &active_vertices, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);

    VertexId * temp_Comm = graph.alloc_vertex_array<VertexId>();
    graph.fill_vertex_array(temp_Comm, (VertexId)0);
    MPI_Allgatherv(&(Update_Comm[offset_array[partition_id]]), num_array[partition_id], MPI_UNSIGNED_LONG, temp_Comm, num_array, offset_array, MPI_UNSIGNED_LONG, MPI_COMM_WORLD);
    std::swap(temp_Comm, Update_Comm);

    Weight * temp_e_tot = graph.alloc_vertex_array<Weight>();
    graph.fill_vertex_array(temp_e_tot, (Weight)0.0);
    MPI_Allreduce(Update_e_tot, temp_e_tot, num_vertices, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    std::swap(temp_e_tot, Update_e_tot);
    MPI_Barrier(MPI_COMM_WORLD);


    graph.stream_vertices<VertexId> (
      [&] (VertexId v) {
        e_tot[v] += Update_e_tot[v];
        Comm[v] = Update_Comm[v];
        return 0;
      },
      full_active
    );

    LOG() << "active_vertices(" << iters << ") = " << active_vertices;
    VertexId diff_num = old_active_num - active_vertices;
    if (diff_num < num_vertices * threshold) {
      break;
    }
    old_active_num = active_vertices;

    if (iters > 1000) {
      break;
    }
  } // End of while

  LOG() << "done propagating process";
  exec_time[0] += get_time();
  printf("%d : first step exec_time = %.2lf seconds\n", partition_id, exec_time[0]);

  VertexId curr_communities = graph.stream_vertices<VertexId>(
    [&] (VertexId v) {
      if (e_tot[v] == 0) {
        return 0;
      } else {
        return 1;
      }
    },
    full_active
  );

  Weight curr_Q = graph.stream_vertices<Weight>(
    [&] (VertexId v) {
      Weight q = 0;
      for (auto e : graph.out_edges(v)) {
        VertexId nbr = e.neighbour;
        if (Comm[v] == Comm[nbr]) q += e.edge_data;
      }
      q -= 1.0 * k[v] * e_tot[Comm[v]] / (2 * m);
      return q;
    },
    full_active
  ) / (2 * m);

  Communities_final = curr_communities;
  Q_final = curr_Q;
  LOG() << "number of communities is " << curr_communities;
  LOG() << "the final value of Q_total is " << curr_Q;


  graph.dealloc_vertex_array(Update_Comm);
  graph.dealloc_vertex_array(Update_e_tot);


  // ------------------ Make Edge --------------------

  int iterations = 0;
  // Weight Q_old = Q_final;
  while (++iterations) {
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

    for (VertexId v = 0; v < graph.get_num_vertices(); v++) {
      if (sub_index[Comm[v]] == -1) {
        continue;
      }
      for (auto & e : graph.out_edges(v)) {
        if (v <= e.neighbour) {
          Weight eWeight = e.edge_data;
          if (v == e.neighbour) {
            eWeight /= 2;
          }
          auto pair_it = sub_edges[sub_index[Comm[v]]].find(sub_index[Comm[e.neighbour]]);
          if (pair_it == sub_edges[sub_index[Comm[v]]].end()) {
            sub_edges[sub_index[Comm[v]]][sub_index[Comm[e.neighbour]]] = eWeight;
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

    LOG() << "make_sub_edges done";



    // --------------------------- Second Step ------------------------

    Graph<Weight> sub_graph;
    sub_graph.load_from_array(*sub_edge_array, num_sub_vertices, sub_edges_num, true);
    delete sub_edge_array;

    VertexId sub_num_vertices = sub_graph.get_num_vertices();
    VertexId sub_part_num_vertices = sub_num_vertices / partitions;

    int sub_num_array[partitions] = {0};
    int sub_offset_array[partitions] = {0};
    for (int i = 0; i < partitions; i++) {
      if (i == partitions - 1) {
        sub_num_array[i] = sub_num_vertices - (partitions-1)*sub_part_num_vertices;
      } else {
        sub_num_array[i] = sub_part_num_vertices;
      }
      sub_offset_array[i] = i*sub_part_num_vertices;
      assert(sub_num_array[i] + sub_offset_array[i] <= (int)sub_num_vertices);
    }


    Bitmap * sub_full_active = sub_graph.alloc_vertex_bitmap();
    Bitmap * sub_part_active = sub_graph.alloc_vertex_bitmap();
    Bitmap * sub_old_active = sub_graph.alloc_vertex_bitmap();
    Bitmap * sub_new_active = sub_graph.alloc_vertex_bitmap();

    Weight * sub_k = sub_graph.alloc_vertex_array<Weight>();
    Weight * sub_e_tot = sub_graph.alloc_vertex_array<Weight>();
    Weight * sub_Update_e_tot = sub_graph.alloc_vertex_array<Weight>();
    VertexId * sub_to_parent_Comm = sub_graph.alloc_vertex_array<VertexId>();
    VertexId * sub_Comm = sub_graph.alloc_vertex_array<VertexId>();
    VertexId * sub_Update_Comm = sub_graph.alloc_vertex_array<VertexId>();

    sub_full_active->fill();

    VertexId sub_test_part_num = sub_graph.stream_vertices<VertexId>(
      [&] (VertexId v) {
        if ((int)v >= sub_offset_array[partition_id] && (int)v < sub_offset_array[partition_id] + sub_num_array[partition_id]) {
          sub_part_active->set_bit(v);
          sub_old_active->set_bit(v);
          return 1;
        }
        return 0;
      },
      sub_full_active
    );
    assert((int)sub_test_part_num == sub_num_array[partition_id]);

    sub_graph.fill_vertex_array(sub_k, (Weight)0.0);
    sub_graph.fill_vertex_array(sub_e_tot, (Weight)0.0);
    // sub_graph.fill_vertex_array(sub_Update_e_tot, (Weight)0.0);
    sub_graph.fill_vertex_array(sub_Comm, (VertexId)0);
    sub_graph.fill_vertex_array(sub_Update_Comm, (VertexId)0);
    sub_graph.fill_vertex_array(sub_to_parent_Comm, (VertexId)0);

    graph.stream_vertices<VertexId> (
      [&] (VertexId v) {
        if (sub_index[v] < 0) {
          return 0;
        }
        sub_to_parent_Comm[sub_index[v]] = v;
        return 1;
      },
      full_active
    );

    Weight sub_m = sub_graph.stream_vertices<Weight> (
      [&] (VertexId v) {
        sub_Comm[v] = v;
        sub_Update_Comm[v] = v;
        for (auto &e : sub_graph.out_edges(v)) {
          sub_k[v] += e.edge_data;
        }
        sub_e_tot[v] = sub_k[v];
        // sub_Update_e_tot[v] = sub_k[v];
        return sub_k[v];
      },
      sub_full_active
    ) / 2;
    LOG() << "";
    printf("sub_m = %lf\n", sub_m);


    VertexId sub_active_vertices = sub_graph.get_num_vertices();
    VertexId sub_old_active_num = sub_active_vertices;
    VertexId sub_part_active_vertices = sub_part_num_vertices;

    LOG() << "";
    LOG() << "begin sub_propagating process " << iterations;
    int sub_iters = 0;
    while(1) {
      sub_iters++;
      sub_new_active->clear();
      sub_graph.fill_vertex_array(sub_Update_e_tot, (Weight)0.0);
      sub_part_active_vertices = sub_graph.stream_vertices<VertexId> (
        [&] (VertexId v) {
          if (sub_graph.out_degree[v] == 0 || sub_old_active->get_bit(v) == 0) {
            return 0;
          }
          assert(sub_part_active->get_bit(v));
          std::unordered_map<VertexId, Weight> sub_count;
          for (auto e : sub_graph.out_edges(v)) {
            if (v == e.neighbour) continue;
            VertexId sub_nbr_comm = sub_Comm[e.neighbour];
            auto it = sub_count.find(sub_nbr_comm);
            if (it == sub_count.end()) {
              sub_count[sub_nbr_comm] = e.edge_data;
            } else {
              it->second += e.edge_data;
            }
          }
          VertexId sub_old_comm = sub_Comm[v];
          Weight sub_k_in_out = 0.0;
          if (sub_count.find(sub_old_comm) != sub_count.end()) {
            sub_k_in_out = sub_count[sub_old_comm];
          }
          Weight sub_delta_in = sub_k[v] * (sub_e_tot[sub_old_comm] - sub_k[v]) - 2 * sub_k_in_out * sub_m;

          Weight sub_delta_in_max = -sub_delta_in;
          VertexId sub_comm_min = sub_old_comm;

          for (auto & ele : sub_count) {
            VertexId sub_new_comm = ele.first;
            Weight sub_k_in_in = ele.second;
            Weight sub_delta_in = 2 * sub_k_in_in * sub_m - sub_k[v] *(sub_e_tot[sub_new_comm]);

            if (sub_delta_in > sub_delta_in_max) {
              sub_delta_in_max = sub_delta_in;
              sub_comm_min = sub_new_comm;
            } else if (sub_delta_in == sub_delta_in_max) {
              if (sub_new_comm < sub_comm_min) {
                sub_comm_min = sub_new_comm;
              }
            }
          }
          if (sub_comm_min != sub_old_comm) {
            if (sub_iters == 1 && sub_comm_min < sub_old_comm) {
              write_sub(&sub_Update_e_tot[sub_old_comm], sub_k[v]);
              write_add(&sub_Update_e_tot[sub_comm_min], sub_k[v]);
              sub_Update_Comm[v] = sub_comm_min;
              for (auto & e : sub_graph.out_edges(v)) {
                if (sub_part_active->get_bit(e.neighbour) != 0) {
                  sub_new_active->set_bit(e.neighbour);
                }
              }
              return 1;
            } else if (sub_iters != 1) {
              write_sub(&sub_Update_e_tot[sub_old_comm], sub_k[v]);
              write_add(&sub_Update_e_tot[sub_comm_min], sub_k[v]);
              sub_Update_Comm[v] = sub_comm_min;
              for (auto & e : sub_graph.out_edges(v)) {
                if (sub_part_active->get_bit(e.neighbour) != 0) {
                  sub_new_active->set_bit(e.neighbour);
                }
              }
              return 1;
            }
          }
          return 0;
        },
        sub_old_active
      );


      std::swap(sub_new_active, sub_old_active);
      sub_new_active->clear();

      MPI_Barrier(MPI_COMM_WORLD);
      MPI_Allreduce(&sub_part_active_vertices, &sub_active_vertices, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);

      VertexId * sub_temp_Comm = sub_graph.alloc_vertex_array<VertexId>();
      sub_graph.fill_vertex_array(sub_temp_Comm, (VertexId)0);
      MPI_Allgatherv(&(sub_Update_Comm[sub_offset_array[partition_id]]), sub_num_array[partition_id], MPI_UNSIGNED_LONG, sub_temp_Comm, sub_num_array, sub_offset_array, MPI_UNSIGNED_LONG, MPI_COMM_WORLD);
      std::swap(sub_temp_Comm, sub_Update_Comm);

      Weight * sub_temp_e_tot = graph.alloc_vertex_array<Weight>();
      sub_graph.fill_vertex_array(sub_temp_e_tot, (Weight)0.0);
      MPI_Allreduce(sub_Update_e_tot, sub_temp_e_tot, sub_num_vertices, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
      std::swap(sub_temp_e_tot, sub_Update_e_tot);
      MPI_Barrier(MPI_COMM_WORLD);

      sub_graph.stream_vertices<VertexId> (
        [&] (VertexId v) {
          sub_e_tot[v] += sub_Update_e_tot[v];
          sub_Comm[v] = sub_Update_Comm[v];
          return 0;
        },
        sub_full_active
      );

      LOG() << "sub_active_vertices(" << sub_iters << ") = " << sub_active_vertices;

      VertexId sub_diff_num = sub_old_active_num - sub_active_vertices;
      if (sub_diff_num < sub_num_vertices * threshold) {
        break;
      }
      sub_old_active_num = sub_active_vertices;

      if (sub_iters > 1000) {
        break;
      }
    }

    graph.stream_vertices<VertexId> (
      [&] (VertexId v) {
        if (sub_index[Comm[v]] < 0) {
          return 0;
        }
        Comm[v] = sub_to_parent_Comm[sub_Comm[sub_index[Comm[v]]]];
        return 1;
      },
      full_active
    );

    graph.fill_vertex_array(e_tot, (Weight)0.0);
    graph.stream_vertices<VertexId> (
      [&] (VertexId v) {
        write_add(&e_tot[Comm[v]], k[v]);
        return 0;
      },
      full_active
    );

    Weight curr_Q = graph.stream_vertices<Weight>(
      [&](VertexId v) {
        Weight q = 0;
        for (auto e : graph.out_edges(v)) {
          VertexId nbr = e.neighbour;
          if (Comm[v] == Comm[nbr]) q += e.edge_data;
        }
        q -= 1.0 * k[v] * e_tot[Comm[v]] / (2 * m);
        return q;
      },
      full_active
    ) / (2 * m);

    VertexId curr_communities = graph.stream_vertices<VertexId>(
      [&] (VertexId v) {
        if (e_tot[v] == 0) {
          return 0;
        } else {
          return 1;
        }
      },
      full_active
    );

    LOG() << "";
    LOG() << "new_community_number = " << curr_communities;
    LOG() << "the value of Q after subprocess is " << curr_Q;
    LOG() << "done the sub_propagating process";

    Q_final = curr_Q;
    Communities_final = curr_communities;

    if (curr_Q - Q_final < threshold * 100) {
      break;
    }
    
    if (iterations > 100) {
      break;
    }
  }  // End of while(++iterations)
  exec_time[1] += get_time();
  printf("Final exec_time = %.2lf seconds\n", exec_time[1]);

  LOG() << "";
  LOG() << "all propagating process done!";
  LOG() << "";
  LOG() << "Q = " << Q_final;
  LOG() << "community number is " << Communities_final;

  if (partition_id == 0) {
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
            std::string line = fma_common::StringFormatter::Format("{} {}\n", v_i, Comm[v_i]);
            fout[t_i].Write(line.c_str(), line.size());
        }
    }
    for (int t_i = 0; t_i < num_threads; t_i++) {
      fout[t_i].Close();
    }
  }

  LOG() << "all processes done!";

  MPI_Finalize();
  return 0;
}
