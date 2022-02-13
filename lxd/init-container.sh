#!/bin/sh
lxc launch ubuntu:21.04 enkidu:propanedb-remote-dev
sleep 5
lxc file push ./setup.env.sh enkidu:propanedb-remote-dev/root/
lxc file push ./build.sh enkidu:propanedb-remote-dev/root/
lxc file push ./propane.service enkidu:propanedb-remote-dev/lib/systemd/system/
lxc exec enkidu:propanedb-remote-dev /root/setup.env.sh
lxc exec enkidu:propanedb-remote-dev /root/build.sh
