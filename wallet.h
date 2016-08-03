#ifndef WALLET_H_INCLUDED
#define WALLET_H_INCLUDED

#include <cryptokernel/storage.h>
#include <cryptokernel/log.h>
#include <cryptokernel/blockchain.h>

#include "protocol.h"

namespace CryptoCurrency
{
    class Wallet
    {
        public:
            Wallet(CryptoKernel::Blockchain* Blockchain, CryptoCurrency::Protocol* Protocol);
            ~Wallet();
            struct address
            {
                std::string name;
                std::string publicKey;
                std::string privateKey;
                uint64_t balance;
            };
            address newAddress(std::string name);
            address getAddressByName(std::string name);
            address getAddressByKey(std::string publicKey);
            bool updateAddressBalance(std::string name, uint64_t amount);
            bool sendToAddress(std::string publicKey, uint64_t amount, uint64_t fee);
            double getTotalBalance();
            void rescan();

        private:
            CryptoKernel::Storage* addresses;
            CryptoKernel::Log* log;
            CryptoCurrency::Protocol* protocol;
            Json::Value addressToJson(address Address);
            CryptoKernel::Blockchain* blockchain;
            address jsonToAddress(Json::Value Address);
    };
}

#endif // WALLET_H_INCLUDED
