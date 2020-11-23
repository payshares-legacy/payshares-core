#pragma once

// Copyright 2014 Payshares Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "util/asio.h"
#include <memory>

/*
listens for peer connections.
When found passes them to the OverlayManagerImpl
*/

namespace payshares
{
class Application;
class PeerDoorStub;

class PeerDoor
{
  protected:
    Application& mApp;
    asio::ip::tcp::acceptor mAcceptor;

    virtual void acceptNextPeer();
    virtual void handleKnock(std::shared_ptr<asio::ip::tcp::socket> pSocket);

    friend PeerDoorStub;

  public:
    typedef std::shared_ptr<PeerDoor> pointer;

    PeerDoor(Application&);

    void close();
};
}
