CryptoCurrency
============

CryptoCurrency in an example digital currency written in C++ using the CryptoKernel library (https://github.com/metalicjames/CryptoKernel). Eventually I aim to use this to write an algorithm for implementing monetary policy in a decentralised manner.

Building on Ubuntu 16.04
------------------------

First build and install CryptoKernel using the instructions here: https://github.com/metalicjames/CryptoKernel.

```
sudo apt-get update
sudo apt-get install libcurl4-openssl-dev libmicrohttpd-dev libjsonrpccpp-dev

git clone https://github.com/metalicjames/CryptoCurrency.git
cd CryptoCurrency
chmod +x build.sh
./build.sh
```

Usage
-----

Run CryptoCurreny to start downloading blocks, mining and listening for commands.
```
screen ./CryptoCurrency
```
Here I use screen to have it dissapear into the background.

Use "getinfo" to retrieve various information about the state of the client
```
./CryptoCurrency getinfo
{
        "balance" : "150",
        "connections" : 0,
        "height" : 4,
        "version" : "1.0.0"
}
```

Use "account" to create or retrieve information about an address
```
./CryptoCurrency account myaccount
{
        "address" : "BImZrsfI0IsKl3Cm/MikTGMfuh/m113FvuDd2sGNZzz6+Cf+oLlq5/AZhNM0K77eFHwKEdvvSUoH9F6MCRghtdc=",
        "balance" : "0",
        "name" : "myaccount"
}
```

Use "sendtoaddress" to send funds to a public key. The first operand in the address to send to, the second is the amount, and the third is the fee
```
./CryptoCurrency sendtoaddress BImZrsfI0IsKl3Cm/MikTGMfuh/m113FvuDd2sGNZzz6+Cf+oLlq5/AZhNM0K77eFHwKEdvvSUoH9F6MCRghtdc= 10 0.001
1

./CryptoCurrency account myaccount
{
        "address" : "BImZrsfI0IsKl3Cm/MikTGMfuh/m113FvuDd2sGNZzz6+Cf+oLlq5/AZhNM0K77eFHwKEdvvSUoH9F6MCRghtdc=",
        "balance" : "10",
        "name" : "myaccount"
}
```

