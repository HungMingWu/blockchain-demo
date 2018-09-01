// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>
#include <catch2/catch.hpp>
#include "wallet/wallet.h"
#include "wallet/walletdb.h"

extern CWallet* pwalletMain;

static void
GetResults(CWalletDB& walletdb, std::map<CAmount, CAccountingEntry>& results)
{
	std::list<CAccountingEntry> aes;

	results.clear();
	REQUIRE(walletdb.ReorderTransactions(pwalletMain) == DB_LOAD_OK);
	walletdb.ListAccountCreditDebit("", aes);
	for (CAccountingEntry& ae : aes)
	{
		results[ae.nOrderPos] = ae;
	}
}

TEST_CASE("acc_orderupgrade")
{
	CWalletDB walletdb(pwalletMain->strWalletFile);
	std::vector<CWalletTx*> vpwtx;
	CWalletTx wtx;
	CAccountingEntry ae;
	std::map<CAmount, CAccountingEntry> results;

	LOCK(pwalletMain->cs_wallet);

	ae.strAccount = "";
	ae.nCreditDebit = 1;
	ae.nTime = 1333333333;
	ae.strOtherAccount = "b";
	ae.strComment = "";
	pwalletMain->AddAccountingEntry(ae, walletdb);

	wtx.mapValue["comment"] = "z";
	pwalletMain->AddToWallet(wtx, false, &walletdb);
	vpwtx.push_back(&pwalletMain->mapWallet[wtx.GetHash()]);
	vpwtx[0]->nTimeReceived = (unsigned int)1333333335;
	vpwtx[0]->nOrderPos = -1;

	ae.nTime = 1333333336;
	ae.strOtherAccount = "c";
	pwalletMain->AddAccountingEntry(ae, walletdb);

	GetResults(walletdb, results);

	REQUIRE(pwalletMain->nOrderPosNext == 3);
	REQUIRE(2 == results.size());
	REQUIRE(results[0].nTime == 1333333333);
	REQUIRE(results[0].strComment.empty());
	REQUIRE(1 == vpwtx[0]->nOrderPos);
	REQUIRE(results[2].nTime == 1333333336);
	REQUIRE(results[2].strOtherAccount == "c");


	ae.nTime = 1333333330;
	ae.strOtherAccount = "d";
	ae.nOrderPos = pwalletMain->IncOrderPosNext();
	pwalletMain->AddAccountingEntry(ae, walletdb);

	GetResults(walletdb, results);

	REQUIRE(results.size() == 3);
	REQUIRE(pwalletMain->nOrderPosNext == 4);
	REQUIRE(results[0].nTime == 1333333333);
	REQUIRE(1 == vpwtx[0]->nOrderPos);
	REQUIRE(results[2].nTime == 1333333336);
	REQUIRE(results[3].nTime == 1333333330);
	REQUIRE(results[3].strComment.empty());


	wtx.mapValue["comment"] = "y";
	{
		CMutableTransaction tx(wtx);
		--tx.nLockTime;  // Just to change the hash :)
		*static_cast<CTransaction*>(&wtx) = CTransaction(tx);
	}
	pwalletMain->AddToWallet(wtx, false, &walletdb);
	vpwtx.push_back(&pwalletMain->mapWallet[wtx.GetHash()]);
	vpwtx[1]->nTimeReceived = (unsigned int)1333333336;

	wtx.mapValue["comment"] = "x";
	{
		CMutableTransaction tx(wtx);
		--tx.nLockTime;  // Just to change the hash :)
		*static_cast<CTransaction*>(&wtx) = CTransaction(tx);
	}
	pwalletMain->AddToWallet(wtx, false, &walletdb);
	vpwtx.push_back(&pwalletMain->mapWallet[wtx.GetHash()]);
	vpwtx[2]->nTimeReceived = (unsigned int)1333333329;
	vpwtx[2]->nOrderPos = -1;

	GetResults(walletdb, results);

	REQUIRE(results.size() == 3);
	REQUIRE(pwalletMain->nOrderPosNext == 6);
	REQUIRE(0 == vpwtx[2]->nOrderPos);
	REQUIRE(results[1].nTime == 1333333333);
	REQUIRE(2 == vpwtx[0]->nOrderPos);
	REQUIRE(results[3].nTime == 1333333336);
	REQUIRE(results[4].nTime == 1333333330);
	REQUIRE(results[4].strComment.empty());
	REQUIRE(5 == vpwtx[1]->nOrderPos);


	ae.nTime = 1333333334;
	ae.strOtherAccount = "e";
	ae.nOrderPos = -1;
	pwalletMain->AddAccountingEntry(ae, walletdb);

	GetResults(walletdb, results);

	REQUIRE(results.size() == 4);
	REQUIRE(pwalletMain->nOrderPosNext == 7);
	REQUIRE(0 == vpwtx[2]->nOrderPos);
	REQUIRE(results[1].nTime == 1333333333);
	REQUIRE(2 == vpwtx[0]->nOrderPos);
	REQUIRE(results[3].nTime == 1333333336);
	REQUIRE(results[3].strComment.empty());
	REQUIRE(results[4].nTime == 1333333330);
	REQUIRE(results[4].strComment.empty());
	REQUIRE(results[5].nTime == 1333333334);
	REQUIRE(6 == vpwtx[1]->nOrderPos);
}
