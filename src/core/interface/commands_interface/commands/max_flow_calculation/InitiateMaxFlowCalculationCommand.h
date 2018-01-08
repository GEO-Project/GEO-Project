#ifndef GEO_NETWORK_CLIENT_INITIATEMAXFLOWCALCULATIONCOMMAND_H
#define GEO_NETWORK_CLIENT_INITIATEMAXFLOWCALCULATIONCOMMAND_H


#include "../BaseUserCommand.h"
#include "../../../../common/exceptions/ValueError.h"

class InitiateMaxFlowCalculationCommand : public BaseUserCommand {

public:
    typedef shared_ptr<InitiateMaxFlowCalculationCommand> Shared;

public:
    InitiateMaxFlowCalculationCommand(
        const CommandUUID &uuid,
        const string &command);

    InitiateMaxFlowCalculationCommand(
        BytesShared buffer);

    static const string &identifier();

    const vector<NodeUUID> &contractors() const;

    CommandResult::SharedConst responseOk(
        string &maxFlowAmount) const;

    [[deprecated("Remove it when parent class would be updated")]]
    void parse(
        const string &_){}

private:
    size_t mContractorsCount;
    vector<NodeUUID> mContractors;
};


#endif //GEO_NETWORK_CLIENT_INITIATEMAXFLOWCALCULATIONCOMMAND_H