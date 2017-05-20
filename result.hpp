/*
 * result.hpp
 * Copyright© 2017 rsw0x
 *
 * Distributed under terms of the MPLv2 license.
 */

#ifndef RESULT_HPP_7LRAEJZ5
#define RESULT_HPP_7LRAEJZ5

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <type_traits>
#include <utility>

namespace util {
  namespace details {
    template<class...>
    using void_t = void;

    template<typename Expr, typename Enabler = void>
    struct isCallableImpl : std::false_type {};

    template<typename F, typename... Args>
    struct isCallableImpl<F(Args...), void_t<std::result_of_t<F(Args...)>>>
      : std::true_type {};

    template<typename Expr>
    constexpr bool isCallable = isCallableImpl<Expr>::value;

    template<typename T>
    struct OkWrapper {
      T&& contents;
      OkWrapper(OkWrapper&&)      = delete;
      OkWrapper(const OkWrapper&) = delete;

      OkWrapper(T&& v)
        : contents(std::forward<T>(v)) {
      }
    };

    template<typename E>
    struct ErrWrapper {
      ErrWrapper(ErrWrapper&&)      = delete;
      ErrWrapper(const ErrWrapper&) = delete;
      E&& contents;

      ErrWrapper(E&& v)
        : contents(std::forward<E>(v)) {
      }
    };

    struct EmptyWrapper {};

#ifdef __GNUC__
    [[noreturn]] inline void unreachable() {
      __builtin_unreachable();
    }
#else
    inline void unreachable() {
    }
#endif

#ifdef __GNUC__
#define LIKELY_(x) __builtin_expect(static_cast<bool>(x), true)
#define UNLIKELY_(x) __builtin_expect(static_cast<bool>(x), false)
#else
#define LIKELY_(x) static_cast<bool>(x)
#define UNLIKELY_(x) static_cast<bool>(x)
#endif

    struct ok_tag {};
    struct err_tag {};

    // Used as the 'invalid' state
    struct dummy_t {};
    enum class ValidityState : char { invalid = 0, err = 1, ok = 2 };

    template<typename T>
    struct result_wrap_t {
    private:
      T contents;

    public:
      template<typename U>
      result_wrap_t(U&& rval)
        : contents(std::forward<U>(rval)) {
      }

      T& get() {
        return contents;
      }

      const T& get() const {
        return contents;
      }
    };

    // replace T& with std::reference_wrapper<T>
    template<typename T>
    struct result_wrap_t<T&> {
      std::reference_wrapper<T> contents;

      operator T&() {
        return contents;
      }

      operator const T&() const {
        return contents;
      }

      result_wrap_t(T& lval)
        : contents(lval) {
      }

      result_wrap_t(T&& rval)
        : contents(rval) {
      }

      T& get() {
        return contents.get();
      }

      const T& get() const {
        return contents.get();
      }
    };

    template<typename T>
    using fix_ref_t =
      std::conditional_t<std::is_lvalue_reference<T>::value,
                         std::reference_wrapper<std::remove_reference_t<T>>,
                         T>;

    // TODO: need to move a lot of logic from Result to a middle layer so
    // BaseResult can be overloaded for constexpr  cases.

    template<typename T, typename E>
    struct BaseResult {
      union contents_t {
        result_wrap_t<T> val;
        result_wrap_t<E> err;
        //`null` state
        dummy_t dummy_;

        explicit constexpr contents_t(dummy_t) noexcept
          : dummy_() {
        }

        explicit contents_t(T&& v, ok_tag)
          : val(std::forward<T>(v)) {
        }
        explicit contents_t(E&& e, err_tag)
          : err(std::forward<E>(e)) {
        }

        ~contents_t() {
        }
      } contents;
      ValidityState validityState_ = ValidityState::invalid;

      explicit BaseResult()
        : contents(dummy_t{})
        , validityState_(ValidityState::invalid) {
      }

      explicit constexpr BaseResult(dummy_t) noexcept
        : contents(dummy_t{})
        , validityState_(ValidityState::invalid) {
      }

      explicit constexpr BaseResult(details::EmptyWrapper) noexcept
        : contents(E{}, err_tag{})
        , validityState_(ValidityState::err) {
      }

      ~BaseResult() {
        destruct();
      }

      void destruct() {
        try {
          switch (validityState_) {
            case ValidityState::ok:
              // clang seems to not accept the decltype dtor syntax...
              using val_dtor_t = decltype(contents.val);
              contents.val.~val_dtor_t();
              break;
            case ValidityState::err:
              using err_dtor_t = decltype(contents.err);
              contents.err.~err_dtor_t();
              break;
            case ValidityState::invalid:
              break;
          }
          validityState_ = ValidityState::invalid;
        } catch (...) {
          validityState_ = ValidityState::invalid;
          throw;
        }
      }
    };
  } // namespace details

  // Lightweight wrapper just meant for return type deduction.
  template<typename T>
  details::OkWrapper<T> Ok(T&& val) {
    static_assert(sizeof(details::OkWrapper<T>) == sizeof(void*), "");
    return {std::forward<T>(val)};
  }

  template<typename T>
  details::ErrWrapper<T> Err(T&& val) {
    static_assert(sizeof(details::ErrWrapper<T>) == sizeof(void*), "");
    return {std::forward<T>(val)};
  }

  constexpr details::EmptyWrapper Err() {
    return {};
  }

  template<typename T, typename E>
  struct Result : private details::BaseResult<T, E> {
    static_assert(!std::is_rvalue_reference<T>::value,
                  "Result<T,E> can't hold rvalue references.");
    static_assert(!std::is_rvalue_reference<E>::value,
                  "Result<T,E> can't hold rvalue references.");

    static_assert(!std::is_convertible<T, E>::value,
                  "T and E cannot be convertible between each other.");
    static_assert(!std::is_convertible<E, T>::value,
                  "T and E cannot be convertible between each other.");

  private:
    using Base          = details::BaseResult<T, E>;
    using ValidityState = details::ValidityState;

    using Error_T = std::remove_reference_t<decltype(
      std::declval<typename Base::contents_t>().err.get())>;
    using Ok_T    = std::remove_reference_t<decltype(
      std::declval<typename Base::contents_t>().val.get())>;

    using Ok_Wrap_T    = decltype(Base::contents_t::val);
    using Error_Wrap_T = decltype(Base::contents_t::err);

    template<typename U>
    void reconstruct(U&& val, details::ok_tag) {
      this->validityState_ = ValidityState::ok;
      ::new (&(this->contents.val)) Ok_Wrap_T(std::forward<U>(val));
    }

    template<typename U>
    void reconstruct(U&& val, details::err_tag) {
      this->validityState_ = ValidityState::err;
      ::new (&(this->contents.err)) Error_Wrap_T(std::forward<U>(val));
    }

    void copy_assign(const Result& other) {
      switch (other.validityState_) {
        case ValidityState::ok:
          reconstruct(other.contents.ok(), details::ok_tag{});
          break;
        case ValidityState::err:
          reconstruct(other.contents.err(), details::err_tag{});
          break;
        case ValidityState::invalid:
          // TODO: this is always a bug, right?
          std::fprintf(stderr, "BUG\n");
          std::abort();
          break;
      }
    }

    void move_assign(Result&& other) {
      switch (other.validityState_) {
        case ValidityState::ok:
          reconstruct(std::move(other.ok()), details::ok_tag{});
          break;
        case ValidityState::err:
          reconstruct(std::move(other.err()), details::err_tag{});
          break;
        case ValidityState::invalid:
          // TODO: this is always a bug, right?
          std::fprintf(stderr, "BUG\n");
          std::abort();
          break;
      }
      // Moving a type leaves it in a valid state.
      //
      // other.validityState_ = ValidityState::invalid;
    }

  public:
    Result& operator=(const Result& other) {
      if (this == &other) {
        return *this;
      }
      this->destruct();
      copy_assign(other);
    }

    Result& operator=(Result&& other) {
      if (this == &other) {
        return *this;
      }
      this->destruct();
      move_assign(std::move(other));
    }

    template<typename U>
    Result& operator=(details::OkWrapper<U>&& val) {
      // TODO: static_assert constraints
      this->destruct();
      reconstruct(std::forward<U>(val.contents), details::ok_tag{});
      return *this;
    }

    template<typename U>
    Result& operator=(details::ErrWrapper<U>&& val) {
      // TODO: static_assert constraints
      this->destruct();
      reconstruct(std::forward<U>(val.contents), details::err_tag{});
      return *this;
    }

    Result(const Result& other) {
      copy_assign(other);
    }

    Result(Result&& other) {
      move_assign(std::move(other));
    }

    ~Result() = default;

    template<typename U>
    Result(details::OkWrapper<U>&& val) {
      reconstruct(std::forward<U>(val.contents), details::ok_tag{});
      // TODO: MOVE THE CONTRACTS.

      // Are we holding a reference T?
      // Yes? -> Doesn't matter.
      // No? -> Is the type inside the details::OkWrapper<U>&& copy
      // constructible?
      //  Yes? -> Doesn't matter.
      //  No? -> Attempted to create a non-reference Result<T,E> by non-copyable
      //  reference.
      static_assert(
        (!std::is_lvalue_reference<T>::value and
         std::is_lvalue_reference<U>::value)
          ? std::is_copy_constructible<std::decay_t<U>>::value
          : true,
        "Attempted to create a Result<T,E> with a move-only type by reference. "
        "Did you intend to std::move() it?");
    }

    template<typename U>
    Result(details::ErrWrapper<U>&& val) {
      reconstruct(std::forward<U>(val.contents), details::err_tag{});
      static_assert(
        (!std::is_lvalue_reference<E>::value and
         std::is_lvalue_reference<U>::value)
          ? std::is_copy_constructible<std::remove_reference_t<U>>::value
          : true,
        "Attempted to create a Result<T,E> with a move-only type by reference. "
        "Did you intend to std::move() it?");
    }

    constexpr Result(details::EmptyWrapper e)
      : Base(e) {
    }

    template<
      typename U,
      std::enable_if_t<std::is_constructible<Ok_T, U&&>::value>* = nullptr>
    Result(U&& val) {
      reconstruct(std::forward<U>(val), details::ok_tag{});
    }

    template<
      typename U,
      std::enable_if_t<std::is_constructible<Error_T, U&&>::value>* = nullptr>
    Result(U&& val) {
      reconstruct(std::forward<U>(val), details::err_tag{});
    }

    constexpr bool is_err() const noexcept {
      return this->validityState_ == details::ValidityState::err;
    }

    constexpr bool is_ok() const noexcept {
      return this->validityState_ == details::ValidityState::ok;
    }

    constexpr bool is_invalid() const noexcept {
      return this->validityState_ == details::ValidityState::invalid;
    }

    constexpr explicit operator bool() const noexcept {
      return is_ok();
    }

    // TODO
    const T& ok_unchecked(const char* msg = nullptr) const & {
      return get_(msg);
    }

    T& ok_unchecked(const char* msg = nullptr) & {
      return get_(msg);
    }

    T&& ok_unchecked(const char* msg = nullptr) && {
      return std::move(get_(msg));
    }

    const T& ok(const char* msg = nullptr) const & {
      return get_(msg);
    }

    T& ok(const char* msg = nullptr) & {
      return get_(msg);
    }

    T&& ok(const char* msg = nullptr) && {
      return std::move(get_(msg));
    }

    const E& err(const char* msg = nullptr) const & {
      return getErr_(msg);
    }

    E& err(const char* msg = nullptr) & {
      return getErr_(msg);
    }

    E&& err(const char* msg = nullptr) && {
      return std::move(getErr_(msg));
    }

    // TODO
    const E& err_unchecked(const char* msg = nullptr) const & {
      return getErr_(msg);
    }

    E& err_unchecked(const char* msg = nullptr) & {
      return getErr_(msg);
    }

    E&& err_unchecked(const char* msg = nullptr) && {
      return std::move(getErr_(msg));
    }

    // attempt to call it by move first if we're an rvalue ref, otherwise SFINAE
    // back to by ref.
    template<typename F,
             typename std::enable_if_t<details::isCallable<F(T&&)>>* = nullptr>
    Result<std::result_of_t<F(T&&)>, E> apply(F&& fn) && {
      if (is_ok()) {
        return {Ok(fn(std::move(ok())))};
      } else {
        return {Err(std::move(err()))};
      }
    }

    template<typename F,
             typename std::enable_if_t<!details::isCallable<F(T&&)>>* = nullptr>
    Result<std::result_of_t<F(T&)>, E> apply(F&& fn) && {
      if (is_ok()) {
        return Ok(fn(ok()));
      } else {
        return {Err(err())};
      }
    }

    // TODO: should there be an overload for void-returning functions?
    // TODO: wrapper for apply that returns the same Result<T,E> which only
    // conditionally moves if it is actually assigned.
    template<typename F>
    Result<std::result_of_t<F(T&)>, E> apply(F&& fn) & {
      using res_t = std::result_of_t<F(T&)>;
      static_assert(!std::is_same<res_t, void>::value,
                    "Cannot apply a function that returns void.");
      if (is_ok()) {
        return {Ok(fn(ok()))};
      } else {
        return {Err(err())};
      }
    }

    template<typename T2,
             std::enable_if_t<!details::isCallable<T2()>>* = nullptr>
    T ok_or(T2&& orVal) && {
      static_assert(std::is_move_constructible<T>::value,
                    "T must be move constructible to use ok_or() with rvalue.");
      static_assert(std::is_convertible<T2&&, T>::value,
                    "Provided value cannot be converted to T.");
      if (is_ok()) {
        return std::move(ok());
      } else {
        return {std::forward<T2>(orVal)};
      }
    }

    /**
     *  Overload for callable F allowing the alternative to be computed lazily.
     */
    template<typename F, std::enable_if_t<details::isCallable<F()>>* = nullptr>
    T ok_or(F&& orFn) && {
      static_assert(std::is_convertible<std::result_of_t<F()>, T>::value,
                    "Alternative does not return a type convertible to T.");
      static_assert(std::is_move_constructible<T>::value,
                    "T must be move constructible to use ok_or() with rvalue.");
      if (is_ok()) {
        return std::move(ok());
      } else {
        return static_cast<T>(orFn());
      }
    }

    template<typename T2,
             std::enable_if_t<!details::isCallable<T2()>>* = nullptr>
    T ok_or(T2&& orVal) const & {
      static_assert(std::is_copy_constructible<T>::value,
                    "lvalue okOr requires a copy constructible T.");
      static_assert(std::is_convertible<T2&&, T>::value,
                    "Provided value cannot be converted to T.");
      if (is_ok()) {
        return ok();
      } else {
        return static_cast<T>(std::forward<T2>(orVal));
      }
    }

    /**
     *  Overload for callable F allowing the alternative to be computed lazily.
     */
    template<typename F, std::enable_if_t<details::isCallable<F()>>* = nullptr>
    T ok_or(F&& orFn) const & {
      static_assert(std::is_convertible<std::result_of_t<F()>, T>::value,
                    "Alternative does not return a type convertible to T.");
      static_assert(std::is_copy_constructible<T>::value,
                    "lvalue okOr requires a copy constructible T.");
      if (is_ok()) {
        return ok();
      } else {
        return static_cast<T>(orFn());
      }
    }

    template<typename... Args>
    Result& context(Args&&... args) & {
      if (is_err()) {
        err().context(std::forward<Args...>(args...));
      }
      return {*this};
    }

    template<typename... Args>
    Result&& context(Args&&... args) && {
      if (is_err()) {
        err().context(std::forward<Args...>(args...));
      }
      return std::move(*this);
    }

    // T& operator->() {
    //   return ok();
    // }
    //
    // T& operator->() const {
    //   return ok();
    // }

  private:
    void err_if_(bool b, const char* msg) const {
      if (UNLIKELY_(b)) {
        abort_(msg);
      }
    }

    T& get_(const char* msg = nullptr) noexcept {
      err_if_(!is_ok(), msg);
      return this->contents.val.get();
    }

    const T& get_(const char* msg = nullptr) const noexcept {
      err_if_(!is_ok(), msg);
      return this->contents.val.get();
    }

    E& getErr_(const char* msg = nullptr) {
      err_if_(!is_err(), msg);
      return this->contents.err.get();
    }

    const E& getErr_(const char* msg = nullptr) const noexcept {
      err_if_(!is_err(), msg);
      return this->contents.err.get();
    }

    template<typename U>
    struct has_get_ctx {
      using Yes = char;
      using No  = Yes[2];

      template<typename C>
      static auto test(void*)
        -> decltype(get_context(std::declval<C>()), Yes{});

      template<typename>
      static No& test(...);

      static constexpr bool value = sizeof(test<E>(0)) == sizeof(Yes);
    };

    template<typename U = E>
    auto print_ctx() const -> std::enable_if_t<!has_get_ctx<U>::value, void> {
    }

    template<typename U = E>
    auto print_ctx() const -> std::enable_if_t<has_get_ctx<U>::value, void> {
      static_assert(
        std::is_same<decltype(get_context(err())), const char*>::value,
        "get_context must return a c string");
      std::fprintf(stderr, "Context: %s\n", get_context(err()));
    }

    void abort_(const char* msg) const {
      std::fprintf(stderr,
                   "Invalid Result access, message given: %s\n",
                   msg ? msg : "No message given.");
      if (is_err()) {
        print_ctx();
      }
      std::abort();
      details::unreachable();
    }
  };

// rvalue ref keeps a temporary alive the same as a const ref [dcl.init.ref]
//
// TODO: unlikely TODO: currently attempts to return a default error if it's
// invalid.
//
#define Try_(expr)                                                             \
  ({                                                                           \
    auto result_var_ = (expr);                                                 \
    if (result_var_.is_err()) {                                                \
      return util::Err(std::move(result_var_.err()));                          \
    } else if (result_var_.is_invalid()) {                                     \
      std::abort();                                                            \
      return util::Err();                                                      \
    }                                                                          \
    std::move(result_var_.ok());                                               \
  })
} // namespace util

#undef LIKELY_
#undef UNLIKELY_
#endif /* end of include guard: RESULT_HPP_7LRAEJZ5 */
