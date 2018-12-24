#ifndef GEO_NETWORK_CLIENT_BASECOLLECTTOPOLOGYTRANSACTION_H
#define GEO_NETWORK_CLIENT_BASECOLLECTTOPOLOGYTRANSACTION_H

#include "BaseTransaction.h"

#include "../../../trust_lines/manager/TrustLinesManager.h"
#include "../../../topology/manager/TopologyTrustLinesManager.h"
#include "../../../topology/cashe/TopologyCacheManager.h"
#include "../../../topology/cashe/MaxFlowCacheManager.h"

#include "../../../network/messages/max_flow_calculation/ResultMaxFlowCalculationMessage.h"
#include "../../../network/messages/max_flow_calculation/ResultMaxFlowCalculationGatewayMessage.h"

class BaseCollectTopologyTransaction : public BaseTransaction {

public:
    typedef shared_ptr<BaseCollectTopologyTransaction> Shared;

public:
    BaseCollectTopologyTransaction(
        const TransactionType type,
        const SerializedEquivalent equivalent,
        ContractorsManager *contractorsManager,
        TrustLinesManager *trustLinesManager,
        TopologyTrustLinesManager *topologyTrustLineManager,
        TopologyCacheManager *topologyCacheManager,
        MaxFlowCacheManager *maxFlowCacheManager,
        Logger &logger);

    BaseCollectTopologyTransaction(
        const TransactionType type,
        const TransactionUUID &transactionUUID,
        const SerializedEquivalent equivalent,
        ContractorsManager *contractorsManager,
        TrustLinesManager *trustLinesManager,
        TopologyTrustLinesManager *topologyTrustLineManager,
        TopologyCacheManager *topologyCacheManager,
        MaxFlowCacheManager *maxFlowCacheManager,
        Logger &logger);

    TransactionResult::SharedConst run();

protected:
    enum Stages {
        SendRequestForCollectingTopology = 1,
        ProcessCollectingTopology = 2,
        CustomLogic = 3
    };

protected:
    virtual TransactionResult::SharedConst sendRequestForCollectingTopology() = 0;

    virtual TransactionResult::SharedConst processCollectingTopology() = 0;

    virtual TransactionResult::SharedConst applyCustomLogic(){};

    void fillTopology();

protected:
    const uint16_t kFinalStep = 10;

protected:
    ContractorsManager *mContractorsManager;
    TrustLinesManager *mTrustLinesManager;
    TopologyTrustLinesManager *mTopologyTrustLineManager;
    TopologyCacheManager *mTopologyCacheManager;
    MaxFlowCacheManager *mMaxFlowCacheManager;
    set<ContractorID> mGateways;
};


#endif //GEO_NETWORK_CLIENT_BASECOLLECTTOPOLOGYTRANSACTION_H
