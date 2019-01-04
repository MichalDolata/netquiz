# netquiz

To compile this project you must install protobuf runtime library.

1. Download and unpack https://github.com/protocolbuffers/protobuf/releases/download/v3.6.1/protobuf-cpp-3.6.1.tar.gz
2. Then run commands listed below (it can take a while to build)

``` 
sudo apt-get install autoconf automake libtool curl make g++ unzip
./configure
make
make check
sudo make install
sudo ldconfig
```

## Client
I recommend to build client with `QtCreator`

## Server
In order to build server application just run `make`

## Protobuf
To compile protobuf files and copy generated files just run
`./protobuf/compile.sh` from root project directory.