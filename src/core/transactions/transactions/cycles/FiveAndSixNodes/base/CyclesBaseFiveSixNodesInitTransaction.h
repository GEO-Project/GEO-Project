#ifndef GEO_NETWORK_CLIENT_CYCLESFIVESIXNODESINITTRANSACTION_H
#define GEO_NETWORK_CLIENT_CYCLESFIVESIXNODESINITTRANSACTION_H
#include "../../../base/UniqueTransaction.h"
#include "../../../../../trust_lines/manager/TrustLinesManager.h"
#include "../../../../../network/messages/cycles/InBetweenNodeTopologyMessage.h"

class CyclesBaseFiveSixNodesInitTransaction : public UniqueTransaction {

public:
    typedef shared_ptr<GetTopologyAndBalancesTransaction> Shared;
    typedef pair<vector<NodeUUID>, TrustLineBalance> MapValuesType;
    typedef multimap<NodeUUID, MapValuesType> CycleMap;
    typedef CycleMap::iterator mapIter;
    typedef vector<MapValuesType> ResultVector;
public:
    CyclesBaseFiveSixNodesInitTransaction(
        const TransactionType type,
        const NodeUUID &nodeUUID,
        TransactionsScheduler *scheduler,
        TrustLinesManager *manager,
        Logger *logger);

    CyclesBaseFiveSixNodesInitTransaction(TransactionsScheduler *scheduler);

    TransactionResult::SharedConst run_2();
    TransactionResult::SharedConst run();

    pair<BytesShared, size_t> serializeToBytes() const{};
    void deserializeFromBytes(
        BytesShared buffer){};
    virtual InBetweenNodeTopologyMessage::CycleTypeID cycleType() = 0;
protected:
    TrustLinesManager *mTrustLinesManager;
    Logger *mlogger;
    InBetweenNodeTopologyMessage::Shared mInBetweeenMessage = nullptr;
//    CycleMap mDebtors;
//    vector<pair<vector<NodeUUID>, TrustLineBalance>> mCycles;
    bool mWaitingFowAnswer = false;

protected:
    void createCyclesFromResponses();
    virtual void sendFirstLevelNodeMessage() = 0;

private:
    const uint16_t mWaitingForResponseTime = 5000; //msec
    const uint16_t kMaxRequestsCount = 5;
};

#endif //GEO_NETWORK_CLIENT_CYCLESFIVESIXNODESINITTRANSACTION_H
