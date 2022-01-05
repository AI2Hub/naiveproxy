// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_MOCK_CLIENT_SOCKET_POOL_MANAGER_H_
#define NET_SOCKET_MOCK_CLIENT_SOCKET_POOL_MANAGER_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "net/base/proxy_server.h"
#include "net/socket/client_socket_pool_manager.h"
#include "net/socket/client_socket_pool_manager_impl.h"

namespace net {

class ClientSocketPool;

class MockClientSocketPoolManager : public ClientSocketPoolManager {
 public:
  MockClientSocketPoolManager();

  MockClientSocketPoolManager(const MockClientSocketPoolManager&) = delete;
  MockClientSocketPoolManager& operator=(const MockClientSocketPoolManager&) =
      delete;

  ~MockClientSocketPoolManager() override;

  // Sets socket pool that gets used for the specified ProxyServer.
  void SetSocketPool(const ProxyServer& proxy_server,
                     std::unique_ptr<ClientSocketPool> pool);

  // ClientSocketPoolManager methods:
  void FlushSocketPoolsWithError(int error,
                                 const char* net_log_reason_utf8) override;
  void CloseIdleSockets(const char* net_log_reason_utf8) override;
  ClientSocketPool* GetSocketPool(const ProxyServer& proxy_server) override;
  std::unique_ptr<base::Value> SocketPoolInfoToValue() const override;

 private:
  using ClientSocketPoolMap =
      std::map<ProxyServer, std::unique_ptr<ClientSocketPool>>;

  ClientSocketPoolMap socket_pools_;
};

}  // namespace net

#endif  // NET_SOCKET_MOCK_CLIENT_SOCKET_POOL_MANAGER_H_
