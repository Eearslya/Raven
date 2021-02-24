#pragma once

#include <Raven/Lib_Export.h>

#include <Raven/Log/Logger.hpp>

#define Log(category, level, msg, ...)                                                     \
  do {                                                                                     \
    if constexpr (static_cast<int>(Raven::Log::Level::level) <=                            \
                  static_cast<int>(Raven::Log::Categories::category::MaxCompileLevel())) { \
      Raven::Log::Logger::Instance().Log(Raven::Log::Categories::category::Instance(),     \
                                         Raven::Log::Level::level, msg, ##__VA_ARGS__);    \
    }                                                                                      \
  } while (0)

#define LogF(category, msg, ...) Log(category, Raven::Log::Level::Fatal, msg, __VA_ARGS__)
#define LogE(category, msg, ...) Log(category, Raven::Log::Level::Error, msg, __VA_ARGS__)
#define LogW(category, msg, ...) Log(category, Raven::Log::Level::Warning, msg, __VA_ARGS__)
#define LogI(category, msg, ...) Log(category, Raven::Log::Level::Info, msg, __VA_ARGS__)
#define LogD(category, msg, ...) Log(category, Raven::Log::Level::Debug, msg, __VA_ARGS__)
#define LogT(category, msg, ...) Log(category, Raven::Log::Level::Trace, msg, __VA_ARGS__)

#define DECLARE_LOG_CATEGORY(category, runtimeLevel, compileLevel)       \
  namespace Raven::Log::Categories {                                     \
  class category : public Raven::Log::Category {                         \
   public:                                                               \
    static Raven::Log::Category& Instance() noexcept;                    \
    constexpr static Raven::Log::Level MaxCompileLevel() noexcept {      \
      return Raven::Log::Level::compileLevel;                            \
    }                                                                    \
                                                                         \
   private:                                                              \
    category(std::string_view name = #category,                          \
             Raven::Log::Level level = Raven::Log::Level::runtimeLevel); \
  };                                                                     \
  }

#define DECLARE_LOG_CATEGORY_SHARED(export, category, runtimeLevel, compileLevel) \
  namespace Raven::Log::Categories {                                              \
  class category : public Raven::Log::Category {                                  \
   public:                                                                        \
    export static Raven::Log::Category& Instance() noexcept;                      \
    export constexpr static Raven::Log::Level MaxCompileLevel() noexcept {        \
      return Raven::Log::Level::compileLevel;                                     \
    }                                                                             \
                                                                                  \
   private:                                                                       \
    category(std::string_view name = #category,                                   \
             Raven::Log::Level level = Raven::Log::Level::runtimeLevel);          \
  };                                                                              \
  }

#define DEFINE_LOG_CATEGORY(category)                                  \
  namespace Raven::Log::Categories {                                   \
  Raven::Log::Category& category::Instance() noexcept {                \
    static category gCategory;                                         \
    return gCategory;                                                  \
  }                                                                    \
  category::category(std::string_view name, Raven::Log::Level level) : \
      Raven::Log::Category(name, level) {}                             \
  }

DECLARE_LOG_CATEGORY_SHARED(RAVEN_LIB_EXPORT, General, All, All);