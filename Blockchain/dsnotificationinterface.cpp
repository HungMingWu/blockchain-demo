// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dsnotificationinterface.h"
#include "privsend.h"
#include "instantx.h"
#include "governance.h"
#include "masternodeman.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "observer_ptr.h"

CDSNotificationInterface::CDSNotificationInterface()
{
}

CDSNotificationInterface::~CDSNotificationInterface()
{
}

void CDSNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindex)
{
	auto index = nonstd::make_observer(pindex);
    mnodeman.UpdatedBlockTip(index);
    privSendPool.UpdatedBlockTip(index);
    instantsend.UpdatedBlockTip(index);
    mnpayments.UpdatedBlockTip(index);
    governance.UpdatedBlockTip(index);
    masternodeSync.UpdatedBlockTip(index);
}

void CDSNotificationInterface::SyncTransaction(const CTransaction &tx, const CBlock *pblock)
{
    instantsend.SyncTransaction(tx, pblock);
}