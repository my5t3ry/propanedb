#!/bin/sh

mkdir -p /app/dependencies
ln -fs /usr/share/zoneinfo/Europe/Amsterdam /etc/localtime

sleep 5
apt-get update -y && apt-get -y --no-install-recommends install zlib1g-dev libssl-dev wget build-essential ca-certificates gcovr\
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

cd /app/dependencies
#build Boost
wget --progress=dot:giga https://boostorg.jfrog.io/artifactory/main/release/1.77.0/source/boost_1_77_0.tar.bz2  && \
    tar --bzip2 -xf boost_1_77_0.tar.bz2 && cd /app/dependencies/boost_1_77_0 && \
    ./bootstrap.sh --prefix=../deploy/ && \
    ./b2  --with-filesystem --with-system install && cd /app/dependencies/ && rm -R boost_1_77_0 && rm boost_1_77_0.tar.bz2 

apt-get update -y && apt-get -y --no-install-recommends install gpg wget git libgflags-dev  && \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main' |  tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
    apt-get update -y && apt-get install cmake -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

#build gflags
# cd /app/dependencies
# git clone https://github.com/gflags/gflags.git && \
#     cd gflags && \
#     mkdir build && \
#     cd build && \
#     cmake -DBUILD_SHARED_LIBS=1 -DGFLAGS_INSTALL_SHARED_LIBS=1 .. && \
#     make -j12 && make install && cd /app/dependencies/ && rm -R gflags   


#install gRPC
cd /app/dependencies
git clone -b v1.41.0 https://github.com/grpc/grpc && \
    cd grpc && \
    git submodule update --init && cd /app/dependencies/grpc &&\
    mkdir -p build && \
    cd build && \
    cmake .. \
    -DgRPC_BUILD_TESTS=OFF \
    -DgRPC_BUILD_CSHARP_EXT=OFF \
    -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
    -DgRPC_ZLIB_PROVIDER=package \
    -DCMAKE_INSTALL_PREFIX=../../deploy/ && \
    make -j 12 && make install && cd /app/dependencies/grpc && rm -R build

#install RocksDB
cd /app/dependencies
wget --progress=dot:giga https://github.com/facebook/rocksdb/archive/refs/tags/v6.25.3.tar.gz && \
    tar -xzf v6.25.3.tar.gz && cd rocksdb-6.25.3 && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=../../deploy/ -DCMAKE_BUILD_TYPE=Release -DROCKSDB_BUILD_SHARED=0 -DWITH_BENCHMARK_TOOLS=0 -DUSE_RTTI=1 && \
    make -j12 && make install && cd /app/dependencies && rm -R rocksdb-6.25.3 && rm v6.25.3.tar.gz

cd /app/dependencies
wget --progress=dot:giga https://github.com/google/glog/archive/refs/tags/v0.5.0.tar.gz && \
    tar -xzf v0.5.0.tar.gz && \
    cd glog-0.5.0 && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=../../deploy/ -DCMAKE_BUILD_TYPE=Release && \
    make -j12 && \
    make install

echo 'export LD_LIBRARY_PATH="/usr/lib:${LD_LIBRARY_PATH}"' >> ~/.bashrc
source "~/.bashrc"

#build PEGTL
cd /app/dependencies
wget --progress=dot:giga https://github.com/taocpp/PEGTL/archive/refs/tags/3.2.0.tar.gz && \
    tar -xzf 3.2.0.tar.gz && \
    cd PEGTL-3.2.0 && \
    mkdir build && \
    cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=../../deploy/ && \
    make -j12 && \
    make install 
#build POCO C++ libraries
cd /app/dependencies
wget --progress=dot:giga https://pocoproject.org/releases/poco-1.11.1/poco-1.11.1-all.tar.gz && \
    tar -xzf poco-1.11.1-all.tar.gz && cd poco-1.11.1-all && \
    mkdir build-cmake && cd build-cmake  && cmake .. -DCMAKE_INSTALL_PREFIX=../../deploy/ -DCMAKE_BUILD_TYPE=Release  && make -j12 install

cd /app/dependencies
wget --progress=dot:giga  https://curl.haxx.se/download/curl-7.65.3.tar.gz && \
    tar -xzf curl-7.65.3.tar.gz  && cd curl-7.65.3 && \
    ./configure --with-openssl --prefix=/app/dependencies/deploy/  && make -j8  && make install

#install clang-tidy
apt-get update -y && apt-get -y --no-install-recommends install clang-tidy\
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*


cp -r  /app/dependencies/deploy/lib/libboost* /usr/local/lib/
cp -r  /app/dependencies/deploy/lib/libglog* /usr/local/lib/
cp -r  /app/dependencies/deploy/lib/libPocoFoundation* /usr/local/lib/
cp -r  /app/dependencies/deploy/lib/libPocoZip* /usr/local/lib/
cp -r  /app/dependencies/deploy/lib/libcurl* /usr/local/lib/

cd /app
git clone https://github.com/my5t3ry/propanedb.git
mv propanedb/* .
mv propanedb/.* .

apt-get update -y && apt install openssh-server
echo 'PermitRootLogin prohibit-password' >> /etc/ssh/sshd_config
< /dev/zero ssh-keygen -q -N ""
systemctl restart sshd
