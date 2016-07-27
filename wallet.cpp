#include <iostream>
#include <thread>
#include <random>

#include <cryptokernel/crypto.h>

#include "wallet.h"

CryptoCurrency::Wallet::Wallet(CryptoKernel::Blockchain* Blockchain, CryptoCurrency::Protocol* Protocol)
{
    protocol = Protocol;
    blockchain = Blockchain;
    log = new CryptoKernel::Log();
    addresses = new CryptoKernel::Storage("./addressesdb");

    rescan();
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

bool CryptoCurrency::Wallet::sendToAddress(std::string publicKey, double amount, double fee)
{
    if(getTotalBalance() < amount + fee)
    {
        return false;
    }

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

    double accumulator = 0;
    std::vector<CryptoKernel::Blockchain::output>::iterator it2;
    for(it2 = inputs.begin(); it2 < inputs.end(); it2++)
    {
        if(accumulator < amount + fee)
        {
            address Address = getAddressByKey((*it2).publicKey);
            toSpend.push_back(*it2);
            Address.balance -= (*it2).value;
            updateAddressBalance(Address.name, Address.balance);
            accumulator += (*it2).value;
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
    change.value = accumulator - amount - fee;

    std::stringstream buffer;
    buffer << now << "_change";
    address Address = newAddress(buffer.str());

    change.publicKey = Address.publicKey;
    change.nonce = now;
    change.id = blockchain->calculateOutputId(change);

    CryptoKernel::Crypto crypto;

    std::vector<CryptoKernel::Blockchain::output> outputs;
    outputs.push_back(change);
    outputs.push_back(toThem);
    std::string outputHash = blockchain->calculateOutputSetId(outputs);

    for(it2 = toSpend.begin(); it2 < toSpend.end(); it2++)
    {
        address Address = getAddressByKey((*it2).publicKey);
        crypto.setPrivateKey(Address.privateKey);
        std::string signature = crypto.sign((*it2).id + outputHash);
        (*it2).signature = signature;
    }

    CryptoKernel::Blockchain::transaction tx;
    tx.inputs = toSpend;
    tx.outputs.push_back(toThem);
    tx.outputs.push_back(change);
    tx.timestamp = now;
    tx.id = blockchain->calculateTransactionId(tx);

    blockchain->submitTransaction(tx);

    protocol->submitTransaction(tx);

    return true;
}

double CryptoCurrency::Wallet::getTotalBalance()
{
    double balance = 0;

    CryptoKernel::Storage::Iterator* it = addresses->newIterator();
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
        balance += it->value()["balance"].asDouble();
    }
    delete it;

    return balance;
}

void CryptoCurrency::Wallet::rescan()
{
    std::vector<address> tempAddresses;
    CryptoKernel::Storage::Iterator* it = addresses->newIterator();
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
        address Address;
        Address = jsonToAddress(it->value());
        Address.balance = blockchain->getBalance(Address.publicKey);
        tempAddresses.push_back(Address);
    }
    delete it;

    std::vector<address>::iterator it2;
    for(it2 = tempAddresses.begin(); it2 < tempAddresses.end(); it2++)
    {
        updateAddressBalance((*it2).name, blockchain->getBalance((*it2).publicKey));
    }
}

void miner(CryptoKernel::Blockchain* blockchain, CryptoCurrency::Wallet* wallet, CryptoCurrency::Protocol* protocol, CryptoKernel::Log* log)
{
    CryptoKernel::Blockchain::block Block;
    wallet->newAddress("mining");

    time_t t = std::time(0);
    uint64_t now = static_cast<uint64_t> (t);

    std::stringstream buffer;
    buffer << now << "_mining";

    std::default_random_engine generator(now);
    std::uniform_int_distribution<unsigned int> distribution(0, wallet->getTotalBalance() - 1);

    wallet->newAddress(buffer.str());
    std::string publicKey = wallet->getAddressByName(buffer.str()).publicKey;
    wallet->sendToAddress(publicKey, distribution(generator), 0.1);

    while(true)
    {
        Block = blockchain->generateMiningBlock(wallet->getAddressByName("mining").publicKey);
        Block.nonce = 0;

        t = std::time(0);
        now = static_cast<uint64_t> (t);

        uint64_t time2 = now;

        do
        {
            t = std::time(0);
            time2 = static_cast<uint64_t> (t);
            if((time2 - now) % 120 == 0 && (time2 - now) > 0)
            {
                std::stringstream message;
                message << "miner(): Hashrate: " << ((Block.nonce / (time2 - now)) / 1000.0f) << " kH/s";
                log->printf(LOG_LEVEL_INFO, message.str());
                Block = blockchain->generateMiningBlock(wallet->getAddressByName("mining").publicKey);
                Block.nonce = 0;
                now = time2;
            }

            Block.nonce += 1;
            Block.PoW = blockchain->calculatePoW(Block);
        }
        while(!hex_greater(Block.target, Block.PoW));

        CryptoKernel::Blockchain::block previousBlock;
        previousBlock = blockchain->getBlock(Block.previousBlockId);
        std::string inverse = subtractHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", Block.PoW);
        Block.totalWork = addHex(inverse, previousBlock.totalWork);

        blockchain->submitBlock(Block);
        protocol->submitBlock(Block);

        t = std::time(0);
        now = static_cast<uint64_t> (t);

        buffer.clear();
        buffer << now << "_mining";

        wallet->newAddress(buffer.str());
        std::string publicKey = wallet->getAddressByName(buffer.str()).publicKey;
        wallet->sendToAddress(publicKey, distribution(generator), 0.1);
    }
}

int main()
{
    CryptoKernel::Crypto::init();
    CryptoKernel::Log log("CryptoKernel.log", true);
    CryptoKernel::Blockchain blockchain(&log);
    CryptoCurrency::Protocol protocol(&blockchain, &log);
    CryptoCurrency::Wallet wallet(&blockchain, &protocol);
    std::thread minerThread(miner, &blockchain, &wallet, &protocol, &log);

    std::string tipId = blockchain.getBlock("tip").id;

    while(true)
    {
        protocol.submitBlock(blockchain.getBlock("tip"));
        std::vector<CryptoKernel::Blockchain::transaction> unconfirmedTransactions = blockchain.getUnconfirmedTransactions();
        std::vector<CryptoKernel::Blockchain::transaction>::iterator it;
        for(it = unconfirmedTransactions.begin(); it < unconfirmedTransactions.end(); it++)
        {
            protocol.submitTransaction(*it);
        }

        if(tipId != blockchain.getBlock("tip").id)
        {
            wallet.rescan();
            tipId = blockchain.getBlock("tip").id;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(120000));
    }

    CryptoKernel::Crypto::destroy();

    return 0;
}
