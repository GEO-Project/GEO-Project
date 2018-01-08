#include "HistoryPaymentsCommand.h"

HistoryPaymentsCommand::HistoryPaymentsCommand(
    const CommandUUID &uuid,
    const string &commandBuffer):

    BaseUserCommand(
        uuid,
        identifier())
{
    parse(commandBuffer);
}

const string &HistoryPaymentsCommand::identifier()
{
    static const string identifier = "GET:history/payments";
    return identifier;
}

/**
 * Throws ValueError if deserialization was unsuccessful.
 */
void HistoryPaymentsCommand::parse(
        const string &command)
{
    const auto minCommandLength = 17;
    if (command.size() < minCommandLength) {
        throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Received command is to short.");
    }
    size_t tokenSeparatorPos = command.find(
        kTokensSeparator);
    string historyFromStr = command.substr(
        0,
        tokenSeparatorPos);
    if (historyFromStr.at(0) == '-') {
        throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. 'from' token can't be negative.");
    }
    try {
        mHistoryFrom = std::stoul(historyFromStr);
    } catch (...) {
        throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Error occurred while parsing  'from' token.");
    }

    size_t nextTokenSeparatorPos = command.find(
        kTokensSeparator,
        tokenSeparatorPos + 1);
    string historyCountStr = command.substr(
        tokenSeparatorPos + 1,
        nextTokenSeparatorPos - tokenSeparatorPos - 1);
    if (historyCountStr.at(0) == '-') {
        throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. 'count' token can't be negative.");
    }
    try {
        mHistoryCount = std::stoul(historyCountStr);
    } catch (...) {
        throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Error occurred while parsing 'count' token.");
    }

    tokenSeparatorPos = nextTokenSeparatorPos;
    nextTokenSeparatorPos = command.find(
        kTokensSeparator,
        tokenSeparatorPos + 1);
    string timeFromStr = command.substr(
        tokenSeparatorPos + 1,
        nextTokenSeparatorPos - tokenSeparatorPos - 1);
    if (timeFromStr == kNullParameter) {
        mIsTimeFromPresent = false;
    } else {
        mIsTimeFromPresent = true;
        try {
            int64_t timeFrom = std::stoul(timeFromStr);
            mTimeFrom = pt::time_from_string("1970-01-01 00:00:00.000");
            mTimeFrom += pt::microseconds(timeFrom);
        } catch (...) {
            throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Error occurred while parsing 'timeFrom' token.");
        }
    }

    tokenSeparatorPos = nextTokenSeparatorPos;
    nextTokenSeparatorPos = command.find(
        kTokensSeparator,
        tokenSeparatorPos + 1);
    string timeToStr = command.substr(
        tokenSeparatorPos + 1,
        nextTokenSeparatorPos - tokenSeparatorPos - 1);
    if (timeToStr == kNullParameter) {
        mIsTimeToPresent = false;
    } else {
        mIsTimeToPresent = true;
        try {
            int64_t timeTo = std::stoul(timeToStr);
            mTimeTo = pt::time_from_string("1970-01-01 00:00:00.000");
            mTimeTo += pt::microseconds(timeTo);
        } catch (...) {
            throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Error occurred while parsing 'timeTo' token.");
        }
    }

    tokenSeparatorPos = nextTokenSeparatorPos;
    nextTokenSeparatorPos = command.find(
        kTokensSeparator,
        tokenSeparatorPos + 1);
    string lowBoundaryAmountStr = command.substr(
        tokenSeparatorPos + 1,
        nextTokenSeparatorPos - tokenSeparatorPos - 1);
    if (lowBoundaryAmountStr == kNullParameter) {
        mIsLowBoundartAmountPresent = false;
    } else {
        mIsLowBoundartAmountPresent = true;
        try {
            mLowBoundaryAmount = TrustLineAmount(
                lowBoundaryAmountStr);
        } catch (...) {
            throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Error occurred while parsing 'lowBoundaryAmount' token.");
        }
    }

    tokenSeparatorPos = nextTokenSeparatorPos;
    nextTokenSeparatorPos = command.find(
        kTokensSeparator,
        tokenSeparatorPos + 1);
    string highBoundaryAmountStr = command.substr(
        tokenSeparatorPos + 1,
        nextTokenSeparatorPos - tokenSeparatorPos - 1);
    if (highBoundaryAmountStr == kNullParameter) {
        mIsHighBoundaryAmountPresent = false;
    } else {
        mIsHighBoundaryAmountPresent = true;
        try {
            mHighBoundaryAmount = TrustLineAmount(
                highBoundaryAmountStr);
        } catch (...) {
            throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Error occurred while parsing 'mHighBoundaryAmount' token.");
        }
    }

    tokenSeparatorPos = nextTokenSeparatorPos;
    nextTokenSeparatorPos = command.size() - 1;
    string paymentRecordCommandUUIDStr = command.substr(
        tokenSeparatorPos + 1,
        nextTokenSeparatorPos - tokenSeparatorPos - 1);
    if (paymentRecordCommandUUIDStr == kNullParameter) {
        mIsPaymentRecordCommandUUIDPresent = false;
    } else {
        mIsPaymentRecordCommandUUIDPresent = true;
        try {
            mPaymentRecordCommandUUID = boost::lexical_cast<uuids::uuid>(paymentRecordCommandUUIDStr);
        } catch (...) {
            throw ValueError("HistoryPaymentsCommand::parse: "
                                 "Can't parse command. Error occurred while parsing 'mPaymentRecordCommandUUID' token.");
        }
    }
}

const size_t HistoryPaymentsCommand::historyFrom() const
{
    return mHistoryFrom;
}

const size_t HistoryPaymentsCommand::historyCount() const
{
    return mHistoryCount;
}

const DateTime HistoryPaymentsCommand::timeFrom() const
{
    return mTimeFrom;
}

const DateTime HistoryPaymentsCommand::timeTo() const
{
    return mTimeTo;
}

const bool HistoryPaymentsCommand::isTimeFromPresent() const
{
    return mIsTimeFromPresent;
}

const bool HistoryPaymentsCommand::isTimeToPresent() const
{
    return mIsTimeToPresent;
}

const TrustLineAmount& HistoryPaymentsCommand::lowBoundaryAmount() const
{
    return mLowBoundaryAmount;
}

const TrustLineAmount& HistoryPaymentsCommand::highBoundaryAmount() const
{
    return mHighBoundaryAmount;
}

const bool HistoryPaymentsCommand::isLowBoundaryAmountPresent() const
{
    return mIsLowBoundartAmountPresent;
}

const bool HistoryPaymentsCommand::isHighBoundaryAmountPresent() const
{
    return mIsHighBoundaryAmountPresent;
}

const CommandUUID& HistoryPaymentsCommand::paymentRecordCommandUUID() const
{
    return mPaymentRecordCommandUUID;
}

const bool HistoryPaymentsCommand::isPaymentRecordCommandUUIDPresent() const
{
    return mIsPaymentRecordCommandUUIDPresent;
}

CommandResult::SharedConst HistoryPaymentsCommand::resultOk(string &historyPaymentsStr) const
{
    return CommandResult::SharedConst(
        new CommandResult(
            identifier(),
            UUID(),
            200,
            historyPaymentsStr));
}