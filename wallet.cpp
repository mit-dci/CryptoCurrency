#include <iostream>

#include <cryptokernel/crypto.h>

#include "wallet.h"

CryptoCurrency::Wallet::Wallet()
{
    log = new CryptoKernel::Log();
    addresses = new CryptoKernel::Storage("./addressesdb");
}

CryptoCurrency::Wallet::~Wallet()
{
    delete log;
    delete addresses;
}

CryptoCurrency::Wallet::address CryptoCurrency::Wallet::newAddress(std::string name)
{
    address Address;
    if(addresses->get(name)["name"] != name)
    {
        CryptoKernel::Crypto crypto(true);

        Address.name = name;
        Address.publicKey = crypto.getPublicKey();
        Address.privateKey = crypto.getPrivateKey();
        Address.balance = 0;

        addresses->store(name, addressToJson(Address));
    }

    return Address;
}

Json::Value CryptoCurrency::Wallet::addressToJson(address Address)
{
    Json::Value returning;

    returning["name"] = Address.name;
    returning["publicKey"] = Address.publicKey;
    returning["privateKey"] = Address.privateKey;
    returning["balance"] = Address.balance;

    return returning;
}

int main()
{
    CryptoCurrency::Wallet wallet;

    return 0;
}
