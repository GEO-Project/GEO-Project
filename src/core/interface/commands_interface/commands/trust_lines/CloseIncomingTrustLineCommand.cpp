#include "CloseIncomingTrustLineCommand.h"

CloseIncomingTrustLineCommand::CloseIncomingTrustLineCommand(
    const CommandUUID &commandUUID,
    const string &command) :

    BaseUserCommand(
        commandUUID,
        identifier())
{
    auto check = [&](auto &ctx) {
        if(_attr(ctx) == kCommandsSeparator) {
            throw ValueError("CloseIncomingTrustLineCommand: there is no input ");
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
            *(int_[contractorIDParse]) > char_(kTokensSeparator) > *(int_[equivalentParse]) > eol);
    } catch(...) {
        throw ValueError("CloseIncomingTrustLineCommand: can't parse command.");
    }
}

const string &CloseIncomingTrustLineCommand::identifier()
    noexcept
{
    static const string identifier = "DELETE:contractors/incoming-trust-line";
    return identifier;
}

const ContractorID CloseIncomingTrustLineCommand::contractorID() const
    noexcept
{
    return mContractorID;
}

const SerializedEquivalent CloseIncomingTrustLineCommand::equivalent() const
    noexcept
{
    return mEquivalent;
}
