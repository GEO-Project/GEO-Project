/**
 * This file is part of GEO Protocol.
 * It is subject to the license terms in the LICENSE.md file found in the top-level directory
 * of this distribution and at https://github.com/GEO-Protocol/GEO-network-client/blob/master/LICENSE.md
 *
 * No part of GEO Protocol, including this file, may be copied, modified, propagated, or distributed
 * except according to the terms contained in the LICENSE.md file.
 */

#include "TransactionResult.h"

TransactionResult::TransactionResult()
{}

TransactionResult::TransactionResult(
    TransactionState::SharedConst transactionState)
{
    setTransactionState(transactionState);
}

void TransactionResult::setCommandResult(
    CommandResult::SharedConst commandResult)
{
    mCommandResult = commandResult;
}

void TransactionResult::setTransactionState(
    TransactionState::SharedConst transactionState)
{
    mTransactionState = transactionState;
}

CommandResult::SharedConst TransactionResult::commandResult() const
{
    return mCommandResult;
}

TransactionState::SharedConst TransactionResult::state() const
{
    return mTransactionState;
}

TransactionResult::ResultType TransactionResult::resultType() const
{
    if (mCommandResult != nullptr) {
        return ResultType::CommandResultType;
    }

    return ResultType::TransactionStateType;
}
