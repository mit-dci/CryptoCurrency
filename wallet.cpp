#include <iostream>

#include <cryptokernel/crypto.h>

#include "wallet.h"

CryptoCurrency::Wallet::Wallet(CryptoKernel::Blockchain* Blockchain)
{
    blockchain = Blockchain;
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

CryptoCurrency::Wallet::address CryptoCurrency::Wallet::getAddressByName(std::string name)
{
    address Address;

    Address = jsonToAddress(addresses->get(name));

    return Address;
}

CryptoCurrency::Wallet::address CryptoCurrency::Wallet::getAddressByKey(std::string publicKey)
{
    address Address;

    CryptoKernel::Storage::Iterator* it = addresses->newIterator();
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
        if(it->value()["publicKey"] == publicKey)
        {
            Address = jsonToAddress(it->value());
            break;
        }
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

CryptoCurrency::Wallet::address CryptoCurrency::Wallet::jsonToAddress(Json::Value Address)
{
    address returning;

    returning.name = Address["name"].asString();
    returning.publicKey = Address["publicKey"].asString();
    returning.privateKey = Address["privateKey"].asString();
    returning.balance = Address["balance"].asDouble();

    return returning;
}

bool CryptoCurrency::Wallet::updateAddressBalance(std::string name, double amount)
{
    address Address;
    Address = jsonToAddress(addresses->get(name));
    if(Address.name == name)
    {
        Address.balance = amount;
        addresses->store(name, addressToJson(Address));
        return true;
    }
    else
    {
        return false;
    }
}

int main()
{
    CryptoKernel::Blockchain blockchain;
    CryptoCurrency::Wallet wallet(&blockchain);

    std::cout << wallet.getAddressByName("test address").publicKey;

    return 0;
}
