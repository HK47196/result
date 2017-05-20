# result


## TODO:

Documentation, tests, finish up static_assert contracts, finish up
implementation of the less important methods.

* Appears to be unnecessary copying in ok()/err() 

* option to throw an exception instead of abort on bad ok()/err()? easy
  propagation?


### usage:


Here's a real example taken from example2.cxx showing the power of `Result<T,E>`:

```cpp

static void two() {
  std::string contents = util::open("conf.ini", util::openmode::in)
    .context("Failed to load conf.ini")
    .apply(util::as_string)
    .ok("Failed to read file.");

  std::printf("File contents:\n%s", contents.c_str());
}

```

It makes use of the utils header which provides some adapters for the standard
library and common functions, which is a major WIP.

don't do this:

```cpp

auto&& r = functionThatReturnsResult().context("some context");

```

it will bind to a reference that is out of scope.

do this:

```cpp

auto r = functionThatReturnsResult().context("some context");

```

or this:

```cpp

auto&& r = functionThatReturnsResult();
r.context("some context");

```

The temporary that is propagated to context ceases to exist at the end of the
full expression containing the call([[class.temporary\]][u1]), and the lifetime
is not extended by an rvalue reference/const reference as you may unintuitively
believe.

I may change it in the future for rvalue member functions to instead return by
value which would make this safe. My only current ideas are providing a wrapper
proxy class to mimic the result which holds an rvalue reference to propagate the
lifetime of the result without requiring any copies.

### general notes:

Don't be afraid of using `ok()` when first writing your code. Unlike the harmful
practice of ignoring an error code, the program will crash as loudly as possible
if it contained an error. This allows for rapid prototyping while making bugs
  more apparent during testing.

`Result<T,E>` (in my opinion) is meant to be dealt with at the call site. It's not
a replacement for exceptions(don't hit me with your shoe please,) it's an
alternative. I came to this decision when studying Rust's error handling
capabilities and their need for crates like `error_chain` to emulate what
exceptions already do. `Result<T,E>` fills a similar role as when you'd return
an optional(`Optional<T>` is just `Result<T,void>`.) Maybe in the future I'll
examine something like `error_chain`'s functionality'.  

Thus, a `Result<T,E>` should be preferred for errors you can handle, and
exceptions for errors that you cannot and need to propagate up the call stack.


### implementation notes:

The `Try_` macro requires a `GCC` and `Clang` specific extension. I'll have to
think of a way to write it without statement expressions. This is one of the few
areas where statement expressions can't be replaced by a do-while(0) or lambda.
Maybe exceptions could be used here.  Not exactly a priority as I don't really
care about `MSVC` though.

[u1]: http://eel.is/c++draft/class.temporary#6
