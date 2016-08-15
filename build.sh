#!/bin/sh

exe() { echo "$@" ; "$@" ; }

CXX=g++
CXXFLAGS="-Wall -std=c++14 -O2"

exe ${CXX} ${CXXFLAGS} -c main.cpp -o main.o
exe ${CXX} ${CXXFLAGS} -c protocol.cpp -o protocol.o
exe ${CXX} ${CXXFLAGS} -c wallet.cpp -o wallet.o
exe ${CXX} ${CXXFLAGS} -c rpcserver.cpp -o rpcserver.o
exe ${CXX} main.o protocol.o wallet.o rpcserver.o -o CryptoCurrency -lCryptoKernel -ljsoncpp -lcrypto -lleveldb -lenet -ljsonrpccpp