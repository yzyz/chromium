// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_UTILITY_UTILITY_THREAD_IMPL_H_
#define CONTENT_UTILITY_UTILITY_THREAD_IMPL_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/child/child_thread_impl.h"
#include "content/public/utility/utility_thread.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/interfaces/service_factory.mojom.h"

namespace service_manager {
struct BindSourceInfo;
}

namespace content {
class UtilityBlinkPlatformImpl;
class UtilityServiceFactory;

#if defined(COMPILER_MSVC)
// See explanation for other RenderViewHostImpl which is the same issue.
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

// This class represents the background thread where the utility task runs.
class UtilityThreadImpl : public UtilityThread,
                          public ChildThreadImpl {
 public:
  UtilityThreadImpl();
  // Constructor used when running in single process mode.
  explicit UtilityThreadImpl(const InProcessChildThreadParams& params);
  ~UtilityThreadImpl() override;
  void Shutdown() override;

  // UtilityThread:
  void ReleaseProcess() override;
  void EnsureBlinkInitialized() override;

 private:
  void Init();

  // ChildThread:
  bool OnControlMessageReceived(const IPC::Message& msg) override;

  // Binds requests to our |service factory_|.
  void BindServiceFactoryRequest(
      const service_manager::BindSourceInfo& source_info,
      service_manager::mojom::ServiceFactoryRequest request);

  // blink::Platform implementation if needed.
  std::unique_ptr<UtilityBlinkPlatformImpl> blink_platform_impl_;

  // service_manager::mojom::ServiceFactory for service_manager::Service
  // hosting.
  std::unique_ptr<UtilityServiceFactory> service_factory_;

  // Bindings to the service_manager::mojom::ServiceFactory impl.
  mojo::BindingSet<service_manager::mojom::ServiceFactory>
      service_factory_bindings_;

  DISALLOW_COPY_AND_ASSIGN(UtilityThreadImpl);
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)
#endif

}  // namespace content

#endif  // CONTENT_UTILITY_UTILITY_THREAD_IMPL_H_
