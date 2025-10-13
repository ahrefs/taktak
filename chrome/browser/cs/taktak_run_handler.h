// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef TAKTAK_RUN_HANDLER_H
#define TAKTAK_RUN_HANDLER_H

#include <memory>
#include "base/memory/scoped_refptr.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace base {
class FilePath;
}  // namespace base

namespace taktak_run_handler {
void Handle(scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
}
#endif  // TAKTAK_RUN_HANDLER_H
