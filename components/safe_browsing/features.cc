// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/features.h"

#include <stddef.h>
#include <algorithm>
#include <utility>
#include <vector>
#include "base/feature_list.h"

#include "base/macros.h"
#include "base/values.h"
namespace safe_browsing {
// Please define any new SafeBrowsing related features in this file, and add
// them to  the ExperimentalFeaturesList below to start displaying their status
// on the  chrome://safe-browsing page.
const base::Feature kLocalDatabaseManagerEnabled{
    "SafeBrowsingV4LocalDatabaseManagerEnabled",
    base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kV4OnlyEnabled{"SafeBrowsingV4OnlyEnabled",
                                   base::FEATURE_DISABLED_BY_DEFAULT};
// This Feature specifies which non-resource HTML Elements to collect based on
// their tag and attributes. It's a single param containing a comma-separated
// list of pairs. For example: "tag1,id,tag1,height,tag2,foo" - this will
// collect elements with tag "tag1" that have attribute "id" or "height" set,
// and elements of tag "tag2" if they have attribute "foo" set. All tag names
// and attributes should be lower case.
const base::Feature kThreatDomDetailsTagAndAttributeFeature{
    "ThreatDomDetailsTagAttributes", base::FEATURE_DISABLED_BY_DEFAULT};

namespace {
// List of experimental features. Boolean value for each list member should be
// set to True if the experiment is currently running at a probability other
// than 1 or 0, or to False otherwise.
std::vector<std::pair<const base::Feature*, bool>>

GetExperimentalFeaturesList() {
  std::vector<std::pair<const base::Feature*, bool>> experimental_features_list{
      std::make_pair(&kLocalDatabaseManagerEnabled, true),
      std::make_pair(&kV4OnlyEnabled, true),
      std::make_pair(&kThreatDomDetailsTagAndAttributeFeature, false)};

  return experimental_features_list;
}

// Adds the name and the enabled/disabled status of a given feature.
void AddFeatureAndAvailability(const base::Feature* exp_feature,
                               base::ListValue* param_list) {
  param_list->GetList().push_back(base::Value(exp_feature->name));
  if (base::FeatureList::IsEnabled(*exp_feature)) {
    param_list->GetList().push_back(base::Value("Enabled"));
  } else {
    param_list->GetList().push_back(base::Value("Disabled"));
  }
}
}  // namespace

// Returns the list of the experimental features that are enabled or disabled,
// as part of currently running Safe Browsing experiments.
base::ListValue GetFeatureStatusList() {
  std::vector<std::pair<const base::Feature*, bool>> features_list =
      GetExperimentalFeaturesList();

  base::ListValue param_list;
  for (std::vector<std::pair<const base::Feature*, bool>>::iterator it =
           features_list.begin();
       it != features_list.end(); ++it) {
    if ((*it).second) {
      AddFeatureAndAvailability((*it).first, &param_list);
    }
  }
  return param_list;
}

}  // namespace safe_browsing
