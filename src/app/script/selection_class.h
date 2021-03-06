// Aseprite
// Copyright (C) 2015-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_SELECTION_CLASS_H_INCLUDED
#define APP_SCRIPT_SELECTION_CLASS_H_INCLUDED
#pragma once

#include "script/engine.h"

namespace app {

  void register_selection_class(script::index_t idx, script::Context& ctx);

} // namespace app

#endif
