/*
 * result.cxx
 * Copyright© 2017 rsw0x
 *
 * Distributed under terms of the MPLv2 license.
 */

#define BACKWARD_HAS_BFD 1
#include "../result.hpp"

#include "doctest.h"

#include <string>
#include <vector>

using util::Ok;
using util::Err;
using namespace std::string_literals;

struct SBN {};

void foobar();
util::Result<int, void*> fungun();
void funbun();

void goobar(util::Result<int, SBN>);

util::Result<int, void*> fungun() {
  return Err(nullptr);
}

void foobar() {
  util::Result<int, SBN> res{Ok(0)};
  const int i = 0;
  util::Result<int, SBN> res2{Ok(i)};
  util::Result<int, SBN> res3{Ok(0.0)};
  util::Result<int, SBN> res4{Err(SBN{})};

  // constexpr util::Result<int, SBN> r1{Ok(0)};
  // res4 = Ok(0);
  // // Should be rejected.
  // res4 = Err(SBN{});

  res4.apply([](int&) { return 4; });
  res4.apply([](int) { return 4; });
  res4.ok();
  res4.ok_or(0.0);
  std::move(res4).ok_or([]() { return 0.0; });
  int i2 = res4.ok_or(i);
  int i3 = res4.ok_or(0);
  res4 = Ok(0);
  (void)i3;
  (void)i2;
  fungun();
  // goobar(Ok(0));
  //
  // util::Result<int, int> res_{Err(0)};

  util::Result<const int, void*> res_5{Ok(0)};
  // res_5 = Ok(5);
}

struct SBF {
  constexpr SBF() = default;
  constexpr SBF(SBF&&) = default;
  constexpr SBF& operator=(SBF&&) = default;
  SBF(const SBF&) = delete;
  SBF& operator=(const SBF&) = delete;
};

struct SBF2 {
  SBF2() = default;
  SBF2(SBF2&&) = default;
  SBF2& operator=(SBF2&&) = default;
  SBF2(const SBF2&) = default;
  SBF2& operator=(const SBF2&) = default;
};

struct SBF3 {
  SBF3() = default;
  SBF3(SBF3&&) = delete;
  SBF3& operator=(SBF3&&) = delete;
  SBF3(const SBF3&) = default;
  SBF3& operator=(const SBF3&) = default;
};

void funbun() {
  // auto r = Ok(SBF{});
  Err(SBF{});
  SBF s;
  util::Result<void*, SBF> r = Err(std::move(s));
  util::Result<SBF, void*> r2 = Ok(std::move(s));
  // Result<std::reference_wrapper<SBF>, void*> r2_ = Ok(s);
  // Result<SBF&, void*> r2_ = Ok(s);
  // r2.ok_or(SBF{});
  std::move(r2).ok_or(SBF{});

  util::Result<SBF2, void*> r3 = Err(nullptr);
  r3.ok_or(SBF2{});

  r.ok();
  r2.ok();
  r3.ok();

  util::Result<SBF, void*> r4 = Err(nullptr);
  (void)r4;
  r4 = Ok(SBF{});
  // constexpr Result<SBF, void*> r_5 = Err(nullptr);

  util::Result<std::vector<int>, void*> r5 = Err(nullptr);
  r4.ok();
  // Result<const SBF, void*>{Err(nullptr)}.ok();

  Ok(SBF2{});
}

static util::Result<std::string, int> loadBigFile() {
  return Err(0);
}

TEST_CASE("Result") {
  util::Result<int, void*> r{Ok(5)};
  CHECK(r);
  r = Err(nullptr);
  CHECK(!r);

  auto r2 = r.apply([](int) { return 0; });
  CHECK(!r);
  CHECK(!r2);

  r = Ok(11);
  CHECK(r);
  CHECK(r.ok() == 11);

  CHECK(!loadBigFile());

  util::Result<SBF, void*> s{Ok(SBF{})};
  // Shouldn't compile.
  // auto s2 = s;
  auto s2 = std::move(s);
  // Shouldn't compile.
  // Result<SBF, void*> s3{s};
  util::Result<SBF, void*> s3{std::move(s2)};

  SBF sbf;
  util::Result<SBF&, void*> s4{Ok(sbf)};

  util::Result<const SBF2, void*> s5{Ok(SBF2{})};
}

namespace {
struct moveonly_t {
  moveonly_t() = default;
  moveonly_t(moveonly_t&&) = default;
  moveonly_t& operator=(moveonly_t&&) = default;
  moveonly_t(const moveonly_t&) = delete;
  moveonly_t& operator=(const moveonly_t&) = delete;
};

struct copyonly_t {
  copyonly_t() = default;
  // copyonly_t(copyonly_t&&) = delete;
  // copyonly_t& operator=(copyonly_t&&) = delete;
  copyonly_t(const copyonly_t&) = default;
  copyonly_t& operator=(const copyonly_t&) = default;
};

struct movecopy_t {
  movecopy_t() = default;
  movecopy_t(movecopy_t&&) = default;
  movecopy_t& operator=(movecopy_t&&) = default;
  movecopy_t(const movecopy_t&) = default;
  movecopy_t& operator=(const movecopy_t&) = default;
};
} // namespace

template <typename T>
void opTest() {
  using result_t = util::Result<T, void*>;
  using result_e = util::Result<void*, T>;

  result_t r0{Ok(T{})};
  r0 = Ok(T{});
  r0 = Err(nullptr);

  result_e r1{Err(T{})};
  r1 = Err(T{});
  r1 = Ok(nullptr);

  T v{};
  Ok(v);
  Err(v);

  // FAILS: For some <T> without copy-ctor.
  // util::Result<T, void*> r2 = Ok(v);
  util::Result<T&, void*> r2 = Ok(v);
  (void)r2;
  util::Result<void*, T&> r3 = Err(v);
  (void)r3;
}

TEST_CASE("generic op test") {
  opTest<moveonly_t>();
  opTest<copyonly_t>();
  opTest<movecopy_t>();

  {
    // FAILS: attempted to create a non-ref Result by reference.
    // moveonly_t v{};
    // util::Result<moveonly_t, void*> r2 = Ok(v);
  }

  util::Result<int&, void*> r = Err(nullptr);
  CHECK(!r);
  CHECK(r.err() == nullptr);
  int i = 0;
  r = Ok(i);
  CHECK(r);
  CHECK(r.ok() == i);
  CHECK(static_cast<int*>(&r.ok()) == static_cast<int*>(&i));
}

static util::Result<copyonly_t, void*> applyRetTest() {
  return {Ok(copyonly_t{})};
}

TEST_CASE("Apply test") {
  loadBigFile().apply([](std::string&) { return std::string{}; });
  loadBigFile().apply([](std::string) { return std::string{}; });
  loadBigFile().apply([](const std::string) { return std::string{}; });
  loadBigFile().apply([](const std::string&) { return std::string{}; });

  applyRetTest().apply([](copyonly_t&) { return ""; });

  std::string s = loadBigFile().ok_or([]() { return "foo"s; });
  CHECK(s == "foo"s);

  util::Result<int, void*> r = Err(nullptr);
  int i = r.ok_or(0.0);
  (void)i;

  r.ok_or([]{ return 0; });
}


struct P{
  // P(const S&) {  }
};

struct S{
  operator P() { return P{}; }
};


static util::Result<std::string, P> gunfun(){
  return Err(S{});
}

// static util::Result<std::string, int> try_(){
//   // std::string s =  Try_(loadBigFile());
//
//   // return Err(0);
// }

// TEST_CASE("Try_"){
//   try_();
//   gunfun();
// }
