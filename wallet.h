#ifndef WALLET_H_INCLUDED
#define WALLET_H_INCLUDED

#include <cryptokernel/storage.h>
#include <cryptokernel/log.h>

namespace CryptoCurrency
{
    class Wallet
    {
        public:
            Wallet();
            ~Wallet();
            struct address
            {
                std::string name;
                std::string publicKey;
                std::string privateKey;
                double balance;
            };
            address newAddress(std::string name);

        private:
            CryptoKernel::Storage* addresses;
            CryptoKernel::Log* log;
            Json::Value addressToJson(address Address);
    };
}

#endif // WALLET_H_INCLUDED
