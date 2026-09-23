#include "core/sentinel_scanner.h"
#include <utility>

SentinelScanner::SentinelScanner(std::string sentinel)
    : sentinel_(std::move(sentinel)) {}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    // TODO
    (void)chunk;
    return {"", false};
}

SentinelScanner::Out SentinelScanner::flush() {
    // TODO
    return {"", false};
}
