// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_TRACKED_TRACKED_PERSISTENT_PREF_STORE_FACTORY_H_
#define SERVICES_PREFERENCES_TRACKED_TRACKED_PERSISTENT_PREF_STORE_FACTORY_H_

#include <utility>
#include "services/preferences/public/interfaces/preferences.mojom.h"

namespace base {
class DictionaryValue;
class SequencedWorkerPool;
}

class PersistentPrefStore;

PersistentPrefStore* CreateTrackedPersistentPrefStore(
    prefs::mojom::TrackedPersistentPrefStoreConfigurationPtr config,
    base::SequencedWorkerPool* worker_pool);

// TODO(sammc): This should move somewhere more appropriate in the longer term.
void InitializeMasterPrefsTracking(
    prefs::mojom::TrackedPersistentPrefStoreConfigurationPtr configuration,
    base::DictionaryValue* master_prefs);

#endif  // SERVICES_PREFERENCES_TRACKED_TRACKED_PERSISTENT_PREF_STORE_FACTORY_H_
