// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/public/event_constants.h"

namespace feature_engagement_tracker {

namespace events {

#if defined(OS_WIN) || defined(OS_LINUX)
const char kOmniboxInteraction[] = "omnibox_used";

const char kHistoryDeleted[] = "history_deleted";
const char kIncognitoWindowOpened[] = "incognito_window_opened";

const char kSessionTime[] = "session_time";
#endif  // defined(OS_WIN) || defined(OS_LINUX)

#if defined(OS_WIN) || defined(OS_LINUX) || defined(OS_IOS)
const char kNewTabOpened[] = "new_tab_opened";
#endif  // defined(OS_WIN) || defined(OS_LINUX) || defined(OS_IOS)

#if defined(OS_IOS)
const char kChromeOpened[] = "chrome_opened";
const char kIncognitoTabOpened[] = "incognito_tab_opened";
const char kClearedBrowsingData[] = "cleared_browsing_data";
const char kAddedItemToReadingList[] = "added_item_to_reading_list";
const char kViewedReadingList[] = "viewed_reading_list";
const char kOpenedReadingListItem[] = "opened_reading_list_item";
#endif  // defined(OS_IOS)

}  // namespace events

}  // namespace feature_engagement_tracker
