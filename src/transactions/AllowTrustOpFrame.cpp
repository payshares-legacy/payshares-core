// Copyright 2014 Payshares Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "transactions/AllowTrustOpFrame.h"
#include "ledger/LedgerManager.h"
#include "ledger/TrustFrame.h"
#include "database/Database.h"

namespace payshares
{
AllowTrustOpFrame::AllowTrustOpFrame(Operation const& op, OperationResult& res,
                                     TransactionFrame& parentTx)
    : OperationFrame(op, res, parentTx)
    , mAllowTrust(mOperation.body.allowTrustOp())
{
}

int32_t
AllowTrustOpFrame::getNeededThreshold() const
{
    return mSourceAccount->getLowThreshold();
}

bool
AllowTrustOpFrame::doApply(LedgerDelta& delta, LedgerManager& ledgerManager)
{
    if (!(mSourceAccount->getAccount().flags & AUTH_REQUIRED_FLAG))
    { // this account doesn't require authorization to hold credit
        innerResult().code(ALLOW_TRUST_TRUST_NOT_REQUIRED);
        return false;
    }

    Currency ci;
    ci.type(ISO4217);
    ci.isoCI().currencyCode = mAllowTrust.currency.currencyCode();
    ci.isoCI().issuer = getSourceID();

    Database& db = ledgerManager.getDatabase();
    TrustFrame trustLine;
    if (!TrustFrame::loadTrustLine(mAllowTrust.trustor, ci, trustLine, db))
    {
        innerResult().code(ALLOW_TRUST_NO_TRUST_LINE);
        return false;
    }

    innerResult().code(ALLOW_TRUST_SUCCESS);

    trustLine.getTrustLine().authorized = mAllowTrust.authorize;
    trustLine.storeChange(delta, db);

    return true;
}

bool
AllowTrustOpFrame::doCheckValid(Application& app)
{
    if (mAllowTrust.currency.type() != ISO4217)
    {
        innerResult().code(ALLOW_TRUST_MALFORMED);
        return false;
    }
    return true;
}
}
