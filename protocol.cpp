#include "protocol.h"

CryptoCurrency::Protocol::Protocol(CryptoKernel::Blockchain* Blockchain)
{
    blockchain = Blockchain;
    log = new CryptoKernel::Log();
    network = new CryptoKernel::Network(log);

    eventThread = new std::thread(&handleEvent, this);
}

CryptoCurrency::Protocol::~Protocol()
{
    delete eventThread;
    delete network;
    delete log;
}

void CryptoCurrency::Protocol::handleEvent()
{
    while(true)
    {
        std::string message = network->popMessage();

        if(message == "")
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        else
        {
            Json::Value command = CryptoKernel::Storage::toJson(message);
            if(command["method"].asString() == "block")
            {
                CryptoKernel::Blockchain::block Block = blockchain->jsonToBlock(command["data"]);
                if(blockchain->submitBlock(Block))
                {
                    submitBlock(Block);
                }
            }

            else if(command["method"].asString() == "transaction")
            {
                CryptoKernel::Blockchain::transaction tx = blockchain->jsonToTransaction(command["data"]);
                if(blockchain->submitTransaction(tx))
                {
                    submitTransaction(tx);
                }
            }
        }
    }
}

bool CryptoCurrency::Protocol::submitBlock(CryptoKernel::Blockchain::block Block)
{
    Json::Value data;
    data["method"] = "block";
    data["data"] = blockchain->blockToJson(Block);
    if(network->sendMessage(CryptoKernel::Storage::toString(data)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CryptoCurrency::Protocol::submitTransaction(CryptoKernel::Blockchain::transaction tx)
{
    Json::Value data;
    data["method"] = "transaction";
    data["data"] = blockchain->transactionToJson(tx);
    if(network->sendMessage(CryptoKernel::Storage::toString(data)))
    {
        return true;
    }
    else
    {
        return false;
    }
}
