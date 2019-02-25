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

## 测试结果 (Time(s) / Modularity)

| 名称                    | enron(3W)     | amazon(40W)   | youtube(115W) | lj.ungraph(400W) |
| --------------------- | ------------- | ------------- | ------------- | ---------------- |
| GeminiLite-louvain(1) | 0.591 / 0.677 | 3.132 / 0.913 | 6.190 / 0.841 | 93 / 0.853       |
| GeminiLite-louvain(2) | 0.573 / 0.677 | 3.086 / 0.912 | 6.332 / 0.841 | 89 / 0.853       |
| GeminiLite-louvain(3) | 0.609 / 0.677 | 3.199 / 0.912 | 6.319 / 0.841 | 90 / 0.853       |
| dga-graphx(1)         | 95 / 0.544    | 593 / 0.870   | 3671 / 0.686  | ----             |
| dga-graphx(2)         | 100 / 0.544   | 592 / 0.870   | 3803 / 0.686  | ----             |
| dga-graphx(3)         | 98  / 0.544   | 584 / 0.870   | 3771 / 0.686  | ----             |

## 横向对比测试结果 (Time(s) / Modularity)

| LPA横向对比           | enron(3W)    | amazon(40W)  | youtube(115W) | lj.ungraph(400W) |
| ----------------- | ------------ | ------------ | ------------- | ---------------- |
| GeminiLite-lpa(1) | 0.08 / 0.810 | 0.95 / 0.734 | 1.01 / 0.910  | 8.14 / 0.954     |
| GeminiLite-lpa(2) | 0.09 / 0.804 | 0.93 / 0.731 | 1.04 / 0.886  | 8.33 / 0.957     |
| GeminiLite-lpa(3) | 0.08 / 0.806 | 0.92 / 0.733 | 1.01 / 0.892  | 8.25 / 0.962     |
