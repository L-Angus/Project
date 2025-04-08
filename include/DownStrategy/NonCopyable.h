#ifndef __NONCOPYABLE_HPP__
#define __NONCOPYABLE_HPP__

class NonCopyable {
protected:
  NonCopyable() = default;
  NonCopyable(const NonCopyable &) = delete;
  NonCopyable &operator=(const NonCopyable &) = delete;
  NonCopyable(NonCopyable &&) = default;
  NonCopyable &operator=(NonCopyable &&) = default;
};

#endif