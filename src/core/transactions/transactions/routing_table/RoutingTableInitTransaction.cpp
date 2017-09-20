#include "RoutingTableInitTransaction.h"
#include "../../../network/messages/routing_table/RoutingTableRequestMessage.h"
#include "../../../network/messages/routing_table/RoutingTableResponseMessage.h"

RoutingTableInitTransaction::RoutingTableInitTransaction(
    const NodeUUID &nodeUUID,
    TrustLinesManager *trustlineManager,
    RoutingTableManager *routingTableManager,
    Logger &logger):
    BaseTransaction(
    BaseTransaction::TransactionType::RoutingTableInitTransactionType,
    nodeUUID,
    logger),
    mTrustlineManager(trustlineManager),
    mRoutingTableManager(routingTableManager),
    mLog(logger)
{

}

TransactionResult::SharedConst RoutingTableInitTransaction::run() {
    while (true) {
        debug() << "run: stage: " << mStep;
        try {
            switch (mStep) {
                case Stages::CollectDataStage:
                    return runCollectDataStage();

                case Stages::UpdateRoutingTableStage:
                    return runUpdateRoutingTableStage();

                default:
                    throw RuntimeError(
                        "RoutingTableInitTransaction::run(): "
                            "invalid transaction step.");
            }
        } catch(std::exception &e){
            error() << "Something happens wrong in method run(). Transaction will be dropped; " << e.what();
            return resultDone();
        }
    }
}

TransactionResult::SharedConst RoutingTableInitTransaction::runCollectDataStage() {
    auto neighbors = mTrustlineManager->rt1();
    for(const auto kNeighborNode: neighbors){
        sendMessage<RoutingTableRequestMessage>(
            kNeighborNode,
            currentNodeUUID()
        );
    }
    return resultAwaikAfterMilliseconds(mkWaitingForResponseTime);
}

const string RoutingTableInitTransaction::logHeader() const {
    stringstream s;
    s << "[RoutingTableInitTA: " << currentTransactionUUID() << "] ";
    return s.str();
}

TransactionResult::SharedConst RoutingTableInitTransaction::runUpdateRoutingTableStage()
{
    if (mContext.size() != 0){
        info() << "No responses from neighbors. RoutingTable will not be updated." << endl;
        return resultDone();
    }
    mRoutingTableManager->clearMap();

    for(auto stepMessage: mContext){
        auto message = static_pointer_cast<RoutingTableResponseMessage>(stepMessage);
        if(!mTrustlineManager->isNeighbor(message->senderUUID)){
            continue;
        }
        mRoutingTableManager->updateMapAddSeveralNeighbors(message->senderUUID, message->neighbors());
    }
    return resultDone();
}
