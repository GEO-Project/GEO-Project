#ifndef GEO_NETWORK_CLIENT_MAXFLOWCALCULATIONMESSAGE_H
#define GEO_NETWORK_CLIENT_MAXFLOWCALCULATIONMESSAGE_H

#include "../../SenderMessage.h"

#include "../../../../common/Types.h"
#include "../../../../common/NodeUUID.h"
#include "../../../../common/memory/MemoryUtils.h"

#include "../../../../transactions/transactions/base/TransactionUUID.h"

#include <memory>
#include <utility>
#include <stdint.h>

using namespace std;

class MaxFlowCalculationMessage : public SenderMessage {
public:
    typedef shared_ptr<MaxFlowCalculationMessage> Shared;

public:
    MaxFlowCalculationMessage(
        const SerializedEquivalent equivalent,
        const NodeUUID &senderUUID,
        ContractorID idOnReceiverSide,
        vector<BaseAddress::Shared> targetAddresses);

    MaxFlowCalculationMessage(
        BytesShared buffer);

    vector<BaseAddress::Shared> targetAddresses() const;

    virtual pair<BytesShared, size_t> serializeToBytes() const
        throw(bad_alloc);

    const size_t kOffsetToInheritedBytes() const
        noexcept;

protected:
    vector<BaseAddress::Shared> mTargetAddresses;
};

#endif //GEO_NETWORK_CLIENT_MAXFLOWCALCULATIONMESSAGE_H
