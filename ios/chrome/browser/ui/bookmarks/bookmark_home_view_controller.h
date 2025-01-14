// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BOOKMARKS_HOME_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_BOOKMARKS_HOME_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#include <set>
#include <vector>

@protocol UrlLoader;
@protocol BookmarkHomePrimaryView;

namespace ios {
class ChromeBrowserState;
}  // namespace ios

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}  // namespace bookmarks

@class BookmarkCollectionView;
@class BookmarkEditViewController;
@class BookmarkFolderEditorViewController;
@class BookmarkFolderViewController;
@class BookmarkHomeWaitingView;
@class BookmarkMenuItem;
@class BookmarkMenuView;
@class BookmarkNavigationBar;
@class BookmarkPanelView;
@class BookmarkPromoController;

// Class to navigate the bookmark hierarchy, needs subclassing for tablet /
// handset case.
@interface BookmarkHomeViewController : UIViewController

- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithLoader:(id<UrlLoader>)loader
                  browserState:(ios::ChromeBrowserState*)browserState
    NS_DESIGNATED_INITIALIZER;

// The bookmark model used.
@property(nonatomic, assign, readonly) bookmarks::BookmarkModel* bookmarks;

// The user's browser state model used.
@property(nonatomic, assign, readonly) ios::ChromeBrowserState* browserState;

// The main view showing all the bookmarks.
@property(nonatomic, strong, readonly) BookmarkCollectionView* folderView;

// The view controller used to pick a folder in which to move the selected
// bookmarks.
@property(nonatomic, strong, readonly)
    BookmarkFolderViewController* folderSelector;

// Object to load URLs.
@property(nonatomic, weak, readonly) id<UrlLoader> loader;

// The menu with all the folders.
@property(nonatomic, strong, readonly) BookmarkMenuView* menuView;

// The navigation bar sits on top of the main content.
@property(nonatomic, strong, readonly) BookmarkNavigationBar* navigationBar;

// At any point in time, there is exactly one collection view whose view is part
// of the view hierarchy. This property determines what data is visible in the
// collection view.
@property(nonatomic, strong, readonly) BookmarkMenuItem* primaryMenuItem;

// This view holds a content view, and a menu view.
@property(nonatomic, strong, readonly) BookmarkPanelView* panelView;

// Either the menu or the primaryView can scrollToTop.
@property(nonatomic, assign) BOOL scrollToTop;

// This view is created and used if the model is not fully loaded yet by the
// time this controller starts. Property is readwrite, so that subclasses can
// set it to nil, once finished with it.
@property(nonatomic, strong) BookmarkHomeWaitingView* waitForModelView;

// The view controller used to view and edit a single bookmark.
@property(nonatomic, strong, readonly)
    BookmarkEditViewController* editViewController;

// The view controller to present when editing the current folder.
@property(nonatomic, strong, readonly)
    BookmarkFolderEditorViewController* folderEditor;

// This method should be called at most once in the life-cycle of the class.
// It should be called at the soonest possible time after the view has been
// loaded, and the bookmark model is loaded.
- (void)loadBookmarkViews;

// Returns the width of the menu.
- (CGFloat)menuWidth;

// This method is called if the view needs to be loaded and the model is not
// ready yet.
- (void)loadWaitingView;

// Updates the property 'primaryMenuItem'.
// Updates the UI to reflect the new state of 'primaryMenuItem'.
- (void)updatePrimaryMenuItem:(BookmarkMenuItem*)menuItem;

// The active collection view that corresponds to primaryMenuItem.
- (UIView<BookmarkHomePrimaryView>*)primaryView;

// The controller managing the display of the promo cell and the promo view
// controller.
@property(nonatomic, strong, readonly)
    BookmarkPromoController* bookmarkPromoController;

#pragma mark - Navigation bar callbacks.

// Called when the edit button is pressed on the navigation bar.
- (void)navigationBarWantsEditing:(id)sender;

#pragma mark - Action sheet callbacks

// Opens the folder move editor for the given node.
- (void)moveNodes:(const std::set<const bookmarks::BookmarkNode*>&)nodes;

// Deletes the current node.
- (void)deleteNodes:(const std::set<const bookmarks::BookmarkNode*>&)nodes;

- (void)editNode:(const bookmarks::BookmarkNode*)node;
@end

#endif  // IOS_CHROME_BROWSER_UI_BOOKMARKS_HOME_VIEW_CONTROLLER_H_
