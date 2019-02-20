# docker basic
## apt fast
apt update
apt install apt-transport-https -y
mv /etc/apt/sources.list /etc/apt/sources.list.bak
echo -e "deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ bionic main restricted universe multiverse \ndeb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ bionic-updates main restricted universe multiverse \ndeb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ bionic-backports main restricted universe multiverse \ndeb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ bionic-security main restricted universe multiverse" > /etc/apt/sources.list
apt update
apt upgrade -y

## install java
apt install default-jdk -y

## install scala (version 2.11.12)
apt install scale -y


## install spark (version 2.4.0)
apt install git wget vim -y
cd $HOME
wget http://mirrors.tuna.tsinghua.edu.cn/apache/spark/spark-2.4.0/spark-2.4.0-bin-hadoop2.7.tgz
tar xvf spark-2.4.0-bin-hadoop2.7.tgz

## sbt
echo "deb https://dl.bintray.com/sbt/debian /" | tee -a /etc/apt/sources.list.d/sbt.list
apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 2EE0EA64E40A89B84B2DF73499E82A75642AC823
apt update
apt install sbt

# sbt run info
sbt --warn > log.txt
grep "19/02/15" log.txt -n
cat log.txt


# Community Detection
cd $HOME
git clone https://github.com/Sotera/spark-distributed-louvain-modularity.git
cd spark-distributed-louvain-modularity/dga-graphx

## gradle
### sdk!
apt install zip unzip curl -y
curl -s "https://get.sdkman.io" | bash
source "/root/.sdkman/bin/sdkman-init.sh"
sdk install gradle

### java 8
apt install openjdk-8-jdk -y
update-alternatives --config java # choose 8
update-alternatives --config javac # choose 8

### gradle run info
gradle clean
gradle build

# fma
apt install gcc g++ make -y
git clone https://github.com/fma-cloud/GeminiLite.git
cd GeminiLite
git submodule init
git submodule update
make

# docker additional
docker exec -it 4a7afcdeb729 bash


### run instructure
 bin/louvain -i /root/data/LiveJ/LiveJournal_new.txt -o /root/graphx_result/ -m local[8] --edgedelimiter "\t" 2>/root/graphx_LiveJournal_log.txt

bin/louvain -i /root/data/twitter/twitter.txt -o /root/graphx_result/ -m local[8] --edgedelimiter " " 2>/root/graphx_twitter_log.txt
