// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop.h"
#include "base/run_loop.h"
#include "content/browser/browser_thread_impl.h"
#include "content/public/browser/devtools_http_handler.h"
#include "content/public/browser/devtools_http_handler_delegate.h"
#include "net/socket/stream_listen_socket.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

using net::StreamListenSocket;

class DummyListenSocket : public StreamListenSocket,
                          public StreamListenSocket::Delegate {
 public:
  DummyListenSocket()
      : StreamListenSocket(0, this) {}

  // StreamListenSocket::Delegate "implementation"
  virtual void DidAccept(StreamListenSocket* server,
                         StreamListenSocket* connection) OVERRIDE {}
  virtual void DidRead(StreamListenSocket* connection,
                       const char* data,
                       int len) OVERRIDE {}
  virtual void DidClose(StreamListenSocket* sock) OVERRIDE {}
 protected:
  virtual ~DummyListenSocket() {}
  virtual void Accept() OVERRIDE {}
};

class DummyListenSocketFactory : public net::StreamListenSocketFactory {
 public:
  DummyListenSocketFactory(
      base::Closure quit_closure_1, base::Closure quit_closure_2)
      : quit_closure_1_(quit_closure_1), quit_closure_2_(quit_closure_2) {}
  virtual ~DummyListenSocketFactory() {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE, quit_closure_2_);
  }

  virtual scoped_refptr<StreamListenSocket> CreateAndListen(
      StreamListenSocket::Delegate* delegate) const OVERRIDE {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE, quit_closure_1_);
    return new DummyListenSocket();
  }
 private:
  base::Closure quit_closure_1_;
  base::Closure quit_closure_2_;
};

class DummyDelegate : public DevToolsHttpHandlerDelegate {
 public:
  virtual std::string GetDiscoveryPageHTML() OVERRIDE { return std::string(); }
  virtual bool BundlesFrontendResources() OVERRIDE { return true; }
  virtual base::FilePath GetDebugFrontendDir() OVERRIDE {
    return base::FilePath();
  }
  virtual std::string GetPageThumbnailData(const GURL& url) OVERRIDE {
    return std::string();
  }
  virtual RenderViewHost* CreateNewTarget() OVERRIDE { return NULL; }
  virtual TargetType GetTargetType(RenderViewHost*) OVERRIDE {
    return kTargetTypeTab;
  }
  virtual std::string GetViewDescription(content::RenderViewHost*) OVERRIDE {
    return std::string();
  }
  virtual scoped_refptr<net::StreamListenSocket> CreateSocketForTethering(
    net::StreamListenSocket::Delegate* delegate,
    std::string* name) OVERRIDE {
    return NULL;
  }
};

}

class DevToolsHttpHandlerTest : public testing::Test {
 public:
  DevToolsHttpHandlerTest()
      : ui_thread_(BrowserThread::UI, &message_loop_) {
  }
 protected:
  virtual void SetUp() {
    file_thread_.reset(new BrowserThreadImpl(BrowserThread::FILE));
    file_thread_->Start();
  }
  virtual void TearDown() {
    file_thread_->Stop();
  }
 private:
  MessageLoopForIO message_loop_;
  BrowserThreadImpl ui_thread_;
  scoped_ptr<BrowserThreadImpl> file_thread_;
};

TEST_F(DevToolsHttpHandlerTest, TestStartStop) {
  base::RunLoop run_loop, run_loop_2;
  content::DevToolsHttpHandler* devtools_http_handler_ =
      content::DevToolsHttpHandler::Start(
          new DummyListenSocketFactory(run_loop.QuitClosure(),
                                       run_loop_2.QuitClosure()),
          std::string(),
          new DummyDelegate());
  // Our dummy socket factory will post a quit message once the server will
  // become ready.
  run_loop.Run();
  devtools_http_handler_->Stop();
  // Make sure the handler actually stops.
  run_loop_2.Run();
}

}  // namespace content
