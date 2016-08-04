#include <sstream>
#include <iomanip>

#include "cryptoserver.h"

CryptoServer::CryptoServer(jsonrpc::AbstractServerConnector &connector) : CryptoRPCServer(connector)
{

}

void CryptoServer::setWallet(CryptoCurrency::Wallet* Wallet, CryptoCurrency::Protocol* Protocol, CryptoKernel::Blockchain* Blockchain)
{
    wallet = Wallet;
    protocol = Protocol;
    blockchain = Blockchain;
}

Json::Value CryptoServer::getinfo()
{
    Json::Value returning;

    returning["version"] = "1.0.0";
    returning["connections"] = protocol->getConnections();
    double balance = wallet->getTotalBalance() / 100000000.0;
    std::stringstream buffer;
    buffer << std::setprecision(8) << balance;
    returning["balance"] = buffer.str();
    returning["height"] = blockchain->getBlock("tip").height;

    return returning;
}

Json::Value CryptoServer::account(const std::string& account)
{
    Json::Value returning;

    wallet->newAddress(account);

    CryptoCurrency::Wallet::address newAccount = wallet->getAddressByName(account);

    returning["name"] = newAccount.name;
    double balance = newAccount.balance / 100000000.0;
    std::stringstream buffer;
    buffer << std::setprecision(8) << balance;
    returning["balance"] = buffer.str();
    returning["address"] = newAccount.publicKey;

    return returning;
}

bool CryptoServer::sendtoaddress(const std::string& address, double amount, double fee)
{
    uint64_t Amount = amount * 100000000;
    uint64_t Fee = fee * 100000000;
    return wallet->sendToAddress(address, Amount, Fee);
}
