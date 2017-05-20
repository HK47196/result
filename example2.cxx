/*
 * example2.cxx
 * Copyright© 2017 rsw0x
 *
 * Distributed under terms of the MPLv2 license.
 */

#include "utils.hpp"
#include <cstdio>

static void two() {
  std::string contents = util::open("conf.ini", util::openmode::in)
    .context("Failed to load conf.ini")
    .apply(util::as_string)
    .ok("Failed to read file.");

  std::printf("File contents:\n%s", contents.c_str());
}

static void one() {
  auto fPtr =
    util::open("conf.ini", util::openmode::in).ok("Failed to load conf.ini");
  std::printf("file ptr:%p\n", fPtr.get());

  std::string contents = util::as_string(fPtr).ok("Failed to read file.");
  std::printf("File contents:\n%s", contents.c_str());
}

int main() {
  two();
}
