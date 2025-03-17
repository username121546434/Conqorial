#ifndef LOGGING_H
#define LOGGING_H

#include <iostream>
#include <sstream>
#include <string>

// Define our various build modes
// NDEBUG - Standard debug flag (when not defined, we're in debug mode)
// DISTRIBUTION - New flag for distribution builds (when defined, we're in distribution mode)
// When neither is defined, we're in debug mode
// When only NDEBUG is defined, we're in release mode
// When both are defined, we're in distribution mode

// Helper function to extract the short path
// (ie: "c:/coding/conqorial/client/src/file.cpp" -> "client/src/file.cpp")
constexpr std::string_view get_short_path(const char* path) {
    const char* end = path + std::char_traits<char>::length(path);
    
    // Find the last slash
    const char* lastSlash = end;
    while (lastSlash > path && *(lastSlash-1) != '/' && *(lastSlash-1) != '\\') --lastSlash;
    if (lastSlash > path) --lastSlash;
    
    // Find the second-last slash
    const char* secondLastSlash = lastSlash;
    while (secondLastSlash > path && *(secondLastSlash-1) != '/' && *(secondLastSlash-1) != '\\') --secondLastSlash;
    if (secondLastSlash > path) --secondLastSlash;
    
    // Find the third-last slash (to get the start of the second-last directory)
    const char* thirdLastSlash = secondLastSlash;
    while (thirdLastSlash > path && *(thirdLastSlash-1) != '/' && *(thirdLastSlash-1) != '\\') --thirdLastSlash;
    
    // If we couldn't find enough slashes, just use whatever we found
    const char* startPoint = (thirdLastSlash > path) ? thirdLastSlash : path;
    
    return std::string_view(startPoint);
}

namespace log {

// Base logger class with configurable compile-time enablement
template<typename StreamType, bool EnableInRelease>
class logger_base {
protected:
    std::stringstream ss;
    std::string filename;
    int line_number;
    StreamType& output_stream;

public:
    logger_base(StreamType& stream) : line_number(0), output_stream(stream) {}
    
    void set_location(const std::string& file, int line) {
        filename = get_short_path(file.c_str());
        line_number = line;
    }
    
    template<typename T>
    inline logger_base<StreamType, EnableInRelease> &operator<<(const T &message) {
#ifdef DISTRIBUTION
        // In distribution mode, all logging is disabled
        return *this;
#else
#ifdef NDEBUG
        // In release mode, only enabled if EnableInRelease is true
        if constexpr (!EnableInRelease) {
            return *this;
        }
#endif
        // Debug mode or (release mode with EnableInRelease=true)
        ss << message;

        if (!ss.str().empty() && ss.str().back() == '\n') {
            output_stream << filename << ":" << line_number << "  \t- " << ss.str();
            ss.str("");
            // Reset location info
            filename.clear();
            line_number = 0;
        }
#endif
        return *this;
    }
};

// Debug-only loggers (disabled in release and distribution modes)
class debug : public logger_base<std::ostream, false> {
public:
    debug() : logger_base(std::cout) {}
};

class debug_error : public logger_base<std::ostream, false> {
public:
    debug_error() : logger_base(std::cerr) {}
};

// Release loggers (enabled in debug and release modes, disabled in distribution mode)
class release_log : public logger_base<std::ostream, true> {
public:
    release_log() : logger_base(std::cout) {}
};

class release_error : public logger_base<std::ostream, true> {
public:
    release_error() : logger_base(std::cerr) {}
};

// External instances
extern debug dout;
extern debug_error derr;
extern release_log rout;
extern release_error rerr;

} // namespace log

// Macros to capture file and line info
#define LOG_DEBUG log::dout.set_location(__FILE__, __LINE__); log::dout
#define LOG_ERROR log::derr.set_location(__FILE__, __LINE__); log::derr
#define LOG_RELEASE log::rout.set_location(__FILE__, __LINE__); log::rout
#define LOG_RELEASE_ERROR log::rerr.set_location(__FILE__, __LINE__); log::rerr

// Base assert implementation with optional code to execute on failure
#define CONQORIAL_ASSERT_ALL(x, msg, ...) { \
    if (!(x)) { \
        std::cerr << "Assertion on " << get_short_path(__FILE__) << ':' << __LINE__ << " failed: " << #x << ' ' << (msg) << '\n'; \
        __VA_ARGS__ \
    } \
}

// Helper macro to handle the case of no extra code
#define CONQORIAL_ASSERT_ALL_CHOOSER(_1, _2, _3, CHOSEN, ...) CHOSEN

// Debug assert macros with optional code execution
#ifdef NDEBUG
#define CONQORIAL_DEBUG_ASSERT_2(x, msg)
#define CONQORIAL_DEBUG_ASSERT_3(x, msg, code)
#define CONQORIAL_DEBUG_ASSERT(...) CONQORIAL_ASSERT_ALL_CHOOSER(__VA_ARGS__, CONQORIAL_DEBUG_ASSERT_3, CONQORIAL_DEBUG_ASSERT_2)(__VA_ARGS__)
#else
#define CONQORIAL_DEBUG_ASSERT_2(x, msg) CONQORIAL_ASSERT_ALL(x, ("(debug-only assert)\n" + (msg)), )
#define CONQORIAL_DEBUG_ASSERT_3(x, msg, code) CONQORIAL_ASSERT_ALL(x, ("(debug-only assert)\n" + (msg)), code)
#define CONQORIAL_DEBUG_ASSERT(...) CONQORIAL_ASSERT_ALL_CHOOSER(__VA_ARGS__, CONQORIAL_DEBUG_ASSERT_3, CONQORIAL_DEBUG_ASSERT_2)(__VA_ARGS__)
#endif

// Release assert macros with optional code execution
#ifdef DISTRIBUTION
#define CONQORIAL_RELEASE_ASSERT_2(x, msg)
#define CONQORIAL_RELEASE_ASSERT_3(x, msg, code)
#define CONQORIAL_RELEASE_ASSERT(...) CONQORIAL_ASSERT_ALL_CHOOSER(__VA_ARGS__, CONQORIAL_RELEASE_ASSERT_3, CONQORIAL_RELEASE_ASSERT_2)(__VA_ARGS__)
#else
#define CONQORIAL_RELEASE_ASSERT_2(x, msg) CONQORIAL_ASSERT_ALL(x, ("(release assert)\n" + (msg)), )
#define CONQORIAL_RELEASE_ASSERT_3(x, msg, code) CONQORIAL_ASSERT_ALL(x, ("(release assert)\n" + (msg)), code)
#define CONQORIAL_RELEASE_ASSERT(...) CONQORIAL_ASSERT_ALL_CHOOSER(__VA_ARGS__, CONQORIAL_RELEASE_ASSERT_3, CONQORIAL_RELEASE_ASSERT_2)(__VA_ARGS__)
#endif

#endif // LOGGING_H
