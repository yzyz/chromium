// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/app/user_status_presenter.h"

#import "ios/third_party/material_components_ios/src/components/Snackbar/src/MaterialSnackbar.h"
#import "remoting/ios/facade/remoting_authentication.h"
#import "remoting/ios/facade/remoting_service.h"

@interface UserStatusPresenter () {
  BOOL _isStarted;
}
@end

@implementation UserStatusPresenter

- (instancetype)init {
  _isStarted = NO;
  return self;
}

- (void)dealloc {
  [self stop];
}

- (void)start {
  if (_isStarted) {
    return;
  }
  _isStarted = YES;
  if ([RemotingService.instance.authentication.user isAuthenticated]) {
    [self presentUserStatus];
  }
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(userDidUpdateNotification:)
             name:kUserDidUpdate
           object:nil];
}

- (void)stop {
  if (!_isStarted) {
    return;
  }
  _isStarted = NO;
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

+ (UserStatusPresenter*)instance {
  static UserStatusPresenter* presenter;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    presenter = [[UserStatusPresenter alloc] init];
  });
  return presenter;
}

#pragma mark - Private

- (void)userDidUpdateNotification:(NSNotification*)notification {
  [self presentUserStatus];
}

- (void)presentUserStatus {
  UserInfo* user = RemotingService.instance.authentication.user;
  if (![user isAuthenticated]) {
    // No need to show the toast since we will pop up a sign-in view in this
    // case.
    return;
  }
  MDCSnackbarMessage* message = [[MDCSnackbarMessage alloc] init];
  message.text =
      [NSString stringWithFormat:@"Currently signed in as %@.", user.userEmail];
  [MDCSnackbarManager showMessage:message];
}

@end
