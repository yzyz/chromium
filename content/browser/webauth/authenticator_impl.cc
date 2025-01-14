// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_impl.h"

#include <memory>

#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "crypto/sha2.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

namespace {

const char kGetAssertionType[] = "navigator.id.getAssertion";

// JSON key values
const char kTypeKey[] = "type";
const char kChallengeKey[] = "challenge";
const char kOriginKey[] = "origin";
const char kCidPubkeyKey[] = "cid_pubkey";

}  // namespace

// Serializes the |value| to a JSON string and returns the result.
std::string SerializeValueToJson(const base::Value& value) {
  std::string json;
  base::JSONWriter::Write(value, &json);
  return json;
}

// static
void AuthenticatorImpl::Create(
    RenderFrameHost* render_frame_host,
    const service_manager::BindSourceInfo& source_info,
    webauth::mojom::AuthenticatorRequest request) {
  auto authenticator_impl =
      base::WrapUnique(new AuthenticatorImpl(render_frame_host));
  mojo::MakeStrongBinding(std::move(authenticator_impl), std::move(request));
}

AuthenticatorImpl::~AuthenticatorImpl() {}

AuthenticatorImpl::AuthenticatorImpl(RenderFrameHost* render_frame_host) {
  DCHECK(render_frame_host);
  caller_origin_ = render_frame_host->GetLastCommittedOrigin();
}

// mojom:Authenticator
void AuthenticatorImpl::MakeCredential(
    webauth::mojom::RelyingPartyAccountPtr account,
    std::vector<webauth::mojom::ScopedCredentialParametersPtr> parameters,
    const std::vector<uint8_t>& challenge,
    webauth::mojom::ScopedCredentialOptionsPtr options,
    MakeCredentialCallback callback) {
  std::string effective_domain;
  std::string relying_party_id;
  std::string client_data_json;
  base::DictionaryValue client_data;

  // Steps 6 & 7 of https://w3c.github.io/webauthn/#createCredential
  // opaque origin
  if (caller_origin_.unique()) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, NULL);
    return;
  }

  if (!options->relying_party_id) {
    relying_party_id = caller_origin_.Serialize();
  } else {
    effective_domain = caller_origin_.host();

    DCHECK(!effective_domain.empty());
    // TODO(kpaulhamus): Check if relyingPartyId is a registrable domain
    // suffix of and equal to effectiveDomain and set relyingPartyId
    // appropriately.
    relying_party_id = options->relying_party_id.value_or(std::string());
  }

  // TODO(kpaulhamus): Check ScopedCredentialParameter's type and
  // algorithmIdentifier after algorithmIdentifier is added to mojom to
  // make sure it is U2F_V2.

  client_data.SetString(kTypeKey, kGetAssertionType);
  client_data.SetString(
      kChallengeKey,
      base::StringPiece(reinterpret_cast<const char*>(challenge.data()),
                        challenge.size()));
  client_data.SetString(kOriginKey, relying_party_id);
  // Channel ID is optional, and missing if the browser doesn't support it.
  // It is present and set to the constant "unused" if the browser
  // supports Channel ID but is not using it to talk to the origin.
  // TODO(kpaulhamus): Fetch and add the Channel ID public key used to
  // communicate with the origin.
  client_data.SetString(kCidPubkeyKey, "unused");

  // SHA-256 hash the JSON data structure
  client_data_json = SerializeValueToJson(client_data);
  std::string client_data_hash = crypto::SHA256HashString(client_data_json);

  std::move(callback).Run(webauth::mojom::AuthenticatorStatus::NOT_IMPLEMENTED,
                          nullptr);
}

}  // namespace content
