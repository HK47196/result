/*
 * utils.cxx
 * Copyright© 2017 rsw0x
 *
 * Distributed under terms of the MPLv2 license.
 */

#include "utils.hpp"

#ifdef _WIN32
#error TODO
#else
#include <unistd.h>
#include <sys/stat.h>

namespace util {
  namespace {
    IOError<off_t> file_size(const fstream_ptr& fPtr) {
      const int fd = fileno(fPtr.get());
      if(fd == -1){
        return io_error{"Unable to convert the file pointer into a fd number."};
      }
      struct stat stbuf;
      // if((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))){
      if((fstat(fd, &stbuf) != 0)){
        return io_error{"Unable to fstat fd."};
      }


      static_assert(std::is_constructible<off_t, long>::value, "");
      return stbuf.st_size;
    }
  }
}
#endif

namespace{
  void fstream_ptr_dtor(std::FILE* fPtr){
    if(fPtr){
      std::fclose(fPtr);
    }
  }
}

namespace util {
  const char* openmode::to_modestring() const{
    switch(value){
      case in:
        return "r";
      case out:
      case out|trunc:
        return "w";
      case app:
      case out|app:
        return "a";
      case out|in:
        return "r+";
      case out|in|trunc:
        return "w+";
      case out|in|app:
      case in|app:
        return "a+";
      case binary|in:
        return "rb";
      case binary|out:
      case binary|out|trunc:
        return "wb";
      case binary|app:
      case binary|out|app:
        return "ab";
      case binary|out|in:
        return "r+b";
      case binary|out|in|trunc:
        return "w+b";
      case binary|out|in|app:
      case binary|in|app:
        return "a+b";
      default:
        return nullptr;
    }
  }

  void io_error::context(const char* msg) {
    if(!buf.empty()){
      buf += "\n\t";
    }
    buf += msg;
  }

  io_error io_error::from_context(const char* msg){
    return {};
  }

  IOError<fstream_ptr> open(const char* path, openmode openm){
    const char* mode = openm.to_modestring();
    if(mode == nullptr){
      return io_error::from_context("Invalid open mode.");
    }
    std::FILE* fPtr = std::fopen(path, mode);
    if(fPtr == nullptr){
      //TODO: errno string
      return io_error::from_context("Failed to open file.");
    }

    return fstream_ptr(fPtr, fstream_ptr_dtor);
  }

  IOError<fstream_ptr> open(const std::string& path, openmode openm){
    return open(path.c_str(), openm);
  }

  IOError<std::string> as_string(const fstream_ptr& fPtr){
    //TODO: platform specific way
    const off_t size = Try_(file_size(fPtr).context("Failed to get the size of the file."));
    
    std::string buf;
    buf.resize(size);
    const off_t ret = std::fread(&buf[0], 1, size, fPtr.get());
    if(ret != size){
      return io_error{"Failed to read entire file."};
    }
    return std::move(buf);
  }
}
