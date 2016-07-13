#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#include <cryptokernel/network.h>
#include <cryptokernel/blockchain.h>

namespace CryptoCurrency
{
    class Protocol
    {
        public:
            Protocol(CryptoKernel::Blockchain* Blockchain);
            ~Protocol();
            bool submitTransaction(CryptoKernel::Blockchain::transaction tx);
            bool submitBlock(CryptoKernel::Blockchain::block Block);

        private:
            CryptoKernel::Network* network;
            CryptoKernel::Blockchain* blockchain;
            CryptoKernel::Log* log;
            void handleEvent();
            std::thread *eventThread;
    };
}

#endif // PROTOCOL_H_INCLUDED
