// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MIME_REGISTRY_IMPL_H_
#define CONTENT_BROWSER_MIME_REGISTRY_IMPL_H_

#include "base/macros.h"
#include "base/sequence_checker.h"
#include "third_party/WebKit/public/platform/mime_registry.mojom.h"

namespace service_manager {
struct BindSourceInfo;
}

namespace content {

class MimeRegistryImpl : public blink::mojom::MimeRegistry {
 public:
  MimeRegistryImpl();
  ~MimeRegistryImpl() override;

  static void Create(const service_manager::BindSourceInfo& source_info,
                     blink::mojom::MimeRegistryRequest request);

 private:
  void GetMimeTypeFromExtension(
      const std::string& extension,
      GetMimeTypeFromExtensionCallback callback) override;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(MimeRegistryImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MIME_REGISTRY_IMPL_H_
