// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/bindings/exception_handler.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/supports_user_data.h"
#include "gin/converter.h"
#include "gin/per_context_data.h"

namespace extensions {

namespace {

const char kExtensionExceptionHandlerPerContextKey[] =
    "extension_exception_handler";

struct ExceptionHandlerPerContextData : public base::SupportsUserData::Data {
  v8::Global<v8::Function> custom_handler;
};

// TODO(devlin): Extract this to a utility method.
ExceptionHandlerPerContextData* GetContextData(v8::Local<v8::Context> context,
                                               bool should_create) {
  gin::PerContextData* per_context_data = gin::PerContextData::From(context);
  if (!per_context_data)
    return nullptr;
  auto* data = static_cast<ExceptionHandlerPerContextData*>(
      per_context_data->GetUserData(kExtensionExceptionHandlerPerContextKey));

  if (!data && should_create) {
    auto api_data = base::MakeUnique<ExceptionHandlerPerContextData>();
    data = api_data.get();
    per_context_data->SetUserData(kExtensionExceptionHandlerPerContextKey,
                                  std::move(api_data));
  }

  return data;
}

}  // namespace

ExceptionHandler::ExceptionHandler(
    const binding::AddConsoleError& add_console_error,
    const binding::RunJSFunction& run_js)
    : add_console_error_(add_console_error), run_js_(run_js) {}
ExceptionHandler::~ExceptionHandler() {}

void ExceptionHandler::HandleException(v8::Local<v8::Context> context,
                                       const std::string& message,
                                       v8::TryCatch* try_catch) {
  DCHECK(try_catch->HasCaught());

  v8::Isolate* isolate = context->GetIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Message> v8_message = try_catch->Message();
  std::string full_message =
      !v8_message.IsEmpty()
          ? base::StringPrintf("%s: %s", message.c_str(),
                               gin::V8ToString(v8_message->Get()).c_str())
          : message;

  v8::Local<v8::Function> handler = GetCustomHandler(context);
  if (!handler.IsEmpty()) {
    v8::Local<v8::Value> arguments[] = {
        gin::StringToV8(isolate, full_message), try_catch->Exception(),
    };
    v8::TryCatch handler_try_catch(isolate);
    run_js_.Run(handler, context, arraysize(arguments), arguments);
  } else {
    add_console_error_.Run(context, full_message);
  }

  try_catch->Reset();  // Reset() to avoid handling the error more than once.
}

void ExceptionHandler::SetHandlerForContext(v8::Local<v8::Context> context,
                                            v8::Local<v8::Function> handler) {
  ExceptionHandlerPerContextData* data = GetContextData(context, true);
  DCHECK(data);
  data->custom_handler.Reset(context->GetIsolate(), handler);
}

v8::Local<v8::Function> ExceptionHandler::GetCustomHandler(
    v8::Local<v8::Context> context) {
  ExceptionHandlerPerContextData* data = GetContextData(context, false);
  return data ? data->custom_handler.Get(context->GetIsolate())
              : v8::Local<v8::Function>();
}

}  // namespace extensions
