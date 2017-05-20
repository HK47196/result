/*
 * apply.cxx
 * Copyright© 2017 rsw0x
 *
 * Distributed under terms of the MPLv2 license.
 */
#define BACKWARD_HAS_BFD 1
#include "../result.hpp"

#include "doctest.h"

struct test_error{  };

template<typename T>
using TestError = util::Result<T, test_error>;

static TestError<int> foo(){
  return 5;
}

static int bar(int){
  return 7;
}

static TestError<double> gun(int){
  return 0.0;
}

static TestError<double> bun(int){
  return test_error{};
}

static TestError<double> hun(int i){
  double l = Try_( bun(i) );
  return l * 5;
}

static TestError<double> foob(int&){
  return test_error{};
}

static TestError<double> foo2(int&&){
  return test_error{};
}

static void foo3(int& i){
  i *= 3;
}

TEST_CASE("APPLY"){
  CHECK(foo().ok() == 5);
  CHECK(foo().apply(bar).ok() == 7);
  CHECK(foo().apply(gun).is_ok());
  CHECK(foo().apply(bun).is_err());
  CHECK(foo().apply(hun).is_err());
  CHECK(foo().apply(foob).is_err());
  CHECK(foo().apply(foo2).is_err());
  CHECK(foo().apply(bar).apply(bar).apply(bar).apply(bar).apply(bar).ok() == 7);

  auto r = foo();
  r.apply(bar);
  std::move(r).apply(foo2);

  CHECK(foo().apply(foo3).apply(foo3).ok() == 45);
}
