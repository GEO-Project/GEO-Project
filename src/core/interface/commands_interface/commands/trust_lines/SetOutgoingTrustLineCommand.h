#ifndef GEO_NETWORK_CLIENT_SETTRUSTLINECOMMAND_H
#define GEO_NETWORK_CLIENT_SETTRUSTLINECOMMAND_H

#include "../BaseUserCommand.h"

#include "../../../../common/multiprecision/MultiprecisionUtils.h"
#include "../../../../common/exceptions/ValueError.h"


using namespace std;

/**
 * This command is used to open/close/update trust line to the remote contractor node.
 *
 * To create trust line — this command must be launched against new contractor.
 * To update trust line — this command must be issued against already present contractor.
 * To remove trust line — this command must be issued with 0 amount.
 */
class SetOutgoingTrustLineCommand:
    public BaseUserCommand {

public:
    typedef shared_ptr<SetOutgoingTrustLineCommand> Shared;

public:
    SetOutgoingTrustLineCommand(
        const CommandUUID &commandUUID,
        const string &commandBuffer);

    SetOutgoingTrustLineCommand(
        BytesShared buffer);

    static const string &identifier()
        noexcept;

    const NodeUUID &contractorUUID() const
        noexcept;

    const TrustLineAmount &amount() const
        noexcept;

    pair<BytesShared, size_t> serializeToBytes();

    static const size_t kRequestedBufferSize()
        noexcept;

protected:
    void deserializeFromBytes(
        BytesShared buffer);

    [[deprecated("Remove it when parent class would be updated")]]
    void parse(
        const string &_){}

private:
    NodeUUID mContractorUUID;
    TrustLineAmount mAmount;
};


#endif //GEO_NETWORK_CLIENT_SETTRUSTLINECOMMAND_H