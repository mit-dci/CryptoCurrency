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
    delete it;

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

bool CryptoCurrency::Wallet::sendToAddress(std::string publicKey, double amount)
{
    std::vector<CryptoKernel::Blockchain::output> inputs;
    CryptoKernel::Storage::Iterator* it = addresses->newIterator();
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
        std::vector<CryptoKernel::Blockchain::output> tempInputs;
        tempInputs = blockchain->getUnspentOutputs(it->value()["publicKey"].asString());
        std::vector<CryptoKernel::Blockchain::output>::iterator it2;
        for(it2 = tempInputs.begin(); it2 < tempInputs.end(); it2++)
        {
            inputs.push_back(*it2);
        }
    }
    delete it;

    std::vector<CryptoKernel::Blockchain::output> toSpend;
    CryptoKernel::Crypto crypto;


    double accumulator = 0;
    std::vector<CryptoKernel::Blockchain::output>::iterator it2;
    for(it2 = inputs.begin(); it2 < inputs.end(); it2++)
    {
        if(accumulator < amount + 0.1)
        {
            address Address = getAddressByKey((*it2).publicKey);
            crypto.setPrivateKey(Address.privateKey);
            accumulator += (*it2).value;
            (*it2).signature = crypto.sign((*it2).id);
            toSpend.push_back(*it2);

            Address.balance -= (*it2).value;
            updateAddressBalance(Address.name, Address.balance);
        }
        else
        {
            break;
        }
    }

    CryptoKernel::Blockchain::output toThem;
    toThem.value = amount;
    toThem.publicKey = publicKey;

    time_t t = std::time(0);
    uint64_t now = static_cast<uint64_t> (t);
    toThem.nonce = now;
    toThem.id = blockchain->calculateOutputId(toThem);

    CryptoKernel::Blockchain::output change;
    change.value = accumulator - amount - 0.1;

    address Address = newAddress("change" + now);

    change.publicKey = Address.publicKey;
    change.nonce = now;
    change.id = blockchain->calculateOutputId(change);

    CryptoKernel::Blockchain::transaction tx;
    tx.inputs = toSpend;
    tx.outputs.push_back(toThem);
    tx.outputs.push_back(change);
    tx.timestamp = now;
    tx.id = blockchain->calculateTransactionId(tx);

    blockchain->submitTransaction(tx);

    return true;
}

int main()
{
    CryptoKernel::Blockchain blockchain;
    CryptoCurrency::Wallet wallet(&blockchain);

    std::cout << wallet.getAddressByName("test address").publicKey;

    return 0;
}
