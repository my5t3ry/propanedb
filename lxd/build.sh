#!/bin/sh

mv /app/dependencies /
rm -rf /app/*
rm -rf /app/.*
cd /app
git clone https://github.com/my5t3ry/propanedb.git
mv propanedb/* .
mv propanedb/.* .
mv /dependencies /app
mkdir -p /app/build

cd  /app/build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j12
apt-get update -y && apt-get -y --no-install-recommends install libgflags-dev  ca-certificates \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*
cd /app
mkdir /var/rocksdb && chmod 777 /var/rocksdb

cp -r  /app/dependencies/deploy/lib/libboost* /usr/local/lib/
cp -r  /app/dependencies/deploy/lib/libglog* /usr/local/lib/
cp -r  /app/dependencies/deploy/lib/libPocoFoundation* /usr/local/lib/
cp -r  /app/dependencies/deploy/lib/libPocoZip* /usr/local/lib/
cp -r  /app/build/server .
echo 'export LD_LIBRARY_PATH="/usr/lib;/usr/local/lib:${LD_LIBRARY_PATH}"' >> ~/.bashrc

chmod 777 /app/server *

systemctl daemon-reload
systemctl enable propane.service
systemctl start propane.service
