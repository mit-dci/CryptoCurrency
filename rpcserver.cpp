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

    returning["version"] = "0.0.1";
    returning["connections"] = protocol->getConnections();
    returning["balance"] = wallet->getTotalBalance();
    returning["height"] = blockchain->getBlock("tip").height;

    return returning;
}

Json::Value CryptoServer::account(const std::string& account)
{
    Json::Value returning;

    wallet->newAddress(account);

    CryptoCurrency::Wallet::address newAccount = wallet->getAddressByName(account);

    returning["name"] = newAccount.name;
    returning["balance"] = newAccount.balance;
    returning["address"] = newAccount.publicKey;

    return returning;
}

bool CryptoServer::sendtoaddress(const std::string& address, double amount, double fee)
{
    return wallet->sendToAddress(address, amount, fee);
}
