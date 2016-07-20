#include <list>
#include <algorithm>

#include "protocol.h"

CryptoCurrency::Protocol::Protocol(CryptoKernel::Blockchain* Blockchain)
{
    blockchain = Blockchain;
    log = new CryptoKernel::Log();
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
                std::vector<CryptoKernel::Blockchain::block> blocks;
                for(unsigned int i = 0; i < command["data"].size(); i++)
                {
                    blocks.push_back(blockchain->jsonToBlock(command["data"][i]));
                }

                std::string firstId = "";
                std::vector<CryptoKernel::Blockchain::block>::iterator it;
                for(it = blocks.begin(); it < blocks.end(); it++)
                {
                    if(blockchain->getBlock((*it).id).id != (*it).id && blockchain->getBlock((*it).previousBlockId).id == (*it).previousBlockId)
                    {
                        firstId = (*it).id;
                    }
                }

                if(firstId != "")
                {
                    std::string nextId = firstId;
                    while(nextId != blockchain->getBlock(nextId).id)
                    {
                        for(it = blocks.begin(); it < blocks.end(); it++)
                        {
                            if((*it).id == nextId)
                            {
                                blockchain->submitBlock((*it));
                                std::vector<CryptoKernel::Blockchain::block>::iterator it2;
                                for(it2 = blocks.begin(); it2 < blocks.end(); it2++)
                                {
                                    if((*it2).previousBlockId == (*it).id)
                                    {
                                        nextId = (*it2).id;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
            else if(command["method"].asString() == "send")
            {
                std::string tipId = command["data"].asString();
                std::vector<CryptoKernel::Blockchain::block> blocks;

                while(tipId != "")
                {
                    blocks.push_back(blockchain->getBlock(tipId));
                    tipId = blockchain->getBlock(tipId).previousBlockId;
                }

                Json::Value returning;

                returning["method"] = "blocks";

                std::vector<CryptoKernel::Blockchain::block>::iterator it;
                for(it = blocks.begin(); it < blocks.end(); it++)
                {
                    returning["data"].append(blockchain->blockToJson(*it));
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
