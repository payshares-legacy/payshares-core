// Copyright 2014 Payshares Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "Peer.h"

#include "crypto/Hex.h"
#include "crypto/SHA.h"
#include "database/Database.h"
#include "generated/PaysharesXDR.h"
#include "herder/Herder.h"
#include "herder/TxSetFrame.h"
#include "main/Application.h"
#include "main/Config.h"
#include "overlay/OverlayManager.h"
#include "overlay/PeerRecord.h"
#include "util/Logging.h"

#include "xdrpp/marshal.h"

#include <soci.h>
#include <time.h>

// LATER: need to add some way of docking peers that are misbehaving by sending
// you bad data

namespace payshares
{

using namespace std;
using namespace soci;

Peer::Peer(Application& app, PeerRole role)
    : mApp(app)
    , mRole(role)
    , mState(role == ACCEPTOR ? CONNECTING : CONNECTED)
    , mRemoteProtocolVersion(0)
    , mRemoteListeningPort(0)
{
}

void
Peer::sendHello()
{
    CLOG(DEBUG,"Overlay") << "Peer::sendHello to " << toString();

    PaysharesMessage msg;
    msg.type(HELLO);
    msg.hello().protocolVersion = mApp.getConfig().PROTOCOL_VERSION;
    msg.hello().versionStr = mApp.getConfig().VERSION_STR;
    msg.hello().listeningPort = mApp.getConfig().PEER_PORT;
    msg.hello().peerID = mApp.getConfig().PEER_PUBLIC_KEY;

    sendMessage(msg);
}

std::string
Peer::toString()
{
    std::stringstream s;
    s << getIP() << ":" << mRemoteListeningPort;
    return s.str();
}

void
Peer::connectHandler(asio::error_code const& error)
{
    if (error)
    {
        CLOG(WARNING, "Overlay")
            << " connectHandler error: " << error.message();
        drop();
    }
    else
    {
        CLOG(DEBUG, "Overlay") << "connected @" << toString();
        connected();
        mState = CONNECTED;
        sendHello();
    }
}

void
Peer::sendDontHave(MessageType type, uint256 const& itemID)
{
    PaysharesMessage msg;
    msg.type(DONT_HAVE);
    msg.dontHave().reqHash = itemID;
    msg.dontHave().type = type;

    sendMessage(msg);
}

void
Peer::sendSCPQuorumSet(SCPQuorumSet const & qSet)
{
    PaysharesMessage msg;
    msg.type(SCP_QUORUMSET);
    msg.qSet() = qSet;

    sendMessage(msg);
}
void
Peer::sendGetTxSet(uint256 const& setID)
{
    PaysharesMessage newMsg;
    newMsg.type(GET_TX_SET);
    newMsg.txSetHash() = setID;

    sendMessage(newMsg);
}
void
Peer::sendGetQuorumSet(uint256 const& setID)
{
    CLOG(TRACE, "Overlay") << "Get quorum set: " << hexAbbrev(setID);

    PaysharesMessage newMsg;
    newMsg.type(GET_SCP_QUORUMSET);
    newMsg.qSetHash() = setID;

    sendMessage(newMsg);
}

void
Peer::sendPeers()
{
    // send top 50 peers we know about
    vector<PeerRecord> peerList;
    PeerRecord::loadPeerRecords(mApp.getDatabase(), 50, mApp.getClock().now(),
                                peerList);
    PaysharesMessage newMsg;
    newMsg.type(PEERS);
    newMsg.peers().resize(xdr::size32(peerList.size()));
    for (size_t n = 0; n < peerList.size(); n++)
    {
        if (!peerList[n].isPrivateAddress())
        {
            peerList[n].toXdr(newMsg.peers()[n]);
        }
    }
    sendMessage(newMsg);
}

void
Peer::sendMessage(PaysharesMessage const& msg)
{
    CLOG(TRACE, "Overlay") << "("
                           << binToHex(mApp.getConfig().PEER_PUBLIC_KEY)
                                  .substr(0, 6) << ")send: " << msg.type()
                           << " to : " << hexAbbrev(mPeerID);
    xdr::msg_ptr xdrBytes(xdr::xdr_to_msg(msg));
    this->sendMessage(std::move(xdrBytes));
}

void
Peer::recvMessage(xdr::msg_ptr const& msg)
{
    CLOG(TRACE, "Overlay") << "received xdr::msg_ptr";
    PaysharesMessage sm;
    xdr::xdr_from_msg(msg, sm);
    recvMessage(sm);
}

void
Peer::recvMessage(PaysharesMessage const& paysharesMsg)
{
    CLOG(TRACE, "Overlay") << "("
                           << binToHex(mApp.getConfig().PEER_PUBLIC_KEY)
                                  .substr(0, 6)
                           << ")recv: " << paysharesMsg.type()
                           << " from:" << hexAbbrev(mPeerID);

    if (mState < GOT_HELLO &&
        ((paysharesMsg.type() != HELLO) && (paysharesMsg.type() != PEERS)))
    {
        CLOG(WARNING, "Overlay") << "recv: " << paysharesMsg.type()
                                 << " before hello";
        drop();
        return;
    }

    switch (paysharesMsg.type())
    {
    case ERROR_MSG:
    {
        recvError(paysharesMsg);
    }
    break;

    case HELLO:
    {
        this->recvHello(paysharesMsg);
    }
    break;

    case DONT_HAVE:
    {
        recvDontHave(paysharesMsg);
    }
    break;

    case GET_PEERS:
    {
        recvGetPeers(paysharesMsg);
    }
    break;

    case PEERS:
    {
        recvPeers(paysharesMsg);
    }
    break;

    case GET_TX_SET:
    {
        recvGetTxSet(paysharesMsg);
    }
    break;

    case TX_SET:
    {
        recvTxSet(paysharesMsg);
    }
    break;

    case TRANSACTION:
    {
        recvTransaction(paysharesMsg);
    }
    break;

    case GET_SCP_QUORUMSET:
    {
        recvGetSCPQuorumSet(paysharesMsg);
    }
    break;

    case SCP_QUORUMSET:
    {
        recvSCPQuorumSet(paysharesMsg);
    }
    break;

    case SCP_MESSAGE:
    {
        recvSCPMessage(paysharesMsg);
    }
    break;
    }
}

void
Peer::recvDontHave(PaysharesMessage const& msg)
{
    switch (msg.dontHave().type)
    {
    case TX_SET:
        mApp.getOverlayManager().getTxSetFetcher().doesntHave(msg.dontHave().reqHash, 
                                                              shared_from_this());
        break;
    case SCP_QUORUMSET:
        mApp.getOverlayManager().getQuorumSetFetcher().doesntHave(msg.dontHave().reqHash,
                                                                  shared_from_this());
        break;
    default:
        break;
    }
}

void
Peer::recvGetTxSet(PaysharesMessage const& msg)
{
    auto self = shared_from_this();
    if (auto txSet = mApp.getOverlayManager().getTxSetFetcher().get(msg.txSetHash()))
    {
        PaysharesMessage newMsg;
        newMsg.type(TX_SET);
        txSet->toXDR(newMsg.txSet());

        self->sendMessage(newMsg);
    } else
    {
        sendDontHave(TX_SET, msg.txSetHash());
    }
}

void
Peer::recvTxSet(PaysharesMessage const& msg)
{
    auto hash = TxSetFrame(msg.txSet()).getContentsHash();
    mApp.getOverlayManager().getTxSetFetcher().recv(hash, msg.txSet());
}

void
Peer::recvTransaction(PaysharesMessage const& msg)
{
    TransactionFramePtr transaction =
        TransactionFrame::makeTransactionFromWire(msg.transaction());
    if (transaction)
    {
        // add it to our current set
        // and make sure it is valid
        if (mApp.getHerder().recvTransaction(transaction))
        {
            mApp.getOverlayManager().recvFloodedMsg(msg, shared_from_this());
            mApp.getOverlayManager().broadcastMessage(msg);
        }
    }
}

void
Peer::recvGetSCPQuorumSet(PaysharesMessage const& msg)
{
    auto localQSet = mApp.getConfig().quorumSet();
    auto localHash = sha256(xdr::xdr_to_opaque(localQSet));

    if (msg.qSetHash() == localHash)
    {
        sendSCPQuorumSet(localQSet);
    } else if (auto qSet = mApp.getOverlayManager().getQuorumSetFetcher().get(msg.qSetHash()))
    {
        sendSCPQuorumSet(*qSet);
    } else
    {
        CLOG(TRACE, "Overlay")
            << "No quorum set: " << hexAbbrev(msg.qSetHash());
        sendDontHave(SCP_QUORUMSET, msg.qSetHash());
        // do we want to ask other people for it?
    }
}
void
Peer::recvSCPQuorumSet(PaysharesMessage const& msg)
{
    auto hash = sha256(xdr::xdr_to_opaque(msg.qSet()));
    mApp.getOverlayManager().getQuorumSetFetcher().recv(hash, msg.qSet());
}

void
Peer::recvSCPMessage(PaysharesMessage const& msg)
{
    SCPEnvelope envelope = msg.envelope();
    CLOG(TRACE, "Overlay") << "recvSCPMessage qset: "
                           << binToHex(msg.envelope().statement.quorumSetHash)
                                  .substr(0, 6);

    mApp.getOverlayManager().recvFloodedMsg(msg, shared_from_this());

    auto cb = [msg, this](SCP::EnvelopeState state)
    {
        if (state == SCP::EnvelopeState::VALID)
        {
            mApp.getOverlayManager().broadcastMessage(msg);
        }
    };
    mApp.getHerder().recvSCPEnvelope(envelope, cb);
}

void
Peer::recvError(PaysharesMessage const& msg)
{
    // TODO.4
}

bool
Peer::recvHello(PaysharesMessage const& msg)
{
    if (msg.hello().peerID == mApp.getConfig().PEER_PUBLIC_KEY)
    {
        CLOG(DEBUG, "Overlay") << "connecting to self";
        drop();
        return false;
    }

    mRemoteProtocolVersion = msg.hello().protocolVersion;
    mRemoteVersion = msg.hello().versionStr;
    if (msg.hello().listeningPort <= 0 ||
        msg.hello().listeningPort > UINT16_MAX)
    {
        CLOG(DEBUG, "Overlay") << "bad port in recvHello";
        drop();
        return false;
    }
    mRemoteListeningPort =
        static_cast<unsigned short>(msg.hello().listeningPort);
    CLOG(DEBUG, "Overlay") << "recvHello from "
                          << toString();
    mState = GOT_HELLO;
    mPeerID = msg.hello().peerID;
    return true;
}

void
Peer::recvGetPeers(PaysharesMessage const& msg)
{
    sendPeers();
}

void
Peer::recvPeers(PaysharesMessage const& msg)
{
    for (auto peer : msg.peers())
    {
        stringstream ip;

        ip << (int)peer.ip[0] << "." << (int)peer.ip[1] << "."
           << (int)peer.ip[2] << "." << (int)peer.ip[3];

        if (peer.port == 0 || peer.port > UINT16_MAX)
        {
            CLOG(DEBUG, "Overlay") << "ignoring peer with bad port";
            continue;
        }
        PeerRecord pr{ip.str(), static_cast<unsigned short>(peer.port),
                      mApp.getClock().now(), peer.numFailures, 1};

        if (pr.isPrivateAddress())
        {
            CLOG(DEBUG, "Overlay") << "ignoring flooded private address";
        }
        else
        {
            pr.insertIfNew(mApp.getDatabase());
        }
    }
}
}
