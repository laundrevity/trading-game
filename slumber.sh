#!/bin/bash
echo "Port 2929" >> /etc/ssh/sshd_config
echo "PermitRootLogin yes" >> /etc/ssh/sshd_config
echo "root:toor" | chpasswd
service ssh start

conan profile new default --detect
conan profile update settings.compiler.libcxx=libstdc++11 default
echo "CC=/usr/bin/gcc-11" >> /root/.conan/profiles/default
echo "CXX=/usr/bin/g++-11" >> /root/.conan/profiles/default

while true
    do
        sleep 1
    done
