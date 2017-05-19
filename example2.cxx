/*
 * example2.cxx
 * Copyright© 2017 rsw0x
 *
 * Distributed under terms of the MPLv2 license.
 */

#include "utils.hpp"
#include <cstdio>


int main(){
  auto ret = util::open("conf.ini", util::openmode::in);
  if(!ret){
    std::printf("Failed to load conf.ini\n");
    return 0;
  }
  auto fPtr = std::move(ret.ok());
  std::printf("file ptr:%p\n", fPtr.get());

  std::string contents = util::as_string(fPtr).ok("Failed to read file.");
  std::printf("File contents:\n%s", contents.c_str());

}
