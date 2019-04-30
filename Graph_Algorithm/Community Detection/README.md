## 算法论文

### Louvain算法

    <!-- 分布式版本 -->

1.  A Scalable Distributed Louvain Algorithm for Large-scale Graph Community Detection
    <!-- 并行多线程版本 -->
2.  Parallel heuristics for scalable community detection
    <!-- 单线程串行版本 -->
3.  Fast unfolding of communities in large networks
    <!-- 模块度的提出 -->
4.  Finding and evaluating community structure in networks
    <!-- 社区发现领域的提出 -->
5.  Community detection in graphs

## 算法过程笔记

1.  载图
2.  初始化
3.  初始化两个Bitmap, old_active, new_active
4.  初始化点总权重k（所有与之相连的边的权重和）, 社区总权重e_tot（所有与社区相连的边的总权重，即邻边权重和）
5.  初始化模块度增益Q, 总边权重m（全图所有边权重和）
6.  初始化社区label, 各点自己为一个社区
    5.  遍历一遍所有点，记下k, e_tot, 计算开始之前点权重即为社区权重，m里所有的边被算了两遍，除二修正。
7.  第一遍大图开始计算
8.  遍历所有点，循环20轮（一般10轮以内就收敛了）
    1.  排除孤立点，设定为自成一个社区
    2.  取当前点的出边，往map里存社区号，边权重和>，这步就是更新count(e_tot)
    3.  计算当前点放入邻接社区的相对增益，留下最大增益社区标签
    4.  更新e_tot, active_vertices
    5.  更新old_active, new_active
    6.  循环20轮propagation
9.  打印LOG
10. 子图进行迭代计算，迭代最大100轮，当没有点更新社区的时候，就跳出循环
11. 根据第一轮的情况构建子图
12. 重复第3步的计算过程
13. 结束运算，输出结果

## 并行算法

1.  载图
2.  分图
    1.  筛选出高度数的点，全节点复制，称之为delegates
    2.  低度数的点按照1D进行分图
3.  并行计算
    1.  节点内进行增益Q计算
    2.  全局同步delegates的最大增益，确定delegates的社区归属
    3.  节点间交换边界点的社区归属
    4.  更新节点内的节点社区归属
    5.  生成新的图，继续循环，直到没有节点更新
4.  最后的小图直接计算

    1.  直接分图
    2.  节点内计算
    3.  节点间更新邻接点
    4.  继续循环，直到没有节点更新
    5.  生成新图，继续循环，直到没有Q值增大
