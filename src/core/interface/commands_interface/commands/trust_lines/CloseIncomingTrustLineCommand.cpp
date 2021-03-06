#include "CloseIncomingTrustLineCommand.h"

CloseIncomingTrustLineCommand::CloseIncomingTrustLineCommand(
    const CommandUUID &commandUUID,
    const string &command) :

    BaseUserCommand(
        commandUUID,
        identifier())
{
    auto check = [&](auto &ctx) {
        if(_attr(ctx) == kCommandsSeparator || _attr(ctx) == kTokensSeparator) {
            throw ValueError("CloseIncomingTrustLineCommand: input is empty.");
        }
    };
    auto contractorIDParse = [&](auto& ctx) {
        mContractorID = _attr(ctx);
    };
    auto equivalentParse = [&](auto& ctx) {
        mEquivalent = _attr(ctx);
    };

    try {
        parse(
            command.begin(),
            command.end(),
            char_[check]);
        parse(
            command.begin(),
            command.end(),
            *(int_[contractorIDParse]) > char_(kTokensSeparator)
            > *(int_[equivalentParse]) > eol > eoi);
    } catch(...) {
        throw ValueError("CloseIncomingTrustLineCommand: cannot parse command.");
    }
}

const string &CloseIncomingTrustLineCommand::identifier()
{
    static const string identifier = "DELETE:contractors/incoming-trust-line";
    return identifier;
}

const ContractorID CloseIncomingTrustLineCommand::contractorID() const
{
    return mContractorID;
}

const SerializedEquivalent CloseIncomingTrustLineCommand::equivalent() const
{
    return mEquivalent;
}
