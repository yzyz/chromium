// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/bookmark_home_tablet_ntp_controller.h"

#include <memory>

#include "base/ios/block_types.h"
#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/sys_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/strings/grit/components_strings.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/bookmarks/bookmarks_utils.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/metrics/new_tab_page_uma.h"
#import "ios/chrome/browser/ui/alert_coordinator/action_sheet_coordinator.h"
#import "ios/chrome/browser/ui/bookmarks/bars/bookmark_editing_bar.h"
#import "ios/chrome/browser/ui/bookmarks/bars/bookmark_navigation_bar.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_collection_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_edit_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_folder_editor_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_folder_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_primary_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_waiting_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_menu_item.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_menu_view.h"
#include "ios/chrome/browser/ui/bookmarks/bookmark_model_bridge_observer.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_navigation_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_panel_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_promo_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_utils_ios.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/url_loader.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/web/public/referrer.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/page_transition_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using bookmarks::BookmarkNode;

namespace {
// The margin on top to the navigation bar.
const CGFloat kNavigationBarTopMargin = 8.0;
}  // namespace

@interface BookmarkHomeTabletNTPController ()<
    BookmarkCollectionViewDelegate,
    BookmarkMenuViewDelegate,
    BookmarkModelBridgeObserver> {
  // Bridge to register for bookmark changes.
  std::unique_ptr<bookmarks::BookmarkModelBridge> _bridge;

  // The following 2 ivars both represent the set of nodes being edited.
  // The set is for fast lookup.
  // The vector maintains the order that edit nodes were added.
  // Use the relevant instance methods to modify these two ivars in tandem.
  // DO NOT modify these two ivars directly.
  std::set<const BookmarkNode*> _editNodes;
  std::vector<const BookmarkNode*> _editNodesOrdered;
}

#pragma mark - Properties and methods akin to BookmarkHomeHandsetViewController

// Whether the view controller is in editing mode.
@property(nonatomic, assign) BOOL editing;
// The set of edited index paths.
@property(nonatomic, strong) NSMutableArray* editIndexPaths;

// Replaces |_editNodes| and |_editNodesOrdered| with new container objects.
- (void)resetEditNodes;
// Adds |node| corresponding to a |cell| if it isn't already present.
- (void)insertEditNode:(const BookmarkNode*)node
           atIndexPath:(NSIndexPath*)indexPath;
// Removes |node| corresponding to a |cell| if it's present.
- (void)removeEditNode:(const BookmarkNode*)node
           atIndexPath:(NSIndexPath*)indexPath;
// This method updates the property, and resets the edit nodes.
- (void)setEditing:(BOOL)editing animated:(BOOL)animated;

#pragma mark - Properties and methods akin to BookmarkHomeHandsetViewController
// When the view is first shown on the screen, this property represents the
// cached value of the y of the content offset of the primary view. This
// property is set to nil after it is used.
@property(nonatomic, strong)
    NSNumber* cachedContentPosition;  // FIXME: INACTIVE

// The editing bar present when items are selected.
@property(nonatomic, strong) BookmarkEditingBar* editingBar;

// The action sheet coordinator used when trying to edit a single bookmark.
@property(nonatomic, strong) ActionSheetCoordinator* actionSheetCoordinator;

#pragma mark Specific to this class.

// Opens the url.
- (void)loadURL:(const GURL&)url;
#pragma mark View loading, laying out, and switching.
// This method should be called at most once in the life-cycle of the class.
// It should be called at the soonest possible time after the view has been
// loaded, and the bookmark model is loaded.
- (void)loadBookmarkViews;
// Updates the property 'primaryMenuItem'.
// Updates the UI to reflect the new state of 'primaryMenuItem'.
- (void)updatePrimaryMenuItem:(BookmarkMenuItem*)menuItem
                     animated:(BOOL)animated;
// Returns whether the menu should be in a side panel that slides in.
- (BOOL)shouldPresentMenuInSlideInPanel;
// Returns the leading margin of the primary view.
- (CGFloat)primaryViewLeadingMargin;
// Moves the menu and primary view to their correct parent views depending on
// the layout.
- (void)moveMenuAndPrimaryViewToAdequateParent;
// Updates the frame of the primary view.
- (void)refreshFrameOfPrimaryView;
// Returns the frame of the primary view.
- (CGRect)frameForPrimaryView;

#pragma mark Editing related methods
// This method statelessly updates the editing top bar from |_editNodes| and
// |editing|.
- (void)updateEditingStateAnimated:(BOOL)animated;
// Shows or hides the editing bar.
- (void)showEditingBarAnimated:(BOOL)animated;
- (void)hideEditingBarAnimated:(BOOL)animated;
// Instaneously updates the shadow of the edit bar.
// This method should be called anytime:
//  (1)|editing| property changes.
//  (2)The primary view changes.
//  (3)The primary view's collection view is scrolled.
// When |editing| is NO, the shadow is never shown.
- (void)updateEditBarShadow;

#pragma mark Editing bar callbacks
// The cancel button was tapped on the editing bar.
- (void)editingBarCancel;
// The move button was tapped on the editing bar.
- (void)editingBarMove;
// The delete button was tapped on the editing bar.
- (void)editingBarDelete;
// The edit button was tapped on the editing bar.
- (void)editingBarEdit;
// The menu button is pressed on the editing bar.
- (void)toggleMenuAnimated;

#pragma mark Action sheet callbacks
// Enters into edit mode by selecting the given node corresponding to the
// given cell.
- (void)selectFirstNode:(const BookmarkNode*)node
               withCell:(UICollectionViewCell*)cell;

#pragma mark private utility methods
// Deletes the nodes, and presents a toast with an undo button.
- (void)deleteSelectedNodes;

#pragma mark Navigation bar
// Updates the UI of the navigation bar with the primaryMenuItem.
// This method should be called anytime:
//  (1)The primary view changes.
//  (2)The primary view has type folder, and the relevant folder has changed.
//  (3)The interface orientation changes.
//  (4)viewWillAppear, as the interface orientation may have changed.
- (void)updateNavigationBarAnimated:(BOOL)animated
                        orientation:(UIInterfaceOrientation)orientation;
- (void)updateNavigationBarWithDuration:(CGFloat)duration
                            orientation:(UIInterfaceOrientation)orientation;
// Whether the edit button on the navigation bar should be shown.
- (BOOL)shouldShowEditButton;
// Whether the back button on the navigation bar should be shown.
- (BOOL)shouldShowBackButton;
// Called when the back button is pressed on the navigation bar.
- (void)navigationBarBack:(id)sender;

// TODO(crbug.com/450646): This should not be needed but require refactoring of
// the BookmarkCollectionViewDelegate.
- (NSIndexPath*)indexPathForCell:(UICollectionViewCell*)cell;

@end

@implementation BookmarkHomeTabletNTPController

@synthesize editing = _editing;
@synthesize editIndexPaths = _editIndexPaths;
@synthesize cachedContentPosition = _cachedContentPosition;
@synthesize editingBar = _editingBar;

@synthesize actionSheetCoordinator = _actionSheetCoordinator;

// Property declared in NewTabPagePanelProtocol.
@synthesize delegate = _delegate;

- (id)initWithLoader:(id<UrlLoader>)loader
        browserState:(ios::ChromeBrowserState*)browserState {
  self = [super initWithLoader:loader browserState:browserState];
  if (self) {
    _bridge.reset(new bookmarks::BookmarkModelBridge(self, self.bookmarks));
    _editIndexPaths = [[NSMutableArray alloc] init];
  }
  return self;
}

#pragma mark - UIViewController method.

- (void)viewWillLayoutSubviews {
  [super viewWillLayoutSubviews];
  if (![self primaryView] && ![self primaryMenuItem] &&
      self.bookmarks->loaded()) {
    BookmarkMenuItem* item = nil;
    CGFloat position = 0;
    BOOL found =
        bookmark_utils_ios::GetPositionCache(self.bookmarks, &item, &position);
    if (!found)
      item = [self.menuView defaultMenuItem];

    [self updatePrimaryMenuItem:item animated:NO];
  }

  [self moveMenuAndPrimaryViewToAdequateParent];
  CGFloat leadingMargin = [self primaryViewLeadingMargin];

  // Prevent the panelView from hijacking the gestures so that the
  // NTPController's scrollview can still scroll with the gestures.
  [self.panelView enableSideSwiping:NO];

  CGFloat width = self.view.bounds.size.width;
  LayoutRect navBarLayout =
      LayoutRectMake(leadingMargin, width, 0, width - leadingMargin,
                     CGRectGetHeight([self navigationBarFrame]));
  self.navigationBar.frame = LayoutRectGetRect(navBarLayout);
  [self.editingBar setFrame:[self editingBarFrame]];

  UIInterfaceOrientation orient = GetInterfaceOrientation();
  [self refreshFrameOfPrimaryView];
  [[self primaryView] changeOrientation:orient];
  [self updateNavigationBarWithDuration:0 orientation:orient];
  if (![self shouldPresentMenuInSlideInPanel])
    [self updateMenuViewLayout];
}

#pragma mark HomeViewController simili methods.

- (void)resetEditNodes {
  _editNodes = std::set<const BookmarkNode*>();
  _editNodesOrdered = std::vector<const BookmarkNode*>();
  [self.editIndexPaths removeAllObjects];
}

- (void)insertEditNode:(const BookmarkNode*)node
           atIndexPath:(NSIndexPath*)indexPath {
  if (_editNodes.find(node) != _editNodes.end())
    return;
  _editNodes.insert(node);
  _editNodesOrdered.push_back(node);
  if (indexPath) {
    [self.editIndexPaths addObject:indexPath];
  } else {
    // Insert null to keep the index valid.
    [self.editIndexPaths addObject:[NSNull null]];
  }
}

- (void)removeEditNode:(const BookmarkNode*)node
           atIndexPath:(NSIndexPath*)indexPath {
  if (_editNodes.find(node) == _editNodes.end())
    return;
  _editNodes.erase(node);
  std::vector<const BookmarkNode*>::iterator it =
      std::find(_editNodesOrdered.begin(), _editNodesOrdered.end(), node);
  DCHECK(it != _editNodesOrdered.end());
  _editNodesOrdered.erase(it);
  if (indexPath) {
    [self.editIndexPaths removeObject:indexPath];
  } else {
    // If we don't have the cell, we remove it by using its index.
    const NSUInteger index = std::distance(_editNodesOrdered.begin(), it);
    if (index < self.editIndexPaths.count) {
      [self.editIndexPaths removeObjectAtIndex:index];
    }
  }

  if (_editNodes.size() == 0)
    [self setEditing:NO animated:YES];
  else
    [self updateEditingStateAnimated:YES];
}

#pragma mark - private methods

- (void)loadURL:(const GURL&)url {
  if (url == GURL() || url.SchemeIs(url::kJavaScriptScheme))
    return;

  new_tab_page_uma::RecordAction(self.browserState,
                                 new_tab_page_uma::ACTION_OPENED_BOOKMARK);
  base::RecordAction(
      base::UserMetricsAction("MobileBookmarkManagerEntryOpened"));
  [self.loader loadURL:url
               referrer:web::Referrer()
             transition:ui::PAGE_TRANSITION_AUTO_BOOKMARK
      rendererInitiated:NO];
}

#pragma mark - Views

- (void)updateMenuViewLayout {
  LayoutRect menuLayout =
      LayoutRectMake(0, self.view.bounds.size.width, 0, self.menuWidth,
                     self.view.bounds.size.height);
  self.menuView.frame = LayoutRectGetRect(menuLayout);
}

- (void)loadBookmarkViews {
  [super loadBookmarkViews];
  DCHECK(self.bookmarks->loaded());

  self.menuView.delegate = self;
  self.folderView.delegate = self;

  [self moveMenuAndPrimaryViewToAdequateParent];

  // Load the last primary menu item which the user had active.
  BookmarkMenuItem* item = nil;
  CGFloat position = 0;
  BOOL found =
      bookmark_utils_ios::GetPositionCache(self.bookmarks, &item, &position);
  if (!found)
    item = [self.menuView defaultMenuItem];

  [self updatePrimaryMenuItem:item animated:NO];

  [[self primaryView] applyContentPosition:position];

  if (found) {
    // If the view has already been laid out, then immediately apply the content
    // position.
    if (self.view.window) {
      [[self primaryView] applyContentPosition:position];
    } else {
      // Otherwise, save the position to be applied once the view has been laid
      // out.
      self.cachedContentPosition = [NSNumber numberWithFloat:position];
    }
  }
}

- (void)updatePrimaryMenuItem:(BookmarkMenuItem*)menuItem
                     animated:(BOOL)animated {
  if (![self.view superview])
    return;

  [super updatePrimaryMenuItem:menuItem];

  [self moveMenuAndPrimaryViewToAdequateParent];

  // [self.view sendSubviewToBack:primaryView];
  [self refreshFrameOfPrimaryView];

  self.navigationBar.hidden = NO;
  [self updateNavigationBarAnimated:animated
                        orientation:GetInterfaceOrientation()];
  [self updateEditBarShadow];
}

- (BOOL)shouldPresentMenuInSlideInPanel {
  return IsCompactTablet();
}

- (CGFloat)primaryViewLeadingMargin {
  if ([self shouldPresentMenuInSlideInPanel])
    return 0;
  return [self menuWidth];
}

- (void)moveMenuAndPrimaryViewToAdequateParent {
  // Remove the menuView, panelView, and primaryView from the view hierarchy.
  if ([self.menuView superview])
    [self.menuView removeFromSuperview];
  if ([self.panelView superview])
    [self.panelView removeFromSuperview];
  UIView* primaryView = [self primaryView];
  if ([primaryView superview])
    [primaryView removeFromSuperview];

  if ([self shouldPresentMenuInSlideInPanel]) {
    // Add the panelView to the view hierarchy.
    [self.view addSubview:self.panelView];
    CGSize size = self.view.bounds.size;
    CGFloat navBarHeight = CGRectGetHeight([self navigationBarFrame]);
    LayoutRect panelLayout = LayoutRectMake(
        0, size.width, navBarHeight, size.width, size.height - navBarHeight);

    // Initialize the panelView with the menuView and the primaryView.
    [self.panelView setFrame:LayoutRectGetRect(panelLayout)];
    [self.panelView.menuView addSubview:self.menuView];
    self.menuView.frame = self.panelView.menuView.bounds;
    [self.panelView.contentView addSubview:primaryView];
  } else {
    [self.view addSubview:self.menuView];
    [self.view addSubview:primaryView];
  }

  // Make sure the navigation bar is the frontmost subview.
  [self.view bringSubviewToFront:self.navigationBar];
}

- (void)refreshFrameOfPrimaryView {
  [self primaryView].frame = [self frameForPrimaryView];
}

- (CGRect)frameForPrimaryView {
  CGFloat topInset = 0;
  if (!IsCompactTablet())
    topInset = CGRectGetHeight([self navigationBarFrame]);

  CGFloat leadingMargin = [self primaryViewLeadingMargin];
  CGSize size = self.view.bounds.size;
  LayoutRect primaryViewLayout =
      LayoutRectMake(leadingMargin, size.width, topInset,
                     size.width - leadingMargin, size.height - topInset);
  return LayoutRectGetRect(primaryViewLayout);
}

#pragma mark - Editing bar methods.

- (CGRect)editingBarFrame {
  return CGRectInset(self.navigationBar.frame, 24.0, 0);
}

- (void)updateEditingStateAnimated:(BOOL)animated {
  if (!self.editing) {
    [self hideEditingBarAnimated:animated];
    [self updateEditBarShadow];
    return;
  }

  if (!self.editingBar) {
    self.editingBar =
        [[BookmarkEditingBar alloc] initWithFrame:[self editingBarFrame]];
    [self.editingBar setCancelTarget:self action:@selector(editingBarCancel)];
    [self.editingBar setDeleteTarget:self action:@selector(editingBarDelete)];
    [self.editingBar setMoveTarget:self action:@selector(editingBarMove)];
    [self.editingBar setEditTarget:self action:@selector(editingBarEdit)];

    [self.view addSubview:self.editingBar];
    self.editingBar.hidden = YES;
  }

  int bookmarkCount = 0;
  int folderCount = 0;
  for (auto* node : _editNodes) {
    if (node->is_url())
      ++bookmarkCount;
    else
      ++folderCount;
  }
  [self.editingBar updateUIWithBookmarkCount:bookmarkCount
                                 folderCount:folderCount];

  [self showEditingBarAnimated:animated];
  [self updateEditBarShadow];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
  if (_editing == editing)
    return;

  _editing = editing;

  if (editing) {
    self.bookmarkPromoController.promoState = NO;
  } else {
    // Only reset the editing state when leaving edit mode. This allows
    // subclasses to add nodes for editing before entering edit mode.
    [self resetEditNodes];
    [self.bookmarkPromoController updatePromoState];
  }

  [self updateEditingStateAnimated:animated];
  if ([[self primaryMenuItem] supportsEditing])
    [[self primaryView] setEditing:editing animated:animated];
}

- (void)showEditingBarAnimated:(BOOL)animated {
  CGRect endFrame = [self editingBarFrame];
  if (self.editingBar.hidden) {
    CGRect startFrame = endFrame;
    startFrame.origin.y = -CGRectGetHeight(startFrame);
    self.editingBar.frame = startFrame;
  }
  self.editingBar.hidden = NO;
  [UIView animateWithDuration:animated ? 0.2 : 0
      delay:0
      options:UIViewAnimationOptionBeginFromCurrentState
      animations:^{
        self.editingBar.frame = endFrame;
      }
      completion:^(BOOL finished) {
        if (finished)
          self.navigationBar.hidden = YES;
      }];
}

- (void)hideEditingBarAnimated:(BOOL)animated {
  CGRect frame = [self editingBarFrame];
  if (!self.editingBar.hidden) {
    frame.origin.y = -CGRectGetHeight(frame);
  }
  self.navigationBar.hidden = NO;
  [UIView animateWithDuration:animated ? 0.2 : 0
      delay:0
      options:UIViewAnimationOptionBeginFromCurrentState
      animations:^{
        self.editingBar.frame = frame;
      }
      completion:^(BOOL finished) {
        if (finished)
          self.editingBar.hidden = YES;
      }];
}

- (void)updateEditBarShadow {
  [self.editingBar showShadow:self.editing];
}

#pragma mark Editing Bar Callbacks

- (void)editingBarCancel {
  [self setEditing:NO animated:YES];
}

- (void)editingBarMove {
  [self moveNodes:_editNodes];
}

- (void)editingBarDelete {
  [self deleteSelectedNodes];
  [self setEditing:NO animated:YES];
}

- (void)editingBarEdit {
  DCHECK_EQ(_editNodes.size(), 1u);
  const BookmarkNode* node = *(_editNodes.begin());
  [self editNode:node];
}

- (void)toggleMenuAnimated {
  if ([self.panelView userDrivenAnimationInProgress])
    return;

  if (self.panelView.showingMenu) {
    [self.panelView hideMenuAnimated:YES];
  } else {
    [self.panelView showMenuAnimated:YES];
  }
}

#pragma mark - BookmarkMenuViewDelegate

- (void)bookmarkMenuView:(BookmarkMenuView*)view
        selectedMenuItem:(BookmarkMenuItem*)menuItem {
  BOOL menuItemChanged = ![[self primaryMenuItem] isEqual:menuItem];
  [self toggleMenuAnimated];
  if (menuItemChanged) {
    [self setEditing:NO animated:YES];
    [self updatePrimaryMenuItem:menuItem animated:YES];
  }
}

#pragma mark - BookmarkCollectionViewDelegate
// This class owns multiple views that have a delegate that conforms to
// BookmarkCollectionViewDelegate, or a subprotocol of
// BookmarkCollectionViewDelegate.
- (void)bookmarkCollectionView:(BookmarkCollectionView*)view
                          cell:(UICollectionViewCell*)cell
             addNodeForEditing:(const BookmarkNode*)node {
  [self insertEditNode:node atIndexPath:[self indexPathForCell:cell]];
  [self updateEditingStateAnimated:YES];
}

- (void)bookmarkCollectionView:(BookmarkCollectionView*)view
                          cell:(UICollectionViewCell*)cell
          removeNodeForEditing:(const BookmarkNode*)node {
  [self removeEditNode:node atIndexPath:[self indexPathForCell:cell]];
}

- (const std::set<const BookmarkNode*>&)nodesBeingEdited {
  return _editNodes;
}

- (void)bookmarkCollectionViewDidScroll:(BookmarkCollectionView*)view {
  [self updateEditBarShadow];
}

- (void)bookmarkCollectionView:(BookmarkCollectionView*)view
      selectedUrlForNavigation:(const GURL&)url {
  [self cachePosition];
  // Before passing the URL to the block, make sure the block has a copy of the
  // URL and not just a reference.
  const GURL localUrl(url);
  dispatch_async(dispatch_get_main_queue(), ^{
    [self loadURL:localUrl];
  });
}

- (void)bookmarkCollectionView:(BookmarkCollectionView*)collectionView
          wantsMenuForBookmark:(const BookmarkNode*)node
                        onView:(UIView*)view
                       forCell:(BookmarkItemCell*)cell {
  DCHECK(!self.editViewController);
  DCHECK(!self.actionSheetCoordinator);
  self.actionSheetCoordinator = [[ActionSheetCoordinator alloc]
      initWithBaseViewController:self.view.window.rootViewController
                           title:nil
                         message:nil
                            rect:view.bounds
                            view:view];
  __weak BookmarkHomeTabletNTPController* weakSelf = self;

  // Select action.
  [self.actionSheetCoordinator
      addItemWithTitle:l10n_util::GetNSString(IDS_IOS_BOOKMARK_ACTION_SELECT)
                action:^{
                  [weakSelf selectFirstNode:node withCell:cell];
                  weakSelf.actionSheetCoordinator = nil;
                }
                 style:UIAlertActionStyleDefault];

  // Edit action.
  [self.actionSheetCoordinator
      addItemWithTitle:l10n_util::GetNSString(IDS_IOS_BOOKMARK_ACTION_EDIT)
                action:^{
                  [weakSelf editNode:node];
                  weakSelf.actionSheetCoordinator = nil;
                }
                 style:UIAlertActionStyleDefault];

  // Move action.
  [self.actionSheetCoordinator
      addItemWithTitle:l10n_util::GetNSString(IDS_IOS_BOOKMARK_ACTION_MOVE)
                action:^{
                  std::set<const BookmarkNode*> nodes;
                  nodes.insert(node);
                  [weakSelf moveNodes:nodes];
                  weakSelf.actionSheetCoordinator = nil;
                }
                 style:UIAlertActionStyleDefault];

  // Delete action.
  [self.actionSheetCoordinator
      addItemWithTitle:l10n_util::GetNSString(IDS_IOS_BOOKMARK_ACTION_DELETE)
                action:^{
                  std::set<const BookmarkNode*> nodes;
                  nodes.insert(node);
                  [weakSelf deleteNodes:nodes];
                  weakSelf.actionSheetCoordinator = nil;
                }
                 style:UIAlertActionStyleDestructive];

  // Cancel action.
  [self.actionSheetCoordinator
      addItemWithTitle:l10n_util::GetNSString(IDS_CANCEL)
                action:^{
                  weakSelf.actionSheetCoordinator = nil;
                }
                 style:UIAlertActionStyleCancel];

  [self.actionSheetCoordinator start];
}

- (void)bookmarkCollectionView:(BookmarkCollectionView*)view
              didLongPressCell:(UICollectionViewCell*)cell
                   forBookmark:(const BookmarkNode*)node {
  DCHECK(!self.editing);
  [self selectFirstNode:node withCell:cell];
}

- (BOOL)bookmarkCollectionViewShouldShowPromoCell:
    (BookmarkCollectionView*)collectionView {
  return self.bookmarkPromoController.promoState;
}

- (void)bookmarkCollectionViewShowSignIn:(BookmarkCollectionView*)view {
  [self.bookmarkPromoController showSignIn];
}

- (void)bookmarkCollectionViewDismissPromo:(BookmarkCollectionView*)view {
  [self.bookmarkPromoController hidePromoCell];
}

#pragma mark Action Sheet Callbacks

- (void)selectFirstNode:(const BookmarkNode*)node
               withCell:(UICollectionViewCell*)cell {
  DCHECK(!self.editing);
  [self insertEditNode:node atIndexPath:[self indexPathForCell:cell]];
  [self setEditing:YES animated:YES];
}

#pragma mark - BookmarkCollectionViewDelegate

- (void)bookmarkCollectionView:(BookmarkCollectionView*)view
    selectedFolderForNavigation:(const BookmarkNode*)folder {
  BookmarkMenuItem* menuItem = nil;
  if (view == self.folderView) {
    const BookmarkNode* parent = RootLevelFolderForNode(folder, self.bookmarks);
    menuItem =
        [BookmarkMenuItem folderMenuItemForNode:folder rootAncestor:parent];
  } else {
    NOTREACHED();
    return;
  }
  [self updatePrimaryMenuItem:menuItem animated:YES];
}

#pragma mark - Internal Utility Methods

- (void)deleteSelectedNodes {
  [self deleteNodes:_editNodes];
}

- (void)moveEditingNodesToFolder:(const BookmarkNode*)folder {
  // The UI only supports moving nodes when there are at least one selected.
  DCHECK_GE(_editNodes.size(), 1u);

  bookmark_utils_ios::MoveBookmarksWithUndoToast(_editNodes, self.bookmarks,
                                                 folder, self.browserState);
}

- (void)cachePosition {
  if ([self primaryView]) {
    bookmark_utils_ios::CachePosition(
        [[self primaryView] contentPositionInPortraitOrientation],
        [self primaryMenuItem]);
  }
}

#pragma mark - Navigation bar

- (CGRect)navigationBarFrame {
  return CGRectMake(0, 0, CGRectGetWidth(self.view.bounds),
                    [BookmarkNavigationBar expectedContentViewHeight] +
                        kNavigationBarTopMargin);
}

- (void)updateNavigationBarAnimated:(BOOL)animated
                        orientation:(UIInterfaceOrientation)orientation {
  CGFloat duration = animated ? bookmark_utils_ios::menuAnimationDuration : 0;
  [self updateNavigationBarWithDuration:duration orientation:orientation];
}

- (void)updateNavigationBarWithDuration:(CGFloat)duration
                            orientation:(UIInterfaceOrientation)orientation {
  [self.navigationBar setTitle:[self.primaryMenuItem titleForNavigationBar]];
  if ([self shouldShowEditButton])
    [self.navigationBar showEditButtonWithAnimationDuration:duration];
  else
    [self.navigationBar hideEditButtonWithAnimationDuration:duration];

  if ([self shouldShowBackButton])
    [self.navigationBar showBackButtonInsteadOfMenuButton:duration];
  else
    [self.navigationBar showMenuButtonInsteadOfBackButton:duration];
}

- (BOOL)shouldShowEditButton {
  if (self.primaryMenuItem.type != bookmarks::MenuItemFolder)
    return NO;
  // The type is MenuItemFolder, so it is safe to access |folder|.
  return !self.bookmarks->is_permanent_node(self.primaryMenuItem.folder);
}

- (BOOL)shouldShowBackButton {
  if (self.primaryMenuItem.type != bookmarks::MenuItemFolder)
    return NO;
  // The type is MenuItemFolder, so it is safe to access |folder|.
  const BookmarkNode* folder = self.primaryMenuItem.folder;
  // Show the back button iff the folder or its immediate parent is a permanent
  // primary folder.
  BOOL isTopFolder = IsPrimaryPermanentNode(folder, self.bookmarks) ||
                     IsPrimaryPermanentNode(folder->parent(), self.bookmarks);
  return !isTopFolder;
}

#pragma mark Navigation Bar Callbacks

- (void)navigationBarBack:(id)sender {
  DCHECK([self shouldShowBackButton]);

  // Go to the parent folder.
  DCHECK(self.primaryMenuItem.type == bookmarks::MenuItemFolder);
  const BookmarkNode* parentFolder = self.primaryMenuItem.folder->parent();
  const BookmarkNode* rootAncestor =
      RootLevelFolderForNode(parentFolder, self.bookmarks);
  BookmarkMenuItem* menuItem =
      [BookmarkMenuItem folderMenuItemForNode:parentFolder
                                 rootAncestor:rootAncestor];
  [self updatePrimaryMenuItem:menuItem animated:YES];
}

#pragma mark - NewTabPagePanelProtocol

- (void)reload {
}

- (void)wasShown {
  [self.folderView wasShown];
}

- (void)wasHidden {
  [self cachePosition];
  [self.folderView wasHidden];
}

- (void)dismissModals {
  [self.actionSheetCoordinator stop];
  self.actionSheetCoordinator = nil;
  [self.editViewController dismiss];
}

- (void)dismissKeyboard {
  // Uses self.view directly instead of going throught self.view to
  // avoid creating the view hierarchy unnecessarily.
  [self.view endEditing:YES];
}

- (void)setScrollsToTop:(BOOL)enabled {
  self.scrollToTop = enabled;
  [[self primaryView] setScrollsToTop:self.scrollToTop];
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.view.backgroundColor = bookmark_utils_ios::mainBackgroundColor();
  self.navigationBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;

  [self.navigationBar setEditTarget:self
                             action:@selector(navigationBarWantsEditing:)];
  [self.navigationBar setBackTarget:self action:@selector(navigationBarBack:)];

  [self.navigationBar setMenuTarget:self action:@selector(toggleMenuAnimated)];

  [self.view addSubview:self.navigationBar];

  if (self.bookmarks->loaded())
    [self loadBookmarkViews];
  else
    [self loadWaitingView];
}

- (CGFloat)alphaForBottomShadow {
  return 0;
}

#pragma mark - BookmarkModelBridgeObserver

- (void)bookmarkModelLoaded {
  if (!self.view)
    return;

  DCHECK(self.waitForModelView);
  __weak BookmarkHomeTabletNTPController* weakSelf = self;
  [self.waitForModelView stopWaitingWithCompletion:^{
    BookmarkHomeTabletNTPController* strongSelf = weakSelf;
    // Early return if the controller has been deallocated.
    if (!strongSelf)
      return;
    [UIView animateWithDuration:0.2
        animations:^{
          strongSelf.waitForModelView.alpha = 0.0;
        }
        completion:^(BOOL finished) {
          [strongSelf.waitForModelView removeFromSuperview];
          strongSelf.waitForModelView = nil;
        }];
    [strongSelf loadBookmarkViews];
  }];
}

- (void)bookmarkNodeChanged:(const BookmarkNode*)bookmarkNode {
  // The title of the folder may have changed.
  if (self.primaryMenuItem.type == bookmarks::MenuItemFolder &&
      self.primaryMenuItem.folder == bookmarkNode) {
    UIInterfaceOrientation orient = GetInterfaceOrientation();
    [self updateNavigationBarAnimated:NO orientation:orient];
  }
}

- (void)bookmarkNodeChildrenChanged:(const BookmarkNode*)bookmarkNode {
  // The node has not changed, but the ordering and existence of its children
  // have changed.
}

- (void)bookmarkNode:(const BookmarkNode*)bookmarkNode
     movedFromParent:(const BookmarkNode*)oldParent
            toParent:(const BookmarkNode*)newParent {
  // The node has moved to a new parent folder.
}

- (void)bookmarkNodeDeleted:(const BookmarkNode*)node
                 fromFolder:(const BookmarkNode*)folder {
  [self removeEditNode:node atIndexPath:nil];
}

- (void)bookmarkModelRemovedAllNodes {
  // All non-permanent nodes have been removed.
  [self setEditing:NO animated:YES];
}

- (NSIndexPath*)indexPathForCell:(UICollectionViewCell*)cell {
  DCHECK([self primaryView].collectionView);
  NSIndexPath* indexPath =
      [[self primaryView].collectionView indexPathForCell:cell];
  return indexPath;
}

@end
