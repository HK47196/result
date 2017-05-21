/*
 * one.cxx
 * Copyright© 2017 rsw0x
 *
 * Distributed under terms of the MPLv2 license.
 */

#include "../result.hpp"

namespace {
  struct moveonly_t{
    moveonly_t(moveonly_t&&) = default;
    moveonly_t(const moveonly_t&) = delete;
    moveonly_t& operator=(moveonly_t&&) = default;
    moveonly_t& operator=(const moveonly_t&) = delete;
  };

  void foo() {
    moveonly_t c{};
    util::Result<void*, moveonly_t> r = util::Err(c);
  }
}
