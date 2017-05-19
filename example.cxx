/*
 * example.cxx
 * Copyright© 2017 rsw0x
 *
 * Distributed under terms of the MPLv2 license.
 */

#include <fstream>
#include "result.hpp"
#include <cstdio>
#include <cassert>
#include <string>

struct ThreadError{
  void context(const char* msg){
    std::printf("context.\n");
  }

  ~ThreadError(){
    // asm("int3");
    std::printf("Thread error dtor.\n");
  }

};

struct NetworkingError{
  operator ThreadError(){
    return ThreadError{};
  }
};

util::Result<int, ThreadError> foobar(){
  //NetworkingError has a conversion operator, Result will happily convert it for us.
  return {NetworkingError{}};
  // return util::Err(NetworkingError{});
  // return util::Err<ThreadError>({});
}

util::Result<int, ThreadError> fungun(){
  Try_(foobar() );

  return util::Ok(5);
}

util::Result<int, ThreadError> gunfun(){
  return util::Ok(5);
}
util::Result<int, ThreadError> bunfun(){
  return 5;
}

struct ReadError{  };

util::Result<std::fstream, ReadError> openFile(const char* fName){
  if(std::fstream f{fName}){
    //type deduction into Ok type, std::move because fstream requires a move constructor
    return std::move(f);
  }
  // Err() can be empty if the error can be default constructed to make a common usage easier.
  return util::Err();
}

util::Result<std::string, ReadError> readFileTest(){
  auto file = Try_( openFile("conf.ini"));
  file.seekg(0, std::ios::end);
  size_t end = file.tellg();
  file.seekg(0);
  std::string buf;
  buf.resize(end);
  file.read(&buf[0], end);
  return util::Ok(std::move(buf));
}

struct S_{

};

struct S2_{
  explicit operator S_(){
    return S_{};
  }
};

int main(){
  int i = static_cast<const int&>(3.4);
  std::printf("int:%d\n", i);


  auto _ = foobar();
  std::printf("foobar done\n");
  auto contents = readFileTest().ok_or("No file to read.");
  std::printf("File contents:\n%s", contents.c_str());
  std::printf(".\n");
  // auto i2 = foobar();
  auto i2{foobar().context("foobar failed.").context("")};
  std::printf("..\n");
  // i2.context("");
  std::printf("...\n");
  //
  if(!fungun().is_err()){
    assert(0);
  }
}
