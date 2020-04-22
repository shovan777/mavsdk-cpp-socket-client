# mavsdk-cpp-socket-client

This is a mavsdk interface that connects Pi to pixhawk and sends data to internet via sockets.io-cpp.

## Installation

Follow the instructions on px4 to install [px4 setup](https://dev.px4.io/master/en/setup/dev_env_linux_ubuntu.html) to install all required mavsdk packages and for SITL.

## Build the code
- configure cmake
```
cmake -D SIO_DIR=<Socket.IO C++ Client dir> -D BOOST_ROOT=<Boost dir> ..
```
- build the cod
```
make
```


## Usage
```
./client
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License
[MIT](https://choosealicense.com/licenses/mit/)