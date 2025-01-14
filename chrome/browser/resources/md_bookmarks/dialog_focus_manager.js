// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('bookmarks', function() {
  /**
   * Manages focus restoration for modal dialogs. After the final dialog in a
   * stack is closed, restores focus to the element which was focused when the
   * first dialog was opened.
   * @constructor
   */
  function DialogFocusManager() {
    /** @private {HTMLElement} */
    this.previousFocusElement_ = null;

    /** @private {boolean} */
    this.previousMouseFocus_ = false;

    /** @private {Set<HTMLDialogElement>} */
    this.dialogs_ = new Set();
  }

  DialogFocusManager.prototype = {
    /**
     * @param {HTMLDialogElement} dialog
     * @param {function()=} showFn
     */
    showDialog: function(dialog, showFn) {
      if (!showFn) {
        showFn = function() {
          dialog.showModal();
        };
      }

      // Update the focus if there are no open dialogs or if this is the only
      // dialog and it's getting reshown.
      if (!this.dialogs_.size ||
          (this.dialogs_.has(dialog) && this.dialogs_.size == 1)) {
        this.updatePreviousFocus_();
      }

      if (!this.dialogs_.has(dialog)) {
        dialog.addEventListener('close', this.getCloseListener_(dialog));
        this.dialogs_.add(dialog);
      }

      showFn();
    },

    /** @private */
    updatePreviousFocus_: function() {
      this.previousFocusElement_ = this.getFocusedElement_();
      this.previousMouseFocus_ = bookmarks.MouseFocusBehavior.isMouseFocused(
          this.previousFocusElement_);
    },

    /**
     * @return {HTMLElement}
     * @private
     */
    getFocusedElement_: function() {
      var focus = document.activeElement;
      while (focus.root && focus.root.activeElement)
        focus = focus.root.activeElement;

      return focus;
    },

    /**
     * @param {HTMLDialogElement} dialog
     * @return {function(Event)}
     * @private
     */
    getCloseListener_: function(dialog) {
      var closeListener = function(e) {
        // If the dialog is open, then it got reshown immediately and we
        // shouldn't clear it until it is closed again.
        if (dialog.open)
          return;

        assert(this.dialogs_.delete(dialog));
        // Focus the originally focused element if there are no more dialogs.
        if (!this.dialogs_.size) {
          this.previousFocusElement_.focus();
          if (this.previousMouseFocus_) {
            bookmarks.MouseFocusBehavior.addMouseFocusClass(
                this.previousFocusElement_);
          }
        }
        dialog.removeEventListener('close', closeListener);
      }.bind(this);

      return closeListener;
    },
  };

  cr.addSingletonGetter(DialogFocusManager);

  return {
    DialogFocusManager: DialogFocusManager,
  };
});
