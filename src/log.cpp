#include "../include/log.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <stdexcept>

namespace lr {
    namespace detail {
        
        namespace {
            static std::ofstream g_logFile;
            
            std::string currentTime() {
            using std::chrono::system_clock;
            auto now = system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()) % 1000;
            std::time_t t = system_clock::to_time_t(now);
    
            std::tm tmBuf;

            #ifdef _WIN32
                localtime_s(&tmBuf, &t);
            #else
                localtime_r(&t, &tmBuf);
            #endif

            std::ostringstream oss;
            oss << std::put_time(&tmBuf, "%T") << "." << std::setw(3)
                << std::setfill('0') << ms.count();
            return oss.str();
        }
    
        std::string currentDateTime() {
            std::time_t now = std::time(nullptr);
            std::tm tmBuf;

            #ifdef _WIN32
                localtime_s(&tmBuf, &now);
            #else
                localtime_r(&now, &tmBuf);
            #endif

            char buffer[64];
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tmBuf);
            return std::string(buffer);
        }
        
        void logMessage(const std::string &level,
            const std::string &colorCode,
            const std::string &msg)
        {
            std::string timeStr = currentTime();
            std::string line = "[" + timeStr + "] [" + level + "] " + msg;

            #if LOG_ENABLE_CONSOLE
            std::cout << colorCode << line << ANSI_COLOR_RESET << std::endl;
            #endif

            #if LOG_ENABLE_FILE
            if (g_logFile.is_open()) {
                // Strip color codes in the file
                g_logFile << line << std::endl;
                g_logFile.flush(); // Ensure immediate write
            }
            #endif
        }
        
        } // end anonymous namespace

        void logInit() {
        #if LOG_ENABLE_FILE
            // Only open a file if file logging is on
            g_logFile.open(LOG_FILE_PATH, std::ios::app);
            if (!g_logFile.is_open()) {
                std::cerr << "LOG_INIT() - Could not open log file: "
                        << LOG_FILE_PATH << std::endl;
            }
        #endif
            // Print a separator in both file and console so we can see new runs.
            std::string initMsg = "---------------------------------------- "
                                "GPU SDL Drawing Log started at: " + currentDateTime() + " " + "---------------------------------------- ";
            logMessage("INIT", ANSI_COLOR_GREEN, initMsg);
        }

        void Info(const std::string &msg) {
            logMessage("INFO", ANSI_COLOR_GREEN, msg);
        }

        void Debug(const std::string &msg) {
            logMessage("DEBUG", ANSI_COLOR_YELLOW, msg);
        }

        void Error(const std::string &msg) {
            logMessage("ERROR", ANSI_COLOR_RED, msg);
        }

        void Fatal(const std::string &msg) {
            logMessage("FATAL", ANSI_COLOR_DARK_RED, msg);
            throw std::runtime_error("Fatal error: " + msg);
        }

        void Assert(bool cond, const std::string &msg){
            if(!cond){
                logMessage("ASSERT FAILED", ANSI_COLOR_DARK_RED, msg);
                assert(false && msg.c_str());
            }
        }

        void Success(const std::string &msg) {
            logMessage("SUCCESS", ANSI_COLOR_DARK_GREEN, msg);
        }

        void Assert_CL_OK(cl_int code, const std::string& situation) {
            if (code == CL_SUCCESS)
                return;  // Everything is OK, do nothing.

            std::string errStr = "Unknown OpenCL error";
            switch(code) {
                // run-time / JIT errors
                case CL_DEVICE_NOT_FOUND:                     errStr = "CL_DEVICE_NOT_FOUND"; break;
                case CL_DEVICE_NOT_AVAILABLE:                 errStr = "CL_DEVICE_NOT_AVAILABLE"; break;
                case CL_COMPILER_NOT_AVAILABLE:               errStr = "CL_COMPILER_NOT_AVAILABLE"; break;
                case CL_MEM_OBJECT_ALLOCATION_FAILURE:        errStr = "CL_MEM_OBJECT_ALLOCATION_FAILURE"; break;
                case CL_OUT_OF_RESOURCES:                     errStr = "CL_OUT_OF_RESOURCES"; break;
                case CL_OUT_OF_HOST_MEMORY:                   errStr = "CL_OUT_OF_HOST_MEMORY"; break;
                case CL_PROFILING_INFO_NOT_AVAILABLE:         errStr = "CL_PROFILING_INFO_NOT_AVAILABLE"; break;
                case CL_MEM_COPY_OVERLAP:                     errStr = "CL_MEM_COPY_OVERLAP"; break;
                case CL_IMAGE_FORMAT_MISMATCH:                errStr = "CL_IMAGE_FORMAT_MISMATCH"; break;
                case CL_IMAGE_FORMAT_NOT_SUPPORTED:           errStr = "CL_IMAGE_FORMAT_NOT_SUPPORTED"; break;
                case CL_BUILD_PROGRAM_FAILURE:                errStr = "CL_BUILD_PROGRAM_FAILURE"; break;
                case CL_MAP_FAILURE:                          errStr = "CL_MAP_FAILURE"; break;
                case CL_MISALIGNED_SUB_BUFFER_OFFSET:         errStr = "CL_MISALIGNED_SUB_BUFFER_OFFSET"; break;
                case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: errStr = "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST"; break;
                case CL_COMPILE_PROGRAM_FAILURE:              errStr = "CL_COMPILE_PROGRAM_FAILURE"; break;
                case CL_LINKER_NOT_AVAILABLE:                 errStr = "CL_LINKER_NOT_AVAILABLE"; break;
                case CL_LINK_PROGRAM_FAILURE:                 errStr = "CL_LINK_PROGRAM_FAILURE"; break;
                case CL_DEVICE_PARTITION_FAILED:              errStr = "CL_DEVICE_PARTITION_FAILED"; break;
                case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:        errStr = "CL_KERNEL_ARG_INFO_NOT_AVAILABLE"; break;

                // compile-time errors
                case CL_INVALID_VALUE:                        errStr = "CL_INVALID_VALUE"; break;
                case CL_INVALID_DEVICE_TYPE:                  errStr = "CL_INVALID_DEVICE_TYPE"; break;
                case CL_INVALID_PLATFORM:                     errStr = "CL_INVALID_PLATFORM"; break;
                case CL_INVALID_DEVICE:                       errStr = "CL_INVALID_DEVICE"; break;
                case CL_INVALID_CONTEXT:                      errStr = "CL_INVALID_CONTEXT"; break;
                case CL_INVALID_QUEUE_PROPERTIES:             errStr = "CL_INVALID_QUEUE_PROPERTIES"; break;
                case CL_INVALID_COMMAND_QUEUE:                errStr = "CL_INVALID_COMMAND_QUEUE"; break;
                case CL_INVALID_HOST_PTR:                     errStr = "CL_INVALID_HOST_PTR"; break;
                case CL_INVALID_MEM_OBJECT:                   errStr = "CL_INVALID_MEM_OBJECT"; break;
                case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:      errStr = "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR"; break;
                case CL_INVALID_IMAGE_SIZE:                   errStr = "CL_INVALID_IMAGE_SIZE"; break;
                case CL_INVALID_SAMPLER:                      errStr = "CL_INVALID_SAMPLER"; break;
                case CL_INVALID_BINARY:                       errStr = "CL_INVALID_BINARY"; break;
                case CL_INVALID_BUILD_OPTIONS:                errStr = "CL_INVALID_BUILD_OPTIONS"; break;
                case CL_INVALID_PROGRAM:                      errStr = "CL_INVALID_PROGRAM"; break;
                case CL_INVALID_PROGRAM_EXECUTABLE:           errStr = "CL_INVALID_PROGRAM_EXECUTABLE"; break;
                case CL_INVALID_KERNEL_NAME:                  errStr = "CL_INVALID_KERNEL_NAME"; break;
                case CL_INVALID_KERNEL_DEFINITION:            errStr = "CL_INVALID_KERNEL_DEFINITION"; break;
                case CL_INVALID_KERNEL:                       errStr = "CL_INVALID_KERNEL"; break;
                case CL_INVALID_ARG_INDEX:                    errStr = "CL_INVALID_ARG_INDEX"; break;
                case CL_INVALID_ARG_VALUE:                    errStr = "CL_INVALID_ARG_VALUE"; break;
                case CL_INVALID_ARG_SIZE:                     errStr = "CL_INVALID_ARG_SIZE"; break;
                case CL_INVALID_KERNEL_ARGS:                  errStr = "CL_INVALID_KERNEL_ARGS"; break;
                case CL_INVALID_WORK_DIMENSION:               errStr = "CL_INVALID_WORK_DIMENSION"; break;
                case CL_INVALID_WORK_GROUP_SIZE:              errStr = "CL_INVALID_WORK_GROUP_SIZE"; break;
                case CL_INVALID_WORK_ITEM_SIZE:               errStr = "CL_INVALID_WORK_ITEM_SIZE"; break;
                case CL_INVALID_GLOBAL_OFFSET:                errStr = "CL_INVALID_GLOBAL_OFFSET"; break;
                case CL_INVALID_EVENT_WAIT_LIST:              errStr = "CL_INVALID_EVENT_WAIT_LIST"; break;
                case CL_INVALID_EVENT:                        errStr = "CL_INVALID_EVENT"; break;
                case CL_INVALID_OPERATION:                    errStr = "CL_INVALID_OPERATION"; break;
                case CL_INVALID_GL_OBJECT:                    errStr = "CL_INVALID_GL_OBJECT"; break;
                default:                                      errStr = "Unknown OpenCL error"; break;
            }
            const std::string& fullMessage = errStr + " (" + std::to_string(code) + ")\n\t\tSituation: " + situation;
            logMessage("OpenCL Error", ANSI_COLOR_LIGHT_RED, fullMessage);
            assert(false && fullMessage.c_str());
        }

    } // namespace detail

} // namespace lr 