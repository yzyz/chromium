// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WorkerBackingThread_h
#define WorkerBackingThread_h

#include <memory>

#include "core/CoreExport.h"
#include "platform/heap/ThreadState.h"
#include "platform/wtf/Forward.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/ThreadingPrimitives.h"
#include "v8/include/v8.h"

namespace blink {

class WebThread;
class WebThreadSupportingGC;
struct WorkerV8Settings;

// WorkerBackingThread represents a WebThread with Oilpan and V8 potentially
// shared by multiple WebWorker scripts. A WebWorker needs to call initialize()
// to using V8 and Oilpan functionalities, and call shutdown() when the script
// no longer needs the thread.
// A WorkerBackingThread represents a WebThread while a WorkerThread corresponds
// to a web worker. There is one-to-one correspondence between WorkerThread and
// WorkerGlobalScope, but multiple WorkerThreads (i.e., multiple
// WorkerGlobalScopes) can share one WorkerBackingThread.
class CORE_EXPORT WorkerBackingThread final {
 public:
  static std::unique_ptr<WorkerBackingThread> Create(const char* name) {
    return WTF::WrapUnique(new WorkerBackingThread(name, false));
  }
  static std::unique_ptr<WorkerBackingThread> Create(WebThread* thread) {
    return WTF::WrapUnique(new WorkerBackingThread(thread, false));
  }

  // These are needed to suppress leak reports. See
  // https://crbug.com/590802 and https://crbug.com/v8/1428.
  static std::unique_ptr<WorkerBackingThread> CreateForTest(const char* name) {
    return WTF::WrapUnique(new WorkerBackingThread(name, true));
  }
  static std::unique_ptr<WorkerBackingThread> CreateForTest(WebThread* thread) {
    return WTF::WrapUnique(new WorkerBackingThread(thread, true));
  }

  ~WorkerBackingThread();

  // initialize() and shutdown() attaches and detaches Oilpan and V8 to / from
  // the caller worker script, respectively. A worker script must not call
  // any function after calling shutdown().
  // They should be called from |this| thread.
  void Initialize(const WorkerV8Settings&);
  void Shutdown();

  WebThreadSupportingGC& BackingThread() {
    DCHECK(backing_thread_);
    return *backing_thread_;
  }

  v8::Isolate* GetIsolate() { return isolate_; }

  static void MemoryPressureNotificationToWorkerThreadIsolates(
      v8::MemoryPressureLevel);

  static void SetRAILModeOnWorkerThreadIsolates(v8::RAILMode);

 private:
  WorkerBackingThread(const char* name, bool should_call_gc_on_shutdown);
  WorkerBackingThread(WebThread*, bool should_call_gc_on_s_hutdown);

  std::unique_ptr<WebThreadSupportingGC> backing_thread_;
  v8::Isolate* isolate_ = nullptr;
  bool is_owning_thread_;
  bool should_call_gc_on_shutdown_;
};

}  // namespace blink

#endif  // WorkerBackingThread_h
