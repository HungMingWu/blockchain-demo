// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "masternode.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "protocol.h"
#include "masternodeconfig.h"
#include "Log.h"


// Keep track of the active Masternode
CActiveMasternode activeMasternode;

void CActiveMasternode::ManageState()
{
    LOG_INFO("CActiveMasternode::ManageState -- Start\n");
    if (!fMasterNode) {
		LOG_INFO("CActiveMasternode::ManageState -- Not a masternode, returning\n");
        return;
    }

    if (Params().NetworkIDString() != CBaseChainParams::REGTEST && !masternodeSync.IsBlockchainSynced()) {
        nState = ACTIVE_MASTERNODE_SYNC_IN_PROCESS;
		LOG_INFO("CActiveMasternode::ManageState -- {}: {}", GetStateString(), GetStatus());
        return;
    }

    if (nState == ACTIVE_MASTERNODE_SYNC_IN_PROCESS) {
        nState = ACTIVE_MASTERNODE_INITIAL;
    }

	LOG_INFO("CActiveMasternode::ManageState -- status = {}, type = {}, pinger enabled = {}", GetStatus(), GetTypeString(), fPingerEnabled);

    if (eType == MASTERNODE_UNKNOWN) {
        ManageStateInitial();
    }

    if (eType == MASTERNODE_REMOTE) {
        ManageStateRemote();
    } else if(eType == MASTERNODE_LOCAL) {
        // Try Remote Start first so the started local masternode can be restarted without recreate masternode broadcast.
        ManageStateRemote();
#ifdef ENABLE_WALLET
        if(nState != ACTIVE_MASTERNODE_STARTED)
            ManageStateLocal();
#endif // ENABLE_WALLET
    }

    SendMasternodePing();
}

std::string CActiveMasternode::GetStateString() const
{
    switch (nState) {
        case ACTIVE_MASTERNODE_INITIAL:         return "INITIAL";
        case ACTIVE_MASTERNODE_SYNC_IN_PROCESS: return "SYNC_IN_PROCESS";
        case ACTIVE_MASTERNODE_INPUT_TOO_NEW:   return "INPUT_TOO_NEW";
        case ACTIVE_MASTERNODE_NOT_CAPABLE:     return "NOT_CAPABLE";
        case ACTIVE_MASTERNODE_STARTED:         return "STARTED";
        default:                                return "UNKNOWN";
    }
}

std::string CActiveMasternode::GetStatus() const
{
    switch (nState) {
        case ACTIVE_MASTERNODE_INITIAL:         return "Node just started, not yet activated";
        case ACTIVE_MASTERNODE_SYNC_IN_PROCESS: return "Sync in progress. Must wait until sync is complete to start Masternode";
        case ACTIVE_MASTERNODE_INPUT_TOO_NEW:   return fmt::format("Masternode input must have at least %d confirmations", Params().GetConsensus().nMasternodeMinimumConfirmations);
        case ACTIVE_MASTERNODE_NOT_CAPABLE:     return "Not capable masternode: " + strNotCapableReason;
        case ACTIVE_MASTERNODE_STARTED:         return "Masternode successfully started";
        default:                                return "Unknown";
    }
}

std::string CActiveMasternode::GetTypeString() const
{
    std::string strType;
    switch(eType) {
    case MASTERNODE_UNKNOWN:
        strType = "UNKNOWN";
        break;
    case MASTERNODE_REMOTE:
        strType = "REMOTE";
        break;
    case MASTERNODE_LOCAL:
        strType = "LOCAL";
        break;
    default:
        strType = "UNKNOWN";
        break;
    }
    return strType;
}

bool CActiveMasternode::SendMasternodePing()
{
    if (!fPingerEnabled) {
		LOG_INFO("CActiveMasternode::SendMasternodePing -- {}: masternode ping service is disabled, skipping...", GetStateString());
        return false;
    }

    if(!mnodeman.Has(vin)) {
        strNotCapableReason = "Masternode not in masternode list";
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
		LOG_INFO("CActiveMasternode::SendMasternodePing -- {}: {}", GetStateString(), strNotCapableReason);
        return false;
    }

    CMasternodePing mnp(vin);
    if(!mnp.Sign(keyMasternode, pubKeyMasternode)) {
		LOG_INFO("CActiveMasternode::SendMasternodePing -- ERROR: Couldn't sign Masternode Ping");
        return false;
    }

    // Update lastPing for our masternode in Masternode list
    if(mnodeman.IsMasternodePingedWithin(vin, MASTERNODE_MIN_MNP_SECONDS, mnp.sigTime)) {
		LOG_INFO("CActiveMasternode::SendMasternodePing -- Too early to send Masternode Ping");
        return false;
    }

    mnodeman.SetMasternodeLastPing(vin, mnp);

	LOG_INFO("CActiveMasternode::SendMasternodePing -- Relaying ping({}-{}), collateral={}", mnp.certifyPeriod, mnp.certifyVersion, vin.ToString());
    mnp.Relay();

    return true;
}

void CActiveMasternode::ManageStateInitial()
{
	LOG_INFO("CActiveMasternode::ManageStateInitial -- status = {}, type = {}, pinger enabled = {}", GetStatus(), GetTypeString(), fPingerEnabled);

    // Check that our local network configuration is correct
    if (!fListen) {
        // listen option is probably overwritten by smth else, no good
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = "Masternode must accept connections from outside. Make sure listen configuration option is not overwritten by some another parameter.";
		LOG_INFO("CActiveMasternode::ManageStateInitial -- {}: {}\n", GetStateString(), strNotCapableReason);
        return;
    }

    bool fFoundLocal = false;
    {
        LOCK(cs_vNodes);

        // First try to find whatever local address is specified by externalip option
        fFoundLocal = GetLocal(service) && CMasternode::IsValidNetAddr(service);
        if(!fFoundLocal) {
            // nothing and no live connections, can't do anything for now
            if (vNodes.empty()) {            
				nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
                strNotCapableReason = "Can't detect valid external address. Will retry when there are some connections available.";
				LOG_INFO("CActiveMasternode::ManageStateInitial -- {}: {}\n", GetStateString(), strNotCapableReason);
                return;
            }
            // We have some peers, let's try to find our local address from one of them
            for (CNode* pnode : vNodes) {
				LOG_INFO("fSuccessfullyConnected = {}, addr.IsIPv4 = {}", pnode->fSuccessfullyConnected ? 'y' : 'n', pnode->addr.IsIPv4() ? 'y' : 'n' );
			    if (pnode->fSuccessfullyConnected && pnode->addr.IsIPv4()) {
                    fFoundLocal = GetLocal(service, &pnode->addr) && CMasternode::IsValidNetAddr(service);
					LOG_INFO("GetLocal() = {}, IsValidNetAddr = {}", GetLocal(service, &pnode->addr), CMasternode::IsValidNetAddr(service));
                    if(fFoundLocal) break;
                }
            }
        }
    }

    if(!fFoundLocal) {
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = "Can't detect valid external address. Please consider using the externalip configuration option if problem persists. Make sure to use IPv4 address only.";
		LOG_INFO("CActiveMasternode::ManageStateInitial -- {}: {}", GetStateString(), strNotCapableReason);
        return;
    }

    int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
    if(Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if(service.GetPort() != mainnetDefaultPort) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = fmt::format("Invalid port: %u - only %d is supported on mainnet.", service.GetPort(), mainnetDefaultPort);
			LOG_INFO("CActiveMasternode::ManageStateInitial -- {}: {}", GetStateString(), strNotCapableReason);
            return;
        }
    } else if(service.GetPort() == mainnetDefaultPort) {
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = fmt::format("Invalid port: %u - %d is only supported on mainnet.", service.GetPort(), mainnetDefaultPort);
		LOG_INFO("CActiveMasternode::ManageStateInitial -- {}: {}", GetStateString(), strNotCapableReason);
        return;
    }

	LOG_INFO("CActiveMasternode::ManageStateInitial -- Checking inbound connection to '{}'", service.ToString());

    if(!ConnectNode((CAddress)service, NULL, true)) {
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = "Could not connect to " + service.ToString();
		LOG_INFO("CActiveMasternode::ManageStateInitial -- {}: {}\n", GetStateString(), strNotCapableReason);
        return;
    }

    // Default to REMOTE
    eType = MASTERNODE_REMOTE;
#if 0
#ifdef ENABLE_WALLET
    const CAmount ct = Params().GetConsensus().colleteral;
    // Check if wallet funds are available
    if(!pwalletMain) {
		LOG_INFO("CActiveMasternode::ManageStateInitial -- {}: Wallet not available\n", GetStateString());
        return;
    }

    if(pwalletMain->IsLocked()) {
		LOG_INFO("CActiveMasternode::ManageStateInitial -- {}: Wallet is locked\n", GetStateString());
        return;
    }

    if(pwalletMain->GetBalance() < ct) {
		LOG_INFO("CActiveMasternode::ManageStateInitial -- {}: Wallet balance is < %lld UT\n", GetStateString(), ct);
        return;
    }

    // Choose coins to use
    CPubKey pubKeyCollateral;
    CKey keyCollateral;

    // If collateral is found switch to LOCAL mode
    if(pwalletMain->GetMasternodeVinAndKeys(vin, pubKeyCollateral, keyCollateral)) {
        eType = MASTERNODE_LOCAL;
    }
#endif // ENABLE_WALLET
#endif

	if(masternodeConfig.GetMasternodeVin(vin))
	{
		eType = MASTERNODE_LOCAL;
	}

	LOG_INFO("CActiveMasternode::ManageStateInitial -- End status = {}, type = {}, pinger enabled = {}", GetStatus(), GetTypeString(), fPingerEnabled);
}

void CActiveMasternode::ManageStateRemote()
{
    LOG_INFO("CActiveMasternode::ManageStateRemote -- Start status = %s, type = %s, pinger enabled = %d, pubKeyMasternode.GetID() = %s\n", 
             GetStatus(), fPingerEnabled, GetTypeString(), pubKeyMasternode.GetID().ToString());

    mnodeman.CheckMasternode(pubKeyMasternode);
    boost::optional<masternode_info_t> infoMn = mnodeman.GetMasternodeInfo(pubKeyMasternode);
    if (infoMn) {
        if (infoMn->nProtocolVersion != PROTOCOL_VERSION) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Invalid protocol version";
			LOG_INFO("CActiveMasternode::ManageStateRemote -- {}: {}", GetStateString(), strNotCapableReason);
            return;
        }
        if (service != infoMn->addr) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Specified IP doesn't match our external address.";
			LOG_INFO("CActiveMasternode::ManageStateRemote -- {}: {}", GetStateString(), strNotCapableReason);
            return;
        }
        if (vin != infoMn->vin) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Specified collateraloutputtxid doesn't match our external vin.";
			LOG_INFO("CActiveMasternode::ManageStateRemote -- {}: {}\n", GetStateString(), strNotCapableReason);
            return;
        }
        if (!CMasternode::IsValidStateForAutoStart(infoMn->nActiveState)) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = fmt::format("Masternode in %s state", CMasternode::StateToString(infoMn->nActiveState));
			LOG_INFO("CActiveMasternode::ManageStateRemote -- {}: {}\n", GetStateString(), strNotCapableReason);
            return;
        }
        if (nState != ACTIVE_MASTERNODE_STARTED) {
			LOG_INFO("CActiveMasternode::ManageStateRemote -- STARTED!\n");
            vin = infoMn->vin;
            service = infoMn->addr;
            fPingerEnabled = true;
            nState = ACTIVE_MASTERNODE_STARTED;
        }
    }
    else {
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = "Masternode not in masternode list";
		LOG_INFO("CActiveMasternode::ManageStateRemote -- {}: {}", GetStateString(), strNotCapableReason);
    }
}

#ifdef ENABLE_WALLET
void CActiveMasternode::ManageStateLocal()
{
    LOG_INFO("CActiveMasternode::ManageStateLocal -- status = %s, type = %s, pinger enabled = %d\n", GetStatus(), GetTypeString(), fPingerEnabled);
    if(nState == ACTIVE_MASTERNODE_STARTED) {
        return;
    }

    // Choose coins to use

    if(masternodeConfig.GetMasternodeVin(vin)) {
        int nInputAge = GetInputAge(vin);
        if(nInputAge < Params().GetConsensus().nMasternodeMinimumConfirmations){
            nState = ACTIVE_MASTERNODE_INPUT_TOO_NEW;
            strNotCapableReason = fmt::format("%s - %d confirmations"), GetStatus(, nInputAge);
			LOG_INFO("CActiveMasternode::ManageStateLocal -- {}: {}", GetStateString(), strNotCapableReason);
            return;
        }

        auto ret = CMasternodeBroadcast::Create(vin, service,  keyMasternode, pubKeyMasternode);
        if (ret.has_error()) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Error creating mastenode broadcast: " + ret.error().message();
			LOG_INFO("CActiveMasternode::ManageStateLocal -- {}: {}", GetStateString(), strNotCapableReason);
            return;
        }

        auto mnb = ret.value();
        if(!CBitcoinAddress().Set(mnb.GetPayeeDestination())) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Error Collateral transaction without change address, can't design a payee address!";
			LOG_INFO("CActiveMasternode::ManageStateLocal -- {}: {}", GetStateString(), strNotCapableReason);
            return;
        }

        // check if it is registered on the Ulord center server
        //CMasternode mn(mnb);
        if(!mnodecenter.LoadLicense(mnb))
        {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = fmt::format("%s didn't registered on Ulord Center", mnb.vin.prevout.ToStringShort());
			LOG_INFO("CMasternodeBroadcast::ManageStateLocal -- Didn't registered on Ulord Center, masternode={}", mnb.vin.prevout.ToStringShort());
            return;
        }
        //mnb.certifyPeriod = mn.certifyPeriod;
        //mnb.certificate = mn.certificate;
		LOG_INFO("CActiveMasternode::ManageStateLocal -- Load License({}-{})", mnb.certifyPeriod, mnb.certifyVersion);
		
        fPingerEnabled = true;
        nState = ACTIVE_MASTERNODE_STARTED;

        //update to masternode list
		LOG_INFO("CActiveMasternode::ManageStateLocal -- Update Masternode List");
        mnodeman.UpdateMasternodeList(mnb);
        mnodeman.NotifyMasternodeUpdates();

        //send to all peers
		LOG_INFO("CActiveMasternode::ManageStateLocal -- Relay broadcast, vin={}", vin.ToString());
        mnb.Relay();
    }
}
#endif
