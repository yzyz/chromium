// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/shipping_option_selection_coordinator.h"

#include "base/mac/foundation_util.h"
#include "base/memory/ptr_util.h"
#include "base/test/ios/wait_util.h"
#include "base/test/scoped_task_environment.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/test_personal_data_manager.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/payments/payment_request.h"
#include "ios/chrome/browser/payments/payment_request_test_util.h"
#include "ios/chrome/browser/payments/test_payment_request.h"
#import "ios/chrome/browser/ui/payments/payment_request_selector_view_controller.h"
#include "ios/web/public/payments/payment_request.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class PaymentRequestShippingOptionSelectionCoordinatorTest
    : public PlatformTest {
 protected:
  PaymentRequestShippingOptionSelectionCoordinatorTest()
      : chrome_browser_state_(TestChromeBrowserState::Builder().Build()) {
    payment_request_ = base::MakeUnique<payments::TestPaymentRequest>(
        payment_request_test_util::CreateTestWebPaymentRequest(),
        chrome_browser_state_.get(), &web_state_, &personal_data_manager_);
  }

  base::test::ScopedTaskEnvironment scoped_task_evironment_;

  web::TestWebState web_state_;
  autofill::TestPersonalDataManager personal_data_manager_;
  std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
  std::unique_ptr<payments::TestPaymentRequest> payment_request_;
};

// Tests that invoking start and stop on the coordinator presents and dismisses
// the PaymentRequestSelectorViewController, respectively.
TEST_F(PaymentRequestShippingOptionSelectionCoordinatorTest, StartAndStop) {
  UIViewController* base_view_controller = [[UIViewController alloc] init];
  UINavigationController* navigation_controller =
      [[UINavigationController alloc]
          initWithRootViewController:base_view_controller];

  ShippingOptionSelectionCoordinator* coordinator =
      [[ShippingOptionSelectionCoordinator alloc]
          initWithBaseViewController:base_view_controller];
  [coordinator setPaymentRequest:payment_request_.get()];

  EXPECT_EQ(1u, navigation_controller.viewControllers.count);

  [coordinator start];
  // Spin the run loop to trigger the animation.
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  EXPECT_EQ(2u, navigation_controller.viewControllers.count);

  UIViewController* view_controller =
      navigation_controller.visibleViewController;
  EXPECT_TRUE([view_controller
      isMemberOfClass:[PaymentRequestSelectorViewController class]]);

  [coordinator stop];
  // Spin the run loop to trigger the animation.
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  EXPECT_EQ(1u, navigation_controller.viewControllers.count);
}

// Tests that calling the view controller delegate method which notifies the
// delegate about selection of a shipping option invokes the corresponding
// coordinator delegate method.
TEST_F(PaymentRequestShippingOptionSelectionCoordinatorTest,
       SelectedShippingOption) {
  UIViewController* base_view_controller = [[UIViewController alloc] init];
  UINavigationController* navigation_controller =
      [[UINavigationController alloc]
          initWithRootViewController:base_view_controller];

  ShippingOptionSelectionCoordinator* coordinator =
      [[ShippingOptionSelectionCoordinator alloc]
          initWithBaseViewController:base_view_controller];
  [coordinator setPaymentRequest:payment_request_.get()];

  // Mock the coordinator delegate.
  id delegate = [OCMockObject
      mockForProtocol:@protocol(ShippingOptionSelectionCoordinatorDelegate)];
  web::PaymentShippingOption* option = payment_request_->shipping_options()[1];
  [[delegate expect] shippingOptionSelectionCoordinator:coordinator
                                didSelectShippingOption:option];
  [coordinator setDelegate:delegate];

  EXPECT_EQ(1u, navigation_controller.viewControllers.count);

  [coordinator start];
  // Spin the run loop to trigger the animation.
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  EXPECT_EQ(2u, navigation_controller.viewControllers.count);

  // Call the controller delegate method.
  PaymentRequestSelectorViewController* view_controller =
      base::mac::ObjCCastStrict<PaymentRequestSelectorViewController>(
          navigation_controller.visibleViewController);
  EXPECT_TRUE([coordinator paymentRequestSelectorViewController:view_controller
                                           didSelectItemAtIndex:1]);

  // Wait for the coordinator delegate to be notified.
  base::test::ios::SpinRunLoopWithMinDelay(base::TimeDelta::FromSecondsD(0.5));

  EXPECT_OCMOCK_VERIFY(delegate);
}

// Tests that calling the view controller delegate method which notifies the
// delegate that the user has chosen to return without making a selection
// invokes the coordinator delegate method.
TEST_F(PaymentRequestShippingOptionSelectionCoordinatorTest, DidReturn) {
  UIViewController* base_view_controller = [[UIViewController alloc] init];
  UINavigationController* navigation_controller =
      [[UINavigationController alloc]
          initWithRootViewController:base_view_controller];

  ShippingOptionSelectionCoordinator* coordinator =
      [[ShippingOptionSelectionCoordinator alloc]
          initWithBaseViewController:base_view_controller];
  [coordinator setPaymentRequest:payment_request_.get()];

  // Mock the coordinator delegate.
  id delegate = [OCMockObject
      mockForProtocol:@protocol(ShippingOptionSelectionCoordinatorDelegate)];
  [[delegate expect] shippingOptionSelectionCoordinatorDidReturn:coordinator];
  [coordinator setDelegate:delegate];

  EXPECT_EQ(1u, navigation_controller.viewControllers.count);

  [coordinator start];
  // Spin the run loop to trigger the animation.
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(1.0));
  EXPECT_EQ(2u, navigation_controller.viewControllers.count);

  // Call the controller delegate method.
  PaymentRequestSelectorViewController* view_controller =
      base::mac::ObjCCastStrict<PaymentRequestSelectorViewController>(
          navigation_controller.visibleViewController);
  [coordinator paymentRequestSelectorViewControllerDidFinish:view_controller];

  EXPECT_OCMOCK_VERIFY(delegate);
}
