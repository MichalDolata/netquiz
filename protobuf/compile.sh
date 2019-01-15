#!/bin/bash

protoc -I=./protobuf --cpp_out=./protobuf ./protobuf/*.proto
