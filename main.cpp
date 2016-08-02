#include <thread>
#include <iostream>
#include <stdlib.h>
#include <algorithm>

#include <jsonrpccpp/server/connectors/httpserver.h>
#include <jsonrpccpp/client/connectors/httpclient.h>

#include <cryptokernel/math.h>
#include <cryptokernel/crypto.h>

#include "wallet.h"
#include "cryptoserver.h"
#include "cryptoclient.h"

void miner(CryptoKernel::Blockchain* blockchain, CryptoCurrency::Wallet* wallet, CryptoCurrency::Protocol* protocol, CryptoKernel::Log* log)
{
    CryptoKernel::Blockchain::block Block;
    wallet->newAddress("mining");

    time_t t = std::time(0);
    uint64_t now = static_cast<uint64_t> (t);

    while(true)
    {
        Block = blockchain->generateMiningBlock(wallet->getAddressByName("mining").publicKey);
        Block.nonce = 0;

        t = std::time(0);
        now = static_cast<uint64_t> (t);

        uint64_t time2 = now;
        uint64_t count = 0;

        do
        {
            t = std::time(0);
            time2 = static_cast<uint64_t> (t);
            if((time2 - now) % 120 == 0 && (time2 - now) > 0)
            {
                std::stringstream message;
                message << "miner(): Hashrate: " << ((count / (time2 - now)) / 1000.0f) << " kH/s";
                log->printf(LOG_LEVEL_INFO, message.str());
                uint64_t nonce = Block.nonce;
                Block = blockchain->generateMiningBlock(wallet->getAddressByName("mining").publicKey);
                Block.nonce = nonce;
                now = time2;
                count = 0;
            }

            count += 1;
            Block.nonce += 1;
            Block.PoW = blockchain->calculatePoW(Block);
        }
        while(!CryptoKernel::Math::hex_greater(Block.target, Block.PoW));

        CryptoKernel::Blockchain::block previousBlock;
        previousBlock = blockchain->getBlock(Block.previousBlockId);
        std::string inverse = CryptoKernel::Math::subtractHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", Block.PoW);
        Block.totalWork = CryptoKernel::Math::addHex(inverse, previousBlock.totalWork);

        blockchain->submitBlock(Block);
        protocol->submitBlock(Block);
    }
}

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        CryptoKernel::Crypto::init();
        CryptoKernel::Log log("CryptoKernel.log", true);
        CryptoKernel::Blockchain blockchain(&log);
        CryptoCurrency::Protocol protocol(&blockchain, &log);
        CryptoCurrency::Wallet wallet(&blockchain, &protocol);
        std::thread minerThread(miner, &blockchain, &wallet, &protocol, &log);

        jsonrpc::HttpServer httpserver(8383);
        CryptoServer server(httpserver);
        server.setWallet(&wallet, &protocol, &blockchain);
        server.StartListening();

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

        server.StopListening();

        CryptoKernel::Crypto::destroy();
    }
    else
    {
        std::string command(argv[1]);
        jsonrpc::HttpClient httpclient("http://localhost:8383");
        CryptoClient client(httpclient);

        try
        {
            if(command == "getinfo")
            {
                std::cout << CryptoKernel::Storage::toString(client.getinfo()) << std::endl;
            }
            else if(command == "account")
            {
                if(argc == 3)
                {
                    std::string name(argv[2]);
                    std::cout << CryptoKernel::Storage::toString(client.account(name)) << std::endl;
                }
                else
                {
                    std::cout << "Usage: account [accountname]" << std::endl;
                }
            }
            else if(command == "sendtoaddress")
            {
                if(argc == 5)
                {
                    std::string address(argv[2]);
                    double amount(std::strtod(argv[3], NULL));
                    double fee(std::strtod(argv[4], NULL));
                    std::cout << client.sendtoaddress(address, amount, fee) << std::endl;
                }
                else
                {
                    std::cout << "Usage: sendtoaddress [address] [amount] [fee]" << std::endl;
                }
            }
        }
        catch(jsonrpc::JsonRpcException e)
        {
            std::cout << e.what() << std::endl;
        }
    }

    return 0;
}
