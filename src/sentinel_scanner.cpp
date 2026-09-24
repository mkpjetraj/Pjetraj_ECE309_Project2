#include "core/sentinel_scanner.h"
#include <utility>

//sentinel <|end_conversation|> : 20 characters (deifned in harness.cpp, line 10)(kSentinel)
// size to withhold: last 19 characters of string - allows to check for sentinel (pending_)
//append next chunk to this withheld section and then sort through that - if safe, put to console and keep checking
//if not safe, end converation and dont send to console

//Constructor
//sentinel passed by value, pending_ starts out as an empty string 
SentinelScanner::SentinelScanner(std::string sentinel)
    : sentinel_(std::move(sentinel)) {}

// Called once for every chunk of the model's reply.
SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    std::string text = pending_ + std::string(chunk); //last round reserved + new 
    
    //search through all that for sentinel  -returns npos if not found or position if found
    std::size_t pos = text.find(sentinel_); 


    if (pos != std::string::npos) { //found
        pending_.clear(); //clear pending
        //substr(0, pos) is the characters before the sentinel starts
        return {text.substr(0, pos), true}; //found it, sentinel and what folllows is not included in message
    }

    //not found but sentinel might have started 
    //must hold back characters in case the sentinel finishes in the next chunk 
    std::size_t hold = sentinel_.empty() ? 0 : sentinel_.size() - 1;

    //hold back UP TO 19 CHARS
    std::size_t keep = std::min(text.size(), hold);

    // everything before what is held is printed to console (safe)
    std::string safe = text.substr(0, text.size() - keep);

    // held back stuff is now pending_ , pending_.size() <= 19
    pending_ = text.substr(text.size() - keep);

    //false: keep streaming
    return {safe, false};
}
// Called once when the reply ends WITHOUT a sentinel 
SentinelScanner::Out SentinelScanner::flush() {
    std::string rest = pending_; 

    // clear pending
    pending_.clear();
    //give it back to be printed to console
    return {rest, false};
}
