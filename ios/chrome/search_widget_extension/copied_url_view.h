// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_SEARCH_WIDGET_EXTENSION_COPIED_URL_VIEW_H_
#define IOS_CHROME_SEARCH_WIDGET_EXTENSION_COPIED_URL_VIEW_H_

#import <UIKit/UIKit.h>

// View to show and allow opening of the copied URL. Shows a button with the
// |copiedURLString| if it has been set. When tapped, |actionSelector| in
// |target| is called. If no |copiedURLString| was set, the button is replaced
// by a hairline separation and placeholder text.
@interface CopiedURLView : UIView

// The copied URL string to be displayed. nil is a valid value to indicate
// there is no copied URL to display.
@property(nonatomic, copy) NSString* copiedURLString;

// Designated initializer, creates the copiedURLView with a |target| to open the
// URL. The |primaryVibrancyEffect| and |secondaryVibrancyEffect| are used to
// display view elements.
- (instancetype)initWithActionTarget:(id)target
                      actionSelector:(SEL)actionSelector
                       primaryEffect:(UIVisualEffect*)primaryEffect
                     secondaryEffect:(UIVisualEffect*)secondaryEffect
    NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_SEARCH_WIDGET_EXTENSION_COPIED_URL_VIEW_H_
