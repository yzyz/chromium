// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.signin;

import android.support.annotation.MainThread;

/**
 * Observer that receives account change notifications. Use {@link AccountManagerHelper#addObserver}
 * and {@link AccountManagerHelper#removeObserver} to update registrations.
 */
public interface AccountsChangeObserver {
    /**
     * Called on every change to the accounts or to the error condition that occurred while getting
     * accounts.
     */
    @MainThread
    void onAccountsChanged();
}
