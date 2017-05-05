/*
 * result.hpp
 * Copyright© 2017 rsw0x
 *
 * Distributed under terms of the MPLv2 license.
 */

#pragma once

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <type_traits>
#include <utility>

namespace util {
namespace details {
  template <class...>
  using void_t = void;

  template <typename Expr, typename Enabler = void>
  struct isCallableImpl : std::false_type {};

  template <typename F, typename... Args>
  struct isCallableImpl<F(Args...), void_t<std::result_of_t<F(Args...)>>> : std::true_type {};

  template <typename Expr>
  constexpr bool isCallable = isCallableImpl<Expr>::value;

  template <typename T>
  struct OkWrapper {
    T&& contents;

    constexpr OkWrapper(const OkWrapper& other)
      : contents(std::forward<T>(other.contents)) {}
    constexpr OkWrapper(T&& v)
      : contents(std::forward<T>(v)) {}
  };

  template <typename E>
  struct ErrWrapper {
    E&& contents;

    constexpr ErrWrapper(const ErrWrapper& other)
      : contents(std::forward<E>(other.contents)) {}
    constexpr ErrWrapper(E&& v)
      : contents(std::forward<E>(v)) {}
  };

#ifdef __GNUC__
  [[noreturn]] inline void unreachable() { __builtin_unreachable(); }
#else
  inline void unreachable() {}
#endif

  template <typename T, typename = void>
  struct wrapper;

  template <typename T>
  struct wrapper<T, typename std::enable_if_t<std::is_move_constructible<T>::value>> {
    using type = T;
  };

  // We rewrite rvalue ref types without move ctors into const lvalue references so that it copies the object instead of
  // moving it
  template <typename T>
  struct wrapper<T, typename std::enable_if_t<!std::is_move_constructible<T>::value>> {
    using type = const std::remove_reference_t<T>&;
  };

  template <typename T>
  using wrapper_t = typename wrapper<T>::type;

  struct ok_tag {};
  struct err_tag {};

  // Used as the 'invalid' state
  struct dummy_t {};
  enum class ValidityState : char { invalid = 0, err = 1, ok = 2 };

  template <typename T>
  using fix_ref_t =
    std::conditional_t<std::is_lvalue_reference<T>::value, std::reference_wrapper<std::remove_reference_t<T>>, T>;

  template <typename T, typename E>
  struct ResultBasePOD {
    union contents_t {
      fix_ref_t<T> val;
      fix_ref_t<E> err;
      dummy_t dummy_;

      explicit constexpr contents_t(dummy_t) noexcept
        : dummy_() {}

      template <typename U>
      explicit constexpr contents_t(U&& v, ok_tag)
        : val(std::forward<U>(v)) {}

      template <typename U>
      explicit constexpr contents_t(U&& e, err_tag)
        : err(std::forward<U>(e)) {}

      ~contents_t() = default;
    } contents;
    ValidityState validityState_ = ValidityState::invalid;

    explicit constexpr ResultBasePOD(dummy_t) noexcept
      : contents(dummy_t{}) {}

    template <typename U>
    explicit constexpr ResultBasePOD(details::OkWrapper<U> val) noexcept
      : contents(std::forward<U>(val.contents), ok_tag{})
      , validityState_(ValidityState::ok) {}

    template <typename U>
    explicit constexpr ResultBasePOD(details::ErrWrapper<U> val) noexcept
      : contents(std::forward<U>(val.contents), err_tag{})
      , validityState_(ValidityState::err) {}

    ~ResultBasePOD() = default;
    constexpr void destruct() { validityState_ = ValidityState::invalid; }
  };

  template <typename T, typename E>
  struct ResultBase {
    union contents_t {
      fix_ref_t<T> val;
      fix_ref_t<E> err;
      dummy_t dummy_;

      explicit constexpr contents_t(dummy_t) noexcept
        : dummy_() {}

      explicit constexpr contents_t(T&& v, ok_tag)
        : val(std::forward<T>(v)) {}
      explicit constexpr contents_t(E&& e, err_tag)
        : err(std::forward<E>(e)) {}

      ~contents_t() {}
    } contents;
    ValidityState validityState_ = ValidityState::invalid;

    explicit constexpr ResultBase(dummy_t) noexcept
      : contents(dummy_t{}) {}

    template <typename U>
    explicit constexpr ResultBase(details::OkWrapper<U> val) noexcept
      : contents(static_cast<T>(std::move(val.contents)), ok_tag{})
      , validityState_(ValidityState::ok) {}

    template <typename U>
    explicit constexpr ResultBase(details::ErrWrapper<U> val) noexcept
      : contents(static_cast<E>(std::move(val.contents)), err_tag{})
      , validityState_(ValidityState::err) {}

    ~ResultBase() { destruct(); }

    void destruct() {
      switch (validityState_) {
        case ValidityState::ok:
          contents.val.~T();
          break;
        case ValidityState::err:
          contents.err.~E();
          break;
        case ValidityState::invalid:
          break;
      }
      validityState_ = ValidityState::invalid;
    }
  };

  template <typename T, typename E>
  using BaseResult =
    typename std::conditional_t<std::is_trivially_destructible<T>::value and std::is_trivially_destructible<E>::value,
                                ResultBasePOD<T, E>,
                                ResultBase<T, E>>;

  template <typename U, typename T>
  constexpr bool validRef() {
    if ((!std::is_reference<T>::value) or (std::is_lvalue_reference<T>::value and std::is_same<T, U>::value)) {
      return true;
    }
    return false;
  }
} // namespace details

// Lightweight wrapper just meant for return type deduction.
template <typename T>
constexpr details::OkWrapper<details::wrapper_t<T>> Ok(T&& val) {
  static_assert(sizeof(details::OkWrapper<T>) == sizeof(void*), "");
  return {std::forward<T>(val)};
}

template <typename T>
constexpr details::ErrWrapper<details::wrapper_t<T>> Err(T&& val) {
  static_assert(sizeof(details::ErrWrapper<T>) == sizeof(void*), "");
  return {std::forward<T>(val)};
}

template <typename T, typename E>
struct Result : private details::BaseResult<T, E> {
  static_assert(!std::is_rvalue_reference<T>::value, "Result<T,E> can't hold rvalue references.");
  static_assert(!std::is_rvalue_reference<E>::value, "Result<T,E> can't hold rvalue references.");

private:
  using Base = details::BaseResult<T, E>;
  using ValidityState = details::ValidityState;

public:
  // TODO: DRY
  Result& operator=(const Result& other) {
    this->destruct();
    this->validityState_ = other.validityState_;
    switch (this->validityState_) {
      case ValidityState::ok:
        ::new (&(this->contents.val)) decltype(this->contents.val)(other.contents.val);
        break;
      case ValidityState::err:
        ::new (&(this->contents.err)) decltype(this->contents.err)(other.contents.err);
        break;
      case ValidityState::invalid:
        // TODO: this is almost always a bug, right?
        break;
    }
  }

  Result& operator=(Result&& other) {
    this->destruct();
    this->validityState_ = other.validityState_;
    switch (this->validityState_) {
      case ValidityState::ok:
        ::new (&(this->contents.val)) decltype(this->contents.val)(std::move(other.contents.val));
        break;
      case ValidityState::err:
        ::new (&(this->contents.err)) decltype(this->contents.err)(std::move(other.contents.err));
        break;
      case ValidityState::invalid:
        // TODO: this is almost always a bug, right?
        break;
    }
  }

  Result(const Result& other)
    : Base(details::dummy_t{}) {
    this->validityState_ = other.validityState_;
    switch (this->validityState_) {
      case ValidityState::ok:
        ::new (&(this->contents.val)) decltype(this->contents.val)(other.contents.val);
        break;
      case ValidityState::err:
        ::new (&(this->contents.err)) decltype(this->contents.err)(other.contents.err);
        break;
      case ValidityState::invalid:
        break;
    }
  }

  Result(Result&& other)
    : Base(details::dummy_t{}) {
    this->validityState_ = other.validityState_;
    switch (this->validityState_) {
      case ValidityState::ok:
        ::new (&(this->contents.val)) decltype(this->contents.val)(std::move(other.contents.val));
        break;
      case ValidityState::err:
        ::new (&(this->contents.err)) decltype(this->contents.err)(std::move(other.contents.err));
        break;
      case ValidityState::invalid:
        break;
    }
  }

  ~Result() = default;

  template <typename U>
  constexpr Result(details::OkWrapper<U> val) noexcept(
    std::is_nothrow_constructible<Base, details::OkWrapper<U>>::value)
    : Base(val) {
    // Are we holding a reference T?
    // Yes? -> Doesn't matter.
    // No? -> Is the type inside the details::OkWrapper<U> copy constructible?
    //  Yes? -> Doesn't matter.
    //  No? -> Attempted to create a non-reference Result<T,E> by non-copyable reference.
    static_assert(
      (!std::is_lvalue_reference<T>::value and std::is_lvalue_reference<U>::value)
        ? std::is_copy_constructible<std::remove_reference_t<U>>::value
        : true,
      "Attempted to create a Result<T,E> with a move-only type by reference. Did you intend to std::move() it?");
  }

  template <typename U>
  constexpr Result(details::ErrWrapper<U> val) noexcept(
    std::is_nothrow_constructible<Base, details::ErrWrapper<U>>::value)
    : Base(val) {
    static_assert(
      (!std::is_lvalue_reference<E>::value and std::is_lvalue_reference<U>::value)
        ? std::is_copy_constructible<std::remove_reference_t<U>>::value
        : true,
      "Attempted to create a Result<T,E> with a move-only type by reference. Did you intend to std::move() it?");
  }

  template <typename U>
  Result& operator=(details::OkWrapper<U> val) {
    this->destruct();
    ::new (&(this->contents.val)) decltype(this->contents.val)(std::forward<U>(val.contents));
    this->validityState_ = ValidityState::ok;
    return *this;
  }

  template <typename U>
  Result& operator=(details::ErrWrapper<U> val) {
    this->destruct();
    ::new (&(this->contents.err)) decltype(this->contents.err)(std::forward<U>(val.contents));
    this->validityState_ = ValidityState::err;
    return *this;
  }

  constexpr bool isErr() const noexcept { return this->validityState_ == details::ValidityState::err; }
  constexpr bool isOk() const noexcept { return this->validityState_ == details::ValidityState::ok; }
  constexpr explicit operator bool() const noexcept { return isOk(); }

  constexpr const T& ok() const & { return get_(); }
  constexpr T& ok() & { return get_(); }
  constexpr const T&& ok() const && { return std::move(get_()); }
  constexpr T&& ok() && { return std::move(get_()); }

  constexpr const E& err() const & { return getErr_(); }
  constexpr E& err() & { return getErr_(); }
  constexpr const E&& err() const && { return std::move(getErr_()); }
  constexpr E&& err() && { return std::move(getErr_()); }

  // attempt to call it by move first if we're an rvalue ref, otherwise SFINAE back to by ref.
  template <typename F, typename std::enable_if_t<details::isCallable<F(T&&)>>* = nullptr>
  constexpr Result<std::result_of_t<F(T&&)>, E> apply(F&& fn) && {
    if (isOk()) {
      return {Ok(fn(std::move(this->contents.val)))};
    } else {
      return {Err(std::move(this->contents.err))};
    }
  }

  template <typename F, typename std::enable_if_t<!details::isCallable<F(T&&)>>* = nullptr>
  constexpr Result<std::result_of_t<F(T&)>, E> apply(F&& fn) && {
    if (isOk()) {
      return Ok(fn(this->contents.val));
    } else {
      return {Err(std::move(this->contents.err))};
    }
  }

  // TODO: should there be an overload for void-returning functions?
  // TODO: wrapper for apply that returns the same Result<T,E> which only conditionally moves if it is actually
  // assigned.
  template <typename F>
  constexpr Result<std::result_of_t<F(T&)>, E> apply(F&& fn) & {
    using res_t = std::result_of_t<F(T&)>;
    static_assert(!std::is_same<res_t, void>::value, "Cannot apply a function that returns void.");
    if (isOk()) {
      return {Ok(fn(this->contents.val))};
    } else {
      return {Err(this->contents.err)};
    }
  }

  template <typename T2, std::enable_if_t<!details::isCallable<T2()>>* = nullptr>
  constexpr T okOr(T2&& orVal) && {
    static_assert(std::is_move_constructible<T>::value, "T must be move constructible to use okOr() with rvalue.");
    static_assert(std::is_convertible<T2&&, T>::value, "Provided value cannot be converted to T.");
    if (isOk()) {
      return std::move(this->contents.val);
    } else {
      return {std::forward<T2>(orVal)};
    }
  }

  /**
   *  Overload for callable F allowing the alternative to be computed lazily.
   */
  template <typename F, std::enable_if_t<details::isCallable<F()>>* = nullptr>
  constexpr T okOr(F&& orFn) && {
    static_assert(std::is_convertible<std::result_of_t<F()>, T>::value,
                  "Alternative does not return a type convertible to T.");
    static_assert(std::is_move_constructible<T>::value, "T must be move constructible to use okOr() with rvalue.");
    if (isOk()) {
      return std::move(this->contents.val);
    } else {
      return static_cast<T>(orFn());
    }
  }

  template <typename T2, std::enable_if_t<!details::isCallable<T2()>>* = nullptr>
  constexpr T okOr(T2&& orVal) const & {
    static_assert(std::is_copy_constructible<T>::value, "lvalue okOr requires a copy constructible T.");
    static_assert(std::is_convertible<T2&&, T>::value, "Provided value cannot be converted to T.");
    if (isOk()) {
      return this->contents.val;
    } else {
      return static_cast<T>(std::forward<T2>(orVal));
    }
  }

  /**
   *  Overload for callable F allowing the alternative to be computed lazily.
   */
  template <typename F, std::enable_if_t<details::isCallable<F()>>* = nullptr>
  constexpr T okOr(F&& orFn) const & {
    static_assert(std::is_convertible<std::result_of_t<F()>, T>::value,
                  "Alternative does not return a type convertible to T.");
    static_assert(std::is_copy_constructible<T>::value, "lvalue okOr requires a copy constructible T.");
    if (isOk()) {
      return this->contents.val;
    } else {
      return static_cast<T>(orFn());
    }
  }

private:
  constexpr T& get_() noexcept { return const_cast<T&>(static_cast<const Result*>(this)->get_()); }
  constexpr const T& get_(const char* msg = nullptr) const noexcept {
    if (!(isOk())) {
      abort_(msg);
    }
    return this->contents.val;
  }

  constexpr E& getErr_() noexcept { return const_cast<E&>(static_cast<const Result*>(this)->getErr_()); }
  constexpr const E& getErr_(const char* msg = nullptr) const noexcept {
    if (!(isErr())) {
      abort_(msg);
    }
    return this->contents.err;
  }

  void abort_(const char* msg) const {
    if (msg) {
      std::fputs(msg, stderr);
    }
    std::abort();
    details::unreachable();
  }
};

} // namespace util
