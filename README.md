<p align="center">
<img src="https://raw.githubusercontent.com/wiki/tinyrolls/GraphRoad/graphroad.png">
</p>

 > :ocean: The Road of Learning Graph Computation

## Introduce
To record what I learn and what I write.

## Catalog

### Graph_Algorithm
+ **Community Detection** - Find Community in Large Graph.
+ **Subgraph isomorphism** - Find Subgraph isomorphism in Large Graph.

#### Community Detection

##### Several Code (Written by myself)

- [Serial version](Graph_Algorithm/Community_Detection/versions/serial.cpp)    
Read Louvain Paper, and wrote it on Gemini Framework  

- [Sync version](Graph_Algorithm/Community_Detection/versions/new_sync.cpp)  
Divided into different segmentations and steps. Shared memory and multi-cores, parallel and sync-update ( wait for all threads finish local computation and update together).  

- [Async version](Graph_Algorithm/Community_Detection/versions/async.cpp)    
Divided into different segmentations and steps, gain new data and re-compute Immediately

- [Distributed version](Graph_Algorithm/Community_Detection/versions/distributed.cpp)    
Distributed memory, communication consumption takes lots of time. When local computation takes a big part, so that distributed computation gains bigger than communication consumption.


## Based Knowledge
### Core Papers

+ **BSP** - Bullket Sync Parallel
+  **Pregel** - The First Distributed BSP model
+ **GraphLab** - TOOD
+ **GraphChi** - TODO


## Graph_tool
### SNAP Data Collect
+ **solve_snap** - delete the header of original SNAP graph.
