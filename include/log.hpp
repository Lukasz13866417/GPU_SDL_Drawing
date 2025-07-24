#ifndef LOG_HPP
#define LOG_HPP

#include "log_config.hpp"
#include <CL/opencl.hpp>
#include <string>
#include <stdexcept>

namespace lr {
    namespace detail {
        void logInit();
        void Debug(const std::string &msg);
        void Error(const std::string &msg);
        void Info(const std::string &msg);     
        void Fatal(const std::string &msg);
        void Assert(bool cond, const std::string &msg);
        void Assert_CL_OK(cl_int code, const std::string &msg);
        void Success(const std::string &msg);
    };
} // namespace lr


#if LOG_ENABLE
    #define LOG_INIT() lr::detail::logInit()
    #define LOG_INFO(msg)  lr::detail::Info(msg)
    #define LOG_DEBUG(msg) lr::detail::Debug(msg)
    #define LOG_ERR(msg)   lr::detail::Error(msg)
    #define LOG_FATAL(msg) lr::detail::Fatal(msg)
    #define LOG_SUCCESS(msg) lr::detail::Success(msg)
#else
    #define LOG_INIT()  do {} while(0)
    #define LOG_INFO(msg)   do {} while(0)
    #define LOG_DEBUG(msg)  do {} while(0)
    #define LOG_ERR(msg)    do {} while(0)
    #define LOG_FATAL(msg) throw std::runtime_error(msg)
    #define LOG_SUCCESS(msg)  do {} while(0)
#endif

#ifndef NDEBUG
    #define LOG_ASSERT(cond, msg) lr::detail::Assert(cond, msg)
    #define LOG_ASSERT_CL_OK(code, when) lr::detail::Assert_CL_OK(code, when)
#else
    #define LOG_ASSERT(cond, msg)    do {} while(0)
    #define LOG_ASSERT_CL_OK(code, msg)  do {} while(0)
#endif

#endif // LOG_HPP 