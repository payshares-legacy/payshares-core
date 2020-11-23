// Copyright 2014 Payshares Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "PeerDoor.h"
#include <memory>
#include "main/Application.h"
#include "main/Config.h"
#include "Peer.h"
#include "overlay/OverlayManager.h"
#include "util/Logging.h"
#include "overlay/TCPPeer.h"

namespace payshares
{

using asio::ip::tcp;
using namespace std;

PeerDoor::PeerDoor(Application& app)
    : mApp(app), mAcceptor(mApp.getClock().getIOService())
{
    if (!mApp.getConfig().RUN_STANDALONE)
    {
        tcp::endpoint endpoint(tcp::v4(), mApp.getConfig().PEER_PORT);
        CLOG(DEBUG, "Overlay") << "PeerDoor binding to endpoint " << endpoint;
        mAcceptor.open(endpoint.protocol());
        mAcceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
        mAcceptor.bind(endpoint);
        mAcceptor.listen();
        acceptNextPeer();
    }
}

void
PeerDoor::close()
{
    mAcceptor.cancel();
}

void
PeerDoor::acceptNextPeer()
{
    CLOG(DEBUG, "Overlay") << "PeerDoor acceptNextPeer()";
    auto sock = make_shared<tcp::socket>(mApp.getClock().getIOService());
    mAcceptor.async_accept(*sock, [this, sock](asio::error_code const& ec)
                           {
        if (ec)
            this->acceptNextPeer();
        else
            this->handleKnock(sock);
    });
}

void
PeerDoor::handleKnock(shared_ptr<tcp::socket> socket)
{
    CLOG(DEBUG, "Overlay") << "PeerDoor handleKnock() @"
                           << mApp.getConfig().PEER_PORT;
    Peer::pointer peer = TCPPeer::accept(mApp, socket);
    mApp.getOverlayManager().addConnectedPeer(peer);
    acceptNextPeer();
}
}
