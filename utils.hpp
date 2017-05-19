/*
 * utils.hpp Copyright © 2017 rsw0x
 *
 * Distributed under terms of the MPLv2 license.
 */

#ifndef UTILS_HPP58921
#define UTILS_HPP58921

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "result.hpp"

namespace util {
  using fstream_ptr = std::unique_ptr<std::FILE, void(*)(std::FILE*)>;

  struct io_error {
    static io_error from_context(const char*);

    void context(const char*);

    friend const char* get_context(const io_error& ioe){
      return ioe.buf.c_str();
    }

    std::string buf;
  };

  template<typename T>
  using IOError = util::Result<T, io_error>;

  struct openmode {
    enum : uint32_t {
      app    = 1,
      binary = 2,
      in     = 4,
      out    = 8,
      trunc  = 16,
      ate    = 32,
    };
    uint32_t value;

    openmode(uint32_t v) : value(v){  }

    const char* to_modestring() const;
  };

  IOError<fstream_ptr> open(const std::string& path, openmode);
  IOError<fstream_ptr> open(const char* path, openmode);

  IOError<std::string> as_string(const fstream_ptr&);
  IOError<std::vector<unsigned char>> as_bytes(const fstream_ptr&);
}

#endif /* !UTILS_HPP58921 */
