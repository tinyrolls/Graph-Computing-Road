# 测试记录

## 服务器信息

### 硬件

| CPU                                          | Memory | 磁盘       |
| -------------------------------------------- | ------ | -------- |
| 8  Intel(R) Xeon(R) CPU E5-2623 v3 @ 3.00GHz | 128G   | 380G SSD |

### 软件

1.  Docker version 17.06.0-ce
2.  DockerHub Ubuntu:16.04
3.  spark-distributed-louvain-modularity <https://github.com/Sotera/spark-distributed-louvain-modularity> 2019/2/22
4.  GeminiLite 2019/2/22

## 数据来源

1.  Enron Dataset <http://snap.stanford.edu/data/email-Enron.html>
2.  Amazon Dataset <http://snap.stanford.edu/data/amazon0601.html>
3.  Youtube Dataset <http://snap.stanford.edu/data/com-Youtube.html>
4.  LiveJournal Dataset <http://snap.stanford.edu/data/com-LiveJournal.html>

## 数据规模

| 名称                      | vertex数目  | edge数目    | 文件大小   |
| ----------------------- | --------- | --------- | ------ |
| email-enron.txt         | 36,691    | 367,661   | 4MB    |
| com-youtube.ungraph.txt | 1,157,806 | 2,987,624 | 38.7MB |
| amazon0601.txt          | 403,393   | 3,387,388 | 47.9MB |
| com-lj.ungraph.txt      | 3997961   | 34681189  | 479MB  |

## 测试结果

| 名称            | enron(3W) | amazon(40W) | youtube(115W) |
| ------------- | --------- | ----------- | ------------- |
| GeminiLite(1) | 0.591     | 3.132       | 6.190         |
| GeminiLite(2) | 0.573     | 3.086       | 6.332         |
| GeminiLite(3) | 0.609     | 3.199       | 6.319         |
| dga-graphx(1) | 95        | 593         | 3671          |
| dga-graphx(2) | 100       | 592         | 3803          |
| dga-graphx(3) | 98        | 584         | 3771          |
