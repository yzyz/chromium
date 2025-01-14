// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/discovery_network_list_wifi.h"

#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <linux/wireless.h>

#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "net/base/network_interfaces_linux.h"

bool MaybeGetWifiSSID(const std::string& if_name, std::string* ssid_out) {
  DCHECK(ssid_out);

  base::ScopedFD ioctl_socket(socket(AF_INET, SOCK_DGRAM, 0));
  if (!ioctl_socket.is_valid())
    return false;
  struct iwreq wreq = {};
  strncpy(wreq.ifr_name, if_name.data(), IFNAMSIZ - 1);

  char ssid[IW_ESSID_MAX_SIZE + 1] = {0};
  wreq.u.essid.pointer = ssid;
  wreq.u.essid.length = IW_ESSID_MAX_SIZE;
  if (ioctl(ioctl_socket.get(), SIOCGIWESSID, &wreq) == -1) {
    return false;
  }
  if (ssid[0] != 0) {
    ssid_out->assign(ssid);
    return true;
  }
  return false;
}
