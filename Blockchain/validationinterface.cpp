// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "validationinterface.h"
#include "primitives/transaction.h"

static CMainSignals g_signals;

CMainSignals& GetMainSignals()
{
    return g_signals;
}

void RegisterValidationInterface(CValidationInterface* pwalletIn) {
    g_signals.UpdatedBlockTipConn = g_signals.UpdatedBlockTip.connect([pwalletIn](auto&&... params) {
        return pwalletIn->UpdatedBlockTip(std::forward<decltype(params)>(params)...);
    });
    g_signals.SyncTransactionConn = g_signals.SyncTransaction.connect([pwalletIn](auto&&... params) {
        return pwalletIn->SyncTransaction(std::forward<decltype(params)>(params)...);
    });
    g_signals.NotifyTransactionLockConn = g_signals.NotifyTransactionLock.connect([pwalletIn](auto&&... params) {
        return pwalletIn->NotifyTransactionLock(std::forward<decltype(params)>(params)...);
    });
    g_signals.UpdatedTransactionConn = g_signals.UpdatedTransaction.connect([pwalletIn](auto&&... params) {
        return pwalletIn->UpdatedTransaction(std::forward<decltype(params)>(params)...);
    });
    g_signals.SetBestChainConn = g_signals.SetBestChain.connect([pwalletIn](auto&&... params) {
        return pwalletIn->SetBestChain(std::forward<decltype(params)>(params)...);
    });
    g_signals.InventoryConn = g_signals.Inventory.connect([pwalletIn](auto&&... params) {
        return pwalletIn->Inventory(std::forward<decltype(params)>(params)...);
    });
    g_signals.BroadcastConn = g_signals.Broadcast.connect([pwalletIn](auto&&... params) {
        return pwalletIn->ResendWalletTransactions(std::forward<decltype(params)>(params)...);
    });
    g_signals.BlockCheckedConn = g_signals.BlockChecked.connect([pwalletIn](auto&&... params) {
        return pwalletIn->BlockChecked(std::forward<decltype(params)>(params)...);
    });
    g_signals.ScriptForMiningConn = g_signals.ScriptForMining.connect([pwalletIn](auto&&... params) {
        return pwalletIn->GetScriptForMining(std::forward<decltype(params)>(params)...);
    });
    g_signals.BlockFoundConn = g_signals.BlockFound.connect([pwalletIn](auto&&... params) {
        return pwalletIn->ResetRequestCount(std::forward<decltype(params)>(params)...);
    });
}

void UnregisterValidationInterface(CValidationInterface* pwalletIn) {
    g_signals.BlockFoundConn.disconnect();
    g_signals.ScriptForMiningConn.disconnect();
    g_signals.BlockCheckedConn.disconnect();
    g_signals.BroadcastConn.disconnect();
    g_signals.InventoryConn.disconnect();
    g_signals.SetBestChainConn.disconnect();
    g_signals.UpdatedTransactionConn.disconnect();
    g_signals.NotifyTransactionLockConn.disconnect();
    g_signals.SyncTransactionConn.disconnect();
    g_signals.UpdatedBlockTipConn.disconnect();
}

void UnregisterAllValidationInterfaces() {
    g_signals.BlockFound.disconnect_all_slots();
    g_signals.ScriptForMining.disconnect_all_slots();
    g_signals.BlockChecked.disconnect_all_slots();
    g_signals.Broadcast.disconnect_all_slots();
    g_signals.Inventory.disconnect_all_slots();
    g_signals.SetBestChain.disconnect_all_slots();
    g_signals.UpdatedTransaction.disconnect_all_slots();
    g_signals.NotifyTransactionLock.disconnect_all_slots();
    g_signals.SyncTransaction.disconnect_all_slots();
    g_signals.UpdatedBlockTip.disconnect_all_slots();
}

void SyncWithWallets(const CTransaction &tx, const CBlock *pblock) {
    g_signals.SyncTransaction(tx, pblock);
}
