#pragma once

// "Smart Pimpl" implementation, courtesy of Andey Upadyshev
// Rewritten and adapted to fit this project's style and usage.
// https://github.com/oliora/samples/blob/master/spimpl.h
// http://oliora.github.io/2015/12/29/pimpl-and-rule-of-zero.html

#include <memory>
#include <type_traits>

namespace rvn {
namespace Details {
template <class T>
T* DefaultCopy(T* src) {
  static_assert(sizeof(T) > 0, "DefaultCopy cannot copy incomplete type");
  static_assert(!std::is_void<T>::value, "DefaultCopy cannot copy incomplete type");
  return new T(*src);
}

template <class T>
void DefaultDelete(T* p) noexcept {
  static_assert(sizeof(T) > 0, "DefaultDelete cannot delete incomplete type");
  static_assert(!std::is_void<T>::value, "DefaultDelete cannot delete incomplete type");
  delete p;
}

template <class T>
struct DefaultDeleter {
  using Type = void (*)(T*);
};

template <class T>
using DefaultDeleterT = typename DefaultDeleter<T>::Type;

template <class T>
struct DefaultCopier {
  using Type = T* (*)(T*);
};

template <class T>
using DefaultCopierT = typename DefaultCopier<T>::Type;

template <class T, class D, class C = DefaultCopierT<T>>
struct IsDefaultManageable
    : public std::integral_constant<bool,
                                    std::is_same<D, DefaultDeleterT<T>>::value &&
                                        std::is_same<C, DefaultCopierT<T>>::value> {};
}  // namespace Details

template <class T, class Deleter = Details::DefaultDeleterT<T>>
class UniqueImpl {
 protected:
  static_assert(!std::is_array<T>::value, "UniqueImpl cannot be used for arrays");
  // Struct is used to force compile-time errors if the enable_if constants do not return true.
  struct DummyT {
    int Dummy;
  };

 public:
  using Ptr = T*;
  using ConstPtr = typename std::add_const<T>::type*;
  using Ref = T&;
  using ConstRef = typename std::add_const<T>::type&;
  using DeleterT = typename std::decay<Deleter>::type;
  using UniquePtrT = std::unique_ptr<T, DeleterT>;
  using IsDefaultManageable = Details::IsDefaultManageable<T, DeleterT>;

  // Default Constructors
  constexpr UniqueImpl() noexcept : mPtr(nullptr, DeleterT{}) {}
  constexpr UniqueImpl(std::nullptr_t) noexcept : UniqueImpl() {}

  // Constructor with custom Deleter
  template <class D>
  UniqueImpl(Ptr p,
             D&& d,
             typename std::enable_if<std::is_convertible<D, DeleterT>::value, DummyT>::type =
                 DummyT()) noexcept :
      mPtr(std::move(p), std::forward<D>(d)) {}

  // Constructor with convertible pointer type
  template <class U>
  UniqueImpl(
      U* u,
      typename std::enable_if<std::is_convertible<U*, Ptr>::value && IsDefaultManageable::value,
                              DummyT>::type = DummyT()) noexcept :
      UniqueImpl(u, &Details::DefaultDelete<T>, &Details::DefaultCopy<T>) {}

  // Move constructor
  UniqueImpl(UniqueImpl&&) noexcept = default;

  // Move constructor with convertible pointer type
  template <class U>
  UniqueImpl(
      std::unique_ptr<U>&& u,
      typename std::enable_if<std::is_convertible<U*, Ptr>::value && IsDefaultManageable::value,
                              DummyT>::type = DummyT()) noexcept :
      mPtr(u.release(), &Details::DefaultDelete<T>) {}

  // Move constructor from unique_ptr with convertible pointer type and compatible deleter
  template <class U, class D>
  UniqueImpl(std::unique_ptr<U, D>&& u,
             typename std::enable_if<std::is_convertible<U*, Ptr>::value &&
                                         std::is_convertible<D, DeleterT>::value,
                                     DummyT>::type = DummyT()) noexcept :
      mPtr(std::move(u)) {}

  // Move constructor from UniqueImpl with convertible pointer type and compatible deleter
  template <class U, class D>
  UniqueImpl(UniqueImpl<U, D>&& u,
             typename std::enable_if<std::is_convertible<U*, Ptr>::value &&
                                         std::is_convertible<D, DeleterT>::value,
                                     DummyT>::type = DummyT()) noexcept :
      mPtr(std::move(u.mPtr)) {}

  // Deleted copy constructor
  UniqueImpl(const UniqueImpl<T, Deleter>&) = delete;

  // Equals move operator
  UniqueImpl& operator=(UniqueImpl&&) noexcept = default;

  // Equals move operator from unique_ptr
  template <class U>
  typename std::enable_if<std::is_convertible<U*, Ptr>::value && IsDefaultManageable::value,
                          UniqueImpl&>::type
  operator=(std::unique_ptr<U>&& u) noexcept {
    return operator=(UniqueImpl(std::move(u)));
  }

  // Deleted equals copy operator
  Ref operator=(const UniqueImpl<T, Deleter>&) = delete;

  // Dereference operators
  Ref operator*() { return *mPtr; }
  ConstRef operator*() const { return *mPtr; }
  Ptr operator->() noexcept { return Get(); }
  ConstPtr operator->() const noexcept { return Get(); }
  Ptr Get() noexcept { return mPtr.get(); }
  ConstPtr Get() const noexcept { return mPtr.get(); }

  // Swap
  void Swap(UniqueImpl& other) noexcept {
    using std::swap;
    mPtr.swap(other.mPtr);
  }

  // Release
  Ptr Release() noexcept { return mPtr.release(); }
  UniquePtrT ReleaseUnique() noexcept { return std::move(mPtr); }

  // Null-check
  explicit operator bool() const noexcept { return static_cast<bool>(mPtr); }

  // Retrieve deleter
  typename std::remove_reference<DeleterT>::type& GetDeleter() noexcept {
    return mPtr.get_deleter();
  }
  const typename std::remove_reference<DeleterT>::type& GetDeleter() const noexcept {
    return mPtr.get_deleter();
  }

 private:
  UniquePtrT mPtr;
};

template <class T, class... Args>
inline UniqueImpl<T> MakeUniqueImpl(Args&&... args) {
  return UniqueImpl<T>(new T(std::forward<Args>(args)...), &Details::DefaultDelete<T>);
}

template <class T,
          class Deleter = Details::DefaultDeleterT<T>,
          class Copier = Details::DefaultCopierT<T>>
class Impl : public UniqueImpl<T, Deleter> {
  using BaseType = UniqueImpl<T, Deleter>;
  using DummyT = typename BaseType::DummyT;

 public:
  using Ptr = typename BaseType::Ptr;
  using ConstPtr = typename BaseType::ConstPtr;
  using Ref = typename BaseType::Ref;
  using ConstRef = typename BaseType::ConstRef;
  using DeleterT = typename BaseType::DeleterT;
  using UniquePtrT = typename BaseType::UniquePtrT;
  using CopierT = typename std::decay<Copier>::type;
  using IsDefaultManageable = Details::IsDefaultManageable<T, DeleterT, CopierT>;

  // Default Constructors
  constexpr Impl() noexcept : BaseType(nullptr, DeleterT{}), mCopier(CopierT{}) {}
  constexpr Impl(std::nullptr_t) noexcept : Impl() {}

  // Constructor with custom deleter and copier
  template <class D, class C>
  Impl(Ptr p,
       D&& d,
       C&& c,
       typename std::enable_if<std::is_convertible<D, DeleterT>::value &&
                                   std::is_convertible<C, CopierT>::value,
                               DummyT>::type = DummyT()) noexcept :
      BaseType(std::move(p), std::forward<D>(d)), mCopier(std::forward<C>(c)) {}

  // Constructor with convertible pointer type
  template <class U>
  Impl(U* u,
       typename std::enable_if<std::is_convertible<U*, Ptr>::value && IsDefaultManageable::value,
                               DummyT>::type = DummyT()) noexcept :
      Impl(u, &Details::DefaultDelete<T>, &Details::DefaultCopy<T>) {}

  // Copy constructor
  Impl(const Impl& other) : Impl(other.Clone()) {}

  // Move constructor
  Impl(Impl&&) noexcept = default;

  // Move constructor with convertible pointer type
  template <class U>
  Impl(std::unique_ptr<U>&& u,
       typename std::enable_if<std::is_convertible<U*, Ptr>::value && IsDefaultManageable::value,
                               DummyT>::type = DummyT()) noexcept :
      BaseType(u.release(), &Details::DefaultDelete<T>) {}

  // Move constructor from unique_ptr with convertible pointer type and compatible deleter/copier
  template <class U, class D, class C>
  Impl(std::unique_ptr<U, D>&& u,
       typename std::enable_if<std::is_convertible<U*, Ptr>::value &&
                                   std::is_convertible<D, DeleterT>::value &&
                                   std::is_convertible<C, CopierT>::value,
                               DummyT>::type = DummyT()) noexcept :
      BaseType(std::move(u)), mCopier(std::forward<C>(c)) {}

  // Move constructor from Impl with convertible pointer type and compatible deleter
  template <class U, class D, class C>
  Impl(Impl<U, D>&& u,
       typename std::enable_if<std::is_convertible<U*, Ptr>::value &&
                                   std::is_convertible<D, DeleterT>::value &&
                                   std::is_convertible<C, CopierT>::value,
                               DummyT>::type = DummyT()) noexcept :
      BaseType(std::move(u.mPtr)), mCopier(std::move(u.mCopier)) {}

  // Equals copy operator
  Impl& operator=(const Impl& other) {
    if (this == &other) {
      return *this;
    }

    return operator=(other.Clone());
  }

  // Equals move operator
  Impl& operator=(Impl&&) noexcept = default;

  // Equals copy operator from compatible types
  template <class U, class D, class C>
  typename std::enable_if<std::is_convertible<U*, Ptr>::value &&
                              std::is_convertible<D, DeleterT>::value &&
                              std::is_convertible<C, CopierT>::value,
                          Impl&>::type
  operator=(const Impl<U, D, C>& other) {
    return operator=(other.Clone());
  }

  // Equals move operator from unique_ptr
  template <class U>
  typename std::enable_if<std::is_convertible<U*, Ptr>::value && IsDefaultManageable::value,
                          Impl&>::type
  operator=(std::unique_ptr<U>&& other) noexcept {
    return operator=(Impl(std::move(other)));
  }

  // Equals move operator from compatible types
  template <class U, class D, class C>
  typename std::enable_if<std::is_convertible<U*, Ptr>::value &&
                              std::is_convertible<D, DeleterT>::value &&
                              std::is_convertible<C, CopierT>::value,
                          Impl&>::type
  operator=(const Impl<U, D, C>&& other) {
    BaseType::mPtr = std::move(other.mPtr);
    mCopier = std::move(other.mCopier);
    return *this;
  }

  // Swap and clone
  void Swap(Impl& other) noexcept {
    using std::swap;
    BaseType::mPtr.swap(other.mPtr);
    swap(mCopier, other.mCopier);
  }
  Impl Clone() const {
    return Impl(BaseType::mPtr ? mCopier(BaseType::mPtr.get()) : nullptr,
                BaseType::mPtr.get_deleter(), mCopier);
  }

  // Retrieve copier
  typename std::remove_reference<CopierT>::type& GetCopier() noexcept { return mCopier; }
  const typename std::remove_reference<CopierT>::type& GetCopier() const noexcept {
    return mCopier;
  }

 private:
  CopierT mCopier;
};

template <class T, class... Args>
inline Impl<T> MakeImpl(Args&&... args) {
  return Impl<T>(new T(std::forward<Args>(args)...), &Details::DefaultDelete<T>,
                 &Details::DefaultCopy<T>);
}

template <class T, class D, class C>
inline void swap(Impl<T, D, C>& a, Impl<T, D, C>& b) noexcept {
  a.Swap(b);
}
}  // namespace rvn

namespace std {
template <class T, class D>
struct hash<rvn::UniqueImpl<T, D>> {
  using ArgType = rvn::UniqueImpl<T, D>;
  using ResultType = size_t;

  ResultType operator()(const ArgType& p) const noexcept {
    return hash<typename ArgType::Ptr>()(p.get());
  }
};

template <class T, class D, class C>
struct hash<rvn::Impl<T, D, C>> {
  using ArgType = rvn::Impl<T, D, C>;
  using ResultType = size_t;

  ResultType operator()(const ArgType& p) const noexcept {
    return hash<typename ArgType::Ptr>()(p.get());
  }
};
}  // namespace std