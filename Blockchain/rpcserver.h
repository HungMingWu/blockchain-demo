// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPCSERVER_H
#define BITCOIN_RPCSERVER_H

#include "amount.h"
#include "rpcprotocol.h"
#include "uint256.h"

#include <list>
#include <map>
#include <stdint.h>
#include <string>

#include "wallet/wallet.h"
#include "wallet/wallet_ismine.h"
#include "claimtrie.h"
#include <stdint.h>

class CRPCCommand;
class CBlockIndex;
class CNetAddr;

class JSONRequest
{
public:
    //UniValue id;
    std::string strMethod;
    //UniValue params;

    JSONRequest() { /*id = NullUniValue;*/ }
    //void parse(const UniValue& valRequest);
};

/* Mark warmup as done.  RPC calls will be processed from now on.  */
void SetRPCWarmupFinished();

#if 0
/**
 * Type-check arguments; throws JSONRPCError if wrong type given. Does not check that
 * the right number of arguments are passed, just that any passed are the correct type.
 * Use like:  RPCTypeCheck(params, boost::assign::list_of(str_type)(int_type)(obj_type));
 */
void RPCTypeCheck(const UniValue& params,
                  const std::list<UniValue::VType>& typesExpected, bool fAllowNull=false);


/*
  Check for expected keys/value types in an Object.
  Use like: RPCTypeCheckObj(object, boost::assign::map_list_of("name", str_type)("value", int_type));
*/
void RPCTypeCheckObj(const UniValue& o,
                  const std::map<std::string, UniValue::VType>& typesExpected, bool fAllowNull=false);
#endif

/** Opaque base class for timers returned by NewTimerFunc.
 * This provides no methods at the moment, but makes sure that delete
 * cleans up the whole state.
 */
class RPCTimerBase
{
public:
    virtual ~RPCTimerBase() {}
};

/**
 * RPC timer "driver".
 */
class RPCTimerInterface
{
public:
    virtual ~RPCTimerInterface() {}
    /** Implementation name */
    virtual const char *Name() = 0;
    /** Factory function for timers.
     * RPC will call the function to create a timer that will call func in *millis* milliseconds.
     * @note As the RPC mechanism is backend-neutral, it can use different implementations of timers.
     * This is needed to cope with the case in which there is no HTTP server, but
     * only GUI RPC console, and to break the dependency of pcserver on httprpc.
     */
    virtual RPCTimerBase* NewTimer(std::function<void(void)>& func, int64_t millis) = 0;
};

/** Register factory function for timers */
void RPCRegisterTimerInterface(RPCTimerInterface *iface);
/** Unregister factory function for timers */
void RPCUnregisterTimerInterface(RPCTimerInterface *iface);

/**
 * Run func nSeconds from now.
 * Overrides previous timer <name> (if any).
 */
void RPCRunLater(const std::string& name, std::function<void(void)> func, int64_t nSeconds);

#if 0
typedef UniValue(*rpcfn_type)(const UniValue& params, bool fHelp);
#endif 

class CRPCCommand
{
public:
    std::string category;
    std::string name;
    // rpcfn_type actor;
    bool okSafeMode;
};

/**
 * Ulord RPC command dispatcher.
 */
class CRPCTable
{
private:
    std::map<std::string, const CRPCCommand*> mapCommands;
public:
    CRPCTable();
    const CRPCCommand* operator[](const std::string& name) const;
    //std::string help(const std::string& name) const;

#if 0
    /**
     * Execute a method.
     * @param method   Method to execute
     * @param params   UniValue Array of arguments (JSON objects)
     * @returns Result of the call.
     * @throws an exception (UniValue) when an error happens.
     */
    UniValue execute(const std::string &method, const UniValue &params) const;
#endif

    /**
    * Returns a list of registered commands
    * @returns List of registered commands.
    */
    std::vector<std::string> listCommands() const;
};

extern const CRPCTable tableRPC;

/**
 * Utilities: convert hex-encoded Values
 * (throws error if not hex).
 */
#if 0
extern uint256 ParseHashV(const UniValue& v, std::string strName);
extern uint256 ParseHashO(const UniValue& o, std::string strKey);
extern std::vector<unsigned char> ParseHexV(const UniValue& v, std::string strName);
extern std::vector<unsigned char> ParseHexO(const UniValue& o, std::string strKey);
#endif
extern int64_t nWalletUnlockTime;
#if 0
extern CAmount AmountFromValue(const UniValue& value);
extern UniValue ValueFromAmount(const CAmount& amount);
#endif
extern double GetDifficulty(const CBlockIndex* blockindex = NULL);
extern std::string HelpRequiringPassphrase();
extern std::string HelpExampleCli(const std::string& methodname, const std::string& args);
extern std::string HelpExampleRpc(const std::string& methodname, const std::string& args);

extern void EnsureWalletIsUnlocked();

#if 0
extern UniValue getconnectioncount(const UniValue& params, bool fHelp); // in rpcnet.cpp
extern UniValue getaddressmempool(const UniValue& params, bool fHelp);
extern UniValue getaddressutxos(const UniValue& params, bool fHelp);
extern UniValue getaddressdeltas(const UniValue& params, bool fHelp);
extern UniValue getaddresstxids(const UniValue& params, bool fHelp);
extern UniValue getaddressbalance(const UniValue& params, bool fHelp);

extern UniValue getpeerinfo(const UniValue& params, bool fHelp);
extern UniValue ping(const UniValue& params, bool fHelp);
extern UniValue addnode(const UniValue& params, bool fHelp);
extern UniValue disconnectnode(const UniValue& params, bool fHelp);
extern UniValue getaddednodeinfo(const UniValue& params, bool fHelp);
extern UniValue getnettotals(const UniValue& params, bool fHelp);
extern UniValue setban(const UniValue& params, bool fHelp);
extern UniValue listbanned(const UniValue& params, bool fHelp);
extern UniValue clearbanned(const UniValue& params, bool fHelp);

extern UniValue dumpprivkey(const UniValue& params, bool fHelp); // in rpcdump.cpp
extern UniValue importprivkey(const UniValue& params, bool fHelp);
extern UniValue importaddress(const UniValue& params, bool fHelp);
extern UniValue importpubkey(const UniValue& params, bool fHelp);
extern UniValue dumpwallet(const UniValue& params, bool fHelp);
extern UniValue importwallet(const UniValue& params, bool fHelp);

extern UniValue getgenerate(const UniValue& params, bool fHelp); // in rpcmining.cpp
extern UniValue setgenerate(const UniValue& params, bool fHelp);
extern UniValue generate(const UniValue& params, bool fHelp);
extern UniValue getnetworkhashps(const UniValue& params, bool fHelp);
extern UniValue getmininginfo(const UniValue& params, bool fHelp);
extern UniValue prioritisetransaction(const UniValue& params, bool fHelp);
extern UniValue getblocktemplate(const UniValue& params, bool fHelp);
extern UniValue submitblock(const UniValue& params, bool fHelp);
extern UniValue estimatefee(const UniValue& params, bool fHelp);
extern UniValue estimatepriority(const UniValue& params, bool fHelp);
extern UniValue estimatesmartfee(const UniValue& params, bool fHelp);
extern UniValue estimatesmartpriority(const UniValue& params, bool fHelp);

extern UniValue instantsendtoaddress(const UniValue& params, bool fHelp);
extern UniValue keepass(const UniValue& params, bool fHelp);
extern UniValue getnewaddress(const UniValue& params, bool fHelp); // in rpcwallet.cpp
extern UniValue getaccountaddress(const UniValue& params, bool fHelp);
extern UniValue getrawchangeaddress(const UniValue& params, bool fHelp);
extern UniValue setaccount(const UniValue& params, bool fHelp);
extern UniValue getaccount(const UniValue& params, bool fHelp);
extern UniValue getaddressesbyaccount(const UniValue& params, bool fHelp);
extern UniValue sendtoaddress(const UniValue& params, bool fHelp);
extern UniValue sendalltoaddress(const UniValue& params, bool fHelp);
extern UniValue uploadmessage(const UniValue &params, bool fHelp);
extern UniValue sendfromAtoB(const UniValue &params, bool fHelp);
extern UniValue sendallfromAtoB(const UniValue &params, bool fHelp);
extern UniValue signmessage(const UniValue& params, bool fHelp);
extern UniValue verifymessage(const UniValue& params, bool fHelp);
extern UniValue getreceivedbyaddress(const UniValue& params, bool fHelp);
extern UniValue getreceivedbyaccount(const UniValue& params, bool fHelp);
extern UniValue getbalance(const UniValue& params, bool fHelp);
extern UniValue getunconfirmedbalance(const UniValue& params, bool fHelp);
extern UniValue movecmd(const UniValue& params, bool fHelp);
extern UniValue sendfrom(const UniValue& params, bool fHelp);
extern UniValue sendmany(const UniValue& params, bool fHelp);
extern UniValue addmultisigaddress(const UniValue& params, bool fHelp);
extern UniValue createmultisig(const UniValue& params, bool fHelp);
extern UniValue listreceivedbyaddress(const UniValue& params, bool fHelp);
extern UniValue listreceivedbyaccount(const UniValue& params, bool fHelp);
extern UniValue listtransactions(const UniValue& params, bool fHelp);
extern UniValue listaddressgroupings(const UniValue& params, bool fHelp);
extern UniValue listaccounts(const UniValue& params, bool fHelp);
extern UniValue listsinceblock(const UniValue& params, bool fHelp);
extern UniValue gettransaction(const UniValue& params, bool fHelp);
extern UniValue abandontransaction(const UniValue& params, bool fHelp);
extern UniValue backupwallet(const UniValue& params, bool fHelp);
extern UniValue keypoolrefill(const UniValue& params, bool fHelp);
extern UniValue walletpassphrase(const UniValue& params, bool fHelp);
extern UniValue walletpassphrasechange(const UniValue& params, bool fHelp);
extern UniValue walletlock(const UniValue& params, bool fHelp);
extern UniValue encryptwallet(const UniValue& params, bool fHelp);
extern UniValue validateaddress(const UniValue& params, bool fHelp);
extern UniValue getinfo(const UniValue& params, bool fHelp);
extern UniValue debug(const UniValue& params, bool fHelp);
extern UniValue getwalletinfo(const UniValue& params, bool fHelp);
extern UniValue getblockchaininfo(const UniValue& params, bool fHelp);
extern UniValue getnetworkinfo(const UniValue& params, bool fHelp);
extern UniValue setmocktime(const UniValue& params, bool fHelp);
extern UniValue resendwallettransactions(const UniValue& params, bool fHelp);

extern UniValue getrawtransaction(const UniValue& params, bool fHelp); // in rcprawtransaction.cpp
extern UniValue listunspent(const UniValue& params, bool fHelp);
extern UniValue lockunspent(const UniValue& params, bool fHelp);
extern UniValue listlockunspent(const UniValue& params, bool fHelp);
extern UniValue createrawtransaction(const UniValue& params, bool fHelp);
extern UniValue decoderawtransaction(const UniValue& params, bool fHelp);
extern UniValue decodescript(const UniValue& params, bool fHelp);
extern UniValue fundrawtransaction(const UniValue& params, bool fHelp);
extern UniValue signrawtransaction(const UniValue& params, bool fHelp);
extern UniValue sendrawtransaction(const UniValue& params, bool fHelp);
extern UniValue gettxoutproof(const UniValue& params, bool fHelp);
extern UniValue verifytxoutproof(const UniValue& params, bool fHelp);

extern UniValue privatesend(const UniValue& params, bool fHelp);
extern UniValue getpoolinfo(const UniValue& params, bool fHelp);
extern UniValue spork(const UniValue& params, bool fHelp);
extern UniValue masternode(const UniValue& params, bool fHelp);
extern UniValue masternodelist(const UniValue& params, bool fHelp);
extern UniValue masternodebroadcast(const UniValue& params, bool fHelp);
extern UniValue signmnpmessage(const UniValue& params, bool fHelp);
extern UniValue gobject(const UniValue& params, bool fHelp);
extern UniValue getgovernanceinfo(const UniValue& params, bool fHelp);
extern UniValue getsuperblockbudget(const UniValue& params, bool fHelp);
extern UniValue voteraw(const UniValue& params, bool fHelp);
extern UniValue mnsync(const UniValue& params, bool fHelp);

extern UniValue getblockcount(const UniValue& params, bool fHelp); // in rpcblockchain.cpp
extern UniValue getsuperblock(const UniValue& params, bool fHelp);
extern UniValue getbestblockhash(const UniValue& params, bool fHelp);
extern UniValue getdifficulty(const UniValue& params, bool fHelp);
extern UniValue settxfee(const UniValue& params, bool fHelp);
extern UniValue getmempoolinfo(const UniValue& params, bool fHelp);
extern UniValue getrawmempool(const UniValue& params, bool fHelp);
extern UniValue getblockhashes(const UniValue& params, bool fHelp);
extern UniValue getblockhash(const UniValue& params, bool fHelp);
extern UniValue getblockheader(const UniValue& params, bool fHelp);
extern UniValue getblockheaders(const UniValue& params, bool fHelp);
extern UniValue getblock(const UniValue& params, bool fHelp);
extern UniValue gettxoutsetinfo(const UniValue& params, bool fHelp);
extern UniValue gettxout(const UniValue& params, bool fHelp);
extern UniValue verifychain(const UniValue& params, bool fHelp);
extern UniValue getchaintips(const UniValue& params, bool fHelp);
extern UniValue invalidateblock(const UniValue& params, bool fHelp);
extern UniValue reconsiderblock(const UniValue& params, bool fHelp);
extern UniValue getspentinfo(const UniValue& params, bool fHelp);
extern UniValue getcointip(const UniValue& params, bool fHelp);
#endif

/*claimtrie*/
#if 0
extern UniValue claimname(const UniValue& params, bool fHelp);    //in rpcwallet.cpp
extern void UpdateName(const std::vector<unsigned char>vchName, const uint160 claimId, const std::vector<unsigned char>vchValue, CAmount nAmount, CWalletTx& wtxNew, CWalletTx wtxIn, unsigned int nTxOut);
extern UniValue updateclaim( const UniValue & params, bool fHelp);
extern void CreateClaim(CScript& claimScript, CAmount nAmount, CWalletTx& wtxNew);
extern UniValue abandonclaim(const UniValue&params, bool fHelp);
extern void ListNameClaims(const CWalletTx& wtx, const std::string &strAccount, int nMinDepth, UniValue &ret, const isminefilter &filter, bool list_spent);
extern UniValue listnameclaims(const UniValue &params, bool fHelp);
extern UniValue abandonsupport(const UniValue &params, bool fHelp);
extern UniValue supportclaim(const UniValue&params, bool fHelp);
extern UniValue abandonsupport(const UniValue &params, bool fHelp);
extern UniValue sendtoaccountname(const UniValue &params, bool fHelp);
#endif

typedef std::pair<CClaimValue, std::vector<CSupportValue> > claimAndSupportsType;
typedef std::map<uint160, claimAndSupportsType> claimSupportMapType;
typedef std::map<uint160, std::vector<CSupportValue> > supportsWithoutClaimsMapType;

//claim interface command Implementation in rpcwallet.cpp
#if 0
extern UniValue getclaimsintrie(const UniValue& params, bool fHelp);   
extern UniValue getclaimtrie(const UniValue& params, bool fHelp);
extern bool getValueForClaim(const COutPoint& out, std::string& sValue);
extern UniValue getvalueforname(const UniValue& params, bool fHelp);
extern UniValue claimsAndSupportsToJSON(claimSupportMapType::const_iterator itClaimsAndSupports, int nCurrentHeight);
extern UniValue supportsWithoutClaimsToJSON(supportsWithoutClaimsMapType::const_iterator itSupportsWithoutClaims, int nCurrentHeight);
extern UniValue getclaimsforname(const UniValue& params, bool fHelp);
extern UniValue getclaimbyid(const UniValue& params, bool fHelp);
extern UniValue gettotalclaimednames(const UniValue& params, bool fHelp);
extern UniValue gettotalclaims(const UniValue& params, bool fHelp);
extern UniValue gettotalvalueofclaims(const UniValue& params, bool fHelp);
extern UniValue getclaimsfortx(const UniValue& params, bool fHelp);
extern UniValue proofToJSON(const CClaimTrieProof& proof);
extern UniValue getnameproof(const UniValue& params, bool fHelp);
#endif

// atomic swap contract of transaction about RPC 
#if 0
extern UniValue crosschaininitial(const UniValue &params, bool fHelp);
extern UniValue crosschainparticipate(const UniValue &params, bool fHelp);
extern UniValue crosschainredeem(const UniValue &params, bool fHelp);
extern UniValue crosschainrefund(const UniValue &params, bool fHelp);
extern UniValue crosschainextractsecret(const UniValue &params, bool fHelp);
extern UniValue crosschainauditcontract(const UniValue &params, bool fHelp);
#endif

bool StartRPC();
void StopRPC();
#if 0
std::string JSONRPCExecBatch(const UniValue& vReq);
#endif
#endif // BITCOIN_RPCSERVER_H
