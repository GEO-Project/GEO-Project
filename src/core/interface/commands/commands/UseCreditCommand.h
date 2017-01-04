#ifndef GEO_NETWORK_CLIENT_USECREDITCOMMAND_H
#define GEO_NETWORK_CLIENT_USECREDITCOMMAND_H

#include "BaseUserCommand.h"
#include "../../../trust_lines/TrustLine.h"
#include "../../../common/exceptions/ValueError.h"


class UseCreditCommand : public BaseUserCommand {

public:
    typedef shared_ptr<UseCreditCommand> Shared;

protected:
    NodeUUID mContractorUUID;
    TrustLineAmount mAmount;
    string mPurpose;

public:
    UseCreditCommand(
        const CommandUUID &uuid,
        const string &commandBuffer);

    static const string &identifier();

    const NodeUUID &contractorUUID() const;

    const TrustLineAmount &amount() const;

    const string &purpose() const;

    const CommandResult *resultOk(
            TransactionUUID &transactionUUID,
            uint16_t resultCode,
            Timestamp &transactionReceived,
            Timestamp &transactionProceed) const;

    const CommandResult *notEnoughtCreditAmountResult() const;

protected:
    void deserialize(
        const string &command);
};

#endif //GEO_NETWORK_CLIENT_USECREDITCOMMAND_H
