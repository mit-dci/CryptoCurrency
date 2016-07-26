#include <list>
#include <algorithm>

#include "protocol.h"

CryptoCurrency::Protocol::Protocol(CryptoKernel::Blockchain* Blockchain, CryptoKernel::Log* GlobalLog)
{
    blockchain = Blockchain;;
    log = GlobalLog;
    network = new CryptoKernel::Network(log);

    eventThread = new std::thread(&CryptoCurrency::Protocol::handleEvent, this);
}

CryptoCurrency::Protocol::~Protocol()
{
    delete eventThread;
    delete network;
    delete log;
}

void CryptoCurrency::Protocol::handleEvent()
{
    std::list<std::string> broadcastTransactions;

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
                if(blockchain->getBlock(Block.previousBlockId).id != Block.previousBlockId)
                {
                    Json::Value send;
                    send["method"] = "send";
                    send["data"] = Block.id;

                    network->sendMessage(CryptoKernel::Storage::toString(send));
                }
                else if(blockchain->submitBlock(Block))
                {
                    submitBlock(Block);
                }
            }

            else if(command["method"].asString() == "transaction")
            {
                CryptoKernel::Blockchain::transaction tx = blockchain->jsonToTransaction(command["data"]);
                if(blockchain->submitTransaction(tx) && std::find(broadcastTransactions.begin(), broadcastTransactions.end(), tx.id) != broadcastTransactions.end())
                {
                    broadcastTransactions.push_back(tx.id);
                    submitTransaction(tx);
                }
            }

            else if(command["method"].asString() == "blocks")
            {
                for(unsigned int i = 0; i < command["data"]["blocks"].size(); i++)
                {
                    CryptoKernel::Blockchain::block Block = blockchain->jsonToBlock(command["data"]["blocks"][i]);
                    blockchain->submitBlock(Block);
                }

                bool found = true;
                std::string blockId = command["data"]["tipId"].asString();
                while(found)
                {
                    CryptoKernel::Blockchain::block Block = blockchain->getBlock(blockId);
                    if(Block.id != blockId)
                    {
                        Json::Value send;
                        send["method"] = "send";
                        send["data"] = blockId;

                        network->sendMessage(CryptoKernel::Storage::toString(send));

                        found = false;
                    }

                    blockId = Block.previousBlockId;
                }
            }
            else if(command["method"].asString() == "send")
            {
                Json::Value returning;

                returning["method"] = "blocks";
                returning["data"]["tipId"] = command["data"].asString();

                std::string tipId = command["data"].asString();

                for(unsigned int i = 0; i < 200; i++)
                {
                    returning["data"]["blocks"].append(blockchain->blockToJson(blockchain->getBlock(tipId)));
                    tipId = blockchain->getBlock(tipId).previousBlockId;
                    if(tipId == "")
                    {
                        break;
                    }
                }

                network->sendMessage(CryptoKernel::Storage::toString(returning));
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
