#include "CyclesBaseFiveSixNodesInitTransaction.h"

CyclesBaseFiveSixNodesInitTransaction::CyclesBaseFiveSixNodesInitTransaction(const TransactionType type,
                                                                     const NodeUUID &nodeUUID,
                                                                     TransactionsScheduler *scheduler,
                                                                     TrustLinesManager *manager,
                                                                     Logger *logger
)
    : UniqueTransaction(type, nodeUUID, scheduler),
      mTrustLinesManager(manager),
      mlogger(logger)
{
};

CyclesBaseFiveSixNodesInitTransaction::CyclesBaseFiveSixNodesInitTransaction(TransactionsScheduler *scheduler)
    : UniqueTransaction(BaseTransaction::TransactionType::CyclesBaseFiveSixNodesInitTransaction, scheduler) {
}

TransactionResult::SharedConst CyclesBaseFiveSixNodesInitTransaction::run() {
    switch (mStep){
        case Stages::CollectDataAndSendMessage:
            return runCollectDataAndSendMessagesStage();
        case Stages::ParseMessageAndCreateCycles:
            return runParseMessageAndCreateCyclesStage();
        default:
            throw RuntimeError(
                "CyclesBaseFiveSixNodesInitTransaction::run(): "
                    "invalid transaction step.");

    }
}

TransactionResult::SharedConst CyclesBaseFiveSixNodesInitTransaction::runParseMessageAndCreateCyclesStage() {

    TrustLineBalance zeroBalance = 0;

    CycleMap mDebtors;
    vector <NodeUUID> stepPath;
    TrustLineBalance stepFlow;
    for(const auto &mess: mContext){
        auto message = static_pointer_cast<BoundaryNodeTopologyMessage>(mess);
// iF max flow less than zero than add this message to map
        stepFlow = message->maxFlow();
        if (stepFlow < zeroBalance){
            stepPath = message->Path();
            for (auto &value: message->BoundaryNodes()){
                mDebtors.insert(make_pair(
                    value.first,
                    make_pair(stepPath, min(stepFlow, value.second))));
            }
        }
    }

//    Create Cycles comparing BoundaryMessages data with debtors map
    ResultVector mCycles;
    vector <NodeUUID> stepCyclePath;
    for(const auto &mess: mContext){
        auto message = static_pointer_cast<BoundaryNodeTopologyMessage>(mess);
        stepFlow = message->maxFlow();
        if (stepFlow > zeroBalance){
            stepPath = message->Path();
            for (auto &value: message->BoundaryNodes()){
                mapIter m_it, s_it;
                pair<mapIter, mapIter> keyRange = mDebtors.equal_range(value.first);
                for (s_it = keyRange.first;  s_it != keyRange.second;  ++s_it){
                    stepCyclePath = stepPath;
                    stepCyclePath.push_back(value.first);
                    for (unsigned long i=s_it->second.first.size() -1 ; i>0; i--)
                        stepCyclePath.push_back(s_it->second.first[i]);
                    mCycles.push_back(make_pair(stepCyclePath, min(stepFlow, value.second)));
                    stepCyclePath.clear();
                }
            }
        }
    }
    mContext.clear();
//    Todo run cycles
    return finishTransaction();
}

