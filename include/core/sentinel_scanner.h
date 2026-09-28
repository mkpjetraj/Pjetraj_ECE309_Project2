#pragma once
#include <string>
#include <string_view>

class SentinelScanner {
public:
    explicit SentinelScanner(std::string sentinel);

    struct Out { std::string safe_text; bool sentinel_found; };

    // Feed the next chunk. Returns text guaranteed not to be part of
    // the sentinel and whether the sentinel has now been fully seen.
    Out feed(std::string_view chunk);

    // Call once, after the stream ends, to release any text still
    // being held back.
    Out flush();

private:
    std::string sentinel_;
    std::string pending_;   // holds back at most sentinel_.size() - 1 chars
};