# 图计算常用工具

1. SNAP源图处理
```
  ./solve_snap [input] [output]
```

2. 随机图生成
```
  ./generate_graph []
```

# 图文件格式

1.  Matrix Market格式  <https://math.nist.gov/MatrixMarket/formats.html>
    基于ASCII的可读性很强的文件格式，目的是促进矩阵数据的交流
2.  Dimacs格式 <http://www.dis.uniroma1.it/~challenge9/download.shtml>
    一种非常好的拓扑网络格式   
3.  Pajek格式 <http://mrvar.fdv.uni-lj.si/pajek/>
    边点定义格式
4.  metis格式 <http://glaros.dtc.umn.edu/gkhome/metis/metis/overview>
    配合图切分的软件METIS的格式
5.  Simple edge list
    最最广泛和普遍的图格式
