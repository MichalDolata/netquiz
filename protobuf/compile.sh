#!/bin/bash

protoc -I=./protobuf --cpp_out=./protobuf ./protobuf/*.proto
cp ./protobuf/*.{cc,h} ./server
cp ./protobuf/*.{cc,h} ./client
rm ./protobuf/*.{cc,h}