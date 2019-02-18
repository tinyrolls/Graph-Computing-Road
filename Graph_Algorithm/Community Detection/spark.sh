# root
apt update
apt install apt-transport-https -y
mv /etc/apt/sources.list /etc/apt/sources.list.bak
echo -e "deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ bionic main restricted universe multiverse \ndeb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ bionic-updates main restricted universe multiverse \ndeb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ bionic-backports main restricted universe multiverse \ndeb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ bionic-security main restricted universe multiverse" > /etc/apt/sources.list
apt update
apt upgrade -y

# install java
apt install default-jdk -y

# install scala (version 2.11.12)
apt install scale -y


# install spark (version 2.4.0)
apt install git wget vim -y
cd
## webpage
wget http://mirrors.tuna.tsinghua.edu.cn/apache/spark/spark-2.4.0/spark-2.4.0-bin-hadoop2.7.tgz
tar xvf spark-2.4.0-bin-hadoop2.7.tgz
cd spark-2.4.0-bin-hadoop2.7
cd bin
./spark-shell

# sbt
echo "deb https://dl.bintray.com/sbt/debian /" | tee -a /etc/apt/sources.list.d/sbt.list
apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 2EE0EA64E40A89B84B2DF73499E82A75642AC823
apt update
apt install sbt

# Community Detection
cd
git clone https://github.com/zzz24512653/CommunityDetection.git
cd CommunityDetection/algorithm/graphx/Louvain
sbt # a long time

# sbt run info
sbt --warn > log.txt
grep "19/02/15" log.txt -n
cat log.txt

## fma
apt install gcc g++ make -y


## gradle
### sdk!
apt install zip unzip -y
curl -s "https://get.sdkman.io" | bash
sdk install gradle
