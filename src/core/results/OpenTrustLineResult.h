#ifndef GEO_NETWORK_CLIENT_OPENTRUSTLINERESULT_H
#define GEO_NETWORK_CLIENT_OPENTRUSTLINERESULT_H

#include "Result.h"

class OpenTrustLineResult : public Result {
private:
    NodeUUID mContractorUUID;
    trust_amount mAmount;

public:
    OpenTrustLineResult(BaseUserCommand *command,
                        const uint16_t &resultCode,
                        const string &timestampCompleted,
                        const NodeUUID &contractorUUID,
                        const trust_amount &amount);

    const uuids::uuid &commandUUID() const;

    const string &id() const;

    const uint16_t &resultCode() const;

    const string &timestampExcepted() const;

    const string &timestampCompleted() const;

    const NodeUUID &contractorUUID() const;

    const trust_amount &amount() const;

    string serialize();
};

#endif //GEO_NETWORK_CLIENT_OPENTRUSTLINERESULT_H
