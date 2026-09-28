// tests/p2/test_p2.cpp
//
// test functions 1-12 defined below and called in main()
//
// my classes 
#include "core/conversation.h"
#include "core/message.h"
#include "core/sentinel_scanner.h"
// provided classes 
#include "harness/harness.h"          // Harness, HarnessConfig, InputSource, OutputSink
#include "model/replay_client.h"      // ReplayModelClient replays a saved transcript
#include "model/scripted_client.h"    // ScriptedModelClient reads a .script file

//library stuff for testing (assert is the big one)
#include <cassert>     // assert()
#include <cstdio>      // std::remove, test 12 for the transcript
#include <fstream>     // std::ofstream (writes a file), test 12 for the transcript
#include <iostream>    // std::cout (prints the "pass!" lines)
#include <memory>      // std::make_unique, used for tests 10-12
#include <stdexcept>   // std::out_of_range, for catch in test 1
#include <string>      
#include <utility>     // std::move (special member function)


//THESE HELPER FUNCTIONS USED IN TESTS 10-12
//Harness needs input and output abstracted away from just keyboard...
//cant really be user input because need to test determinitstically 



// input: says "bird" (lines) times, then acts like Ctrl-D was pressed
// I like birds
class FakeInput : public InputSource {       
public:
    // e.g. FakeInput in(10) -> 10 lines of "bird", then EOF
    explicit FakeInput(int lines) : left_(lines) {}

    // harness calls this every time it prints "you> "
    std::string read_line() override {
        if (left_ == 0) { eof_ = true; return ""; }   
        --left_;                                      
        return "bird";                                  // what the "user" typed
    }

    // harness checks right after read_line(); true -> stops with UserExit
    bool is_eof() const override { return eof_; }

private:
    int  left_;           // how many "bird"s are left
    bool eof_ = false;    // true once they run out
};



// output: saves everything printed into `text`
// (so a test can check what the user would have seen)
class FakeOutput : public OutputSink {
public:
    // harness calls this for everything it prints
    void write(std::string_view t) override { text += t; }
    std::string text;     // public so tests can read it afterwards
};



// ---------------------------------------------------------------------------
// CONVERSATION TESTS !!
// if anythign fails then itll crash after the last working test, 
// output messsage in main() below will indicate last working test
// ---------------------------------------------------------------------------

// 1. Empty Conversation Bounds: Handle empty conversations without out-of-bounds access.
void test_empty_conversation_bounds() {
    Conversation c;                       // brand new, nothing appended
    assert(c.size() == 0);                
    assert(c.begin() == c.end());        

    // if it DOESN'T throw, threw stays false and the assert fails.
    bool threw = false;
    try { c.at(0); } catch (const std::out_of_range&) { threw = true; }
    assert(threw);
}

// 2. System Message Ordering: Ensure system messages remain pinned at the front.
// system message stays at index 0 after appends
void test_system_message_ordering() {
    Conversation c;
    c.append(Message(Role::System, "Be concise."));        
    
    for (int i = 0; i < 50; ++i) c.append(Message(Role::User, "msg"));

    assert(c.size() == 51);                       // 1 system + 50 user
    assert(c.at(0).role() == Role::System);       // still at the front
    assert(c.at(0).content() == "Be concise.");  
}

// 3. Rule of Five (Copy): Assert copy constructors allocate entirely different pointer addresses.
void test_rule_of_five_copy() {
    Conversation a;
    a.append(Message(Role::User, "one"));
    a.append(Message(Role::User, "two"));

    Conversation b(a);                      // copy constructor
    assert(b.begin() != a.begin());         // different array in memory = deep copy
    assert(b.size() == 2 && b.at(1).content() == "two");   // same contents

    Conversation c;
    c.append(Message(Role::User, "old"));   // c already owns an array
    c = a;                                  // copy assignment: free old, copy a
    assert(c.begin() != a.begin());         
    assert(c.size() == 2 && c.at(0).content() == "one");   // "old" is gone
}

// 4. Rule of Five (Move): Assert move constructors steal the data pointer and zero the source.
void test_rule_of_five_move() {
    Conversation a;
    a.append(Message(Role::User, "one"));
    const Message* p = a.begin();           // remember where a's array is

    Conversation b(std::move(a));           // move constructor
    assert(b.begin() == p);                 // same array 
    assert(a.size() == 0 && a.begin() == a.end());   // a was zeroed

    Conversation c;
    c.append(Message(Role::User, "old"));   // c owns an array that must be freed
    c = std::move(b);                       // move assignment
    assert(c.begin() == p);                 // c  has the original array
    assert(b.size() == 0 && b.begin() == b.end());   // b was zeroed
}

// 5. Growth behavior: Assert capacity grows per your documented growth factor and
//    size()/at() stay correct across reallocation.
// growth: capacity 1, 2, 4, 8... (doubling).
void test_growth_behavior() {
    Conversation c;
    for (std::size_t i = 0; i < 100; ++i) {         // i = size BEFORE append
        const Message* before = c.begin();          // address BEFORE append
        c.append(Message(Role::User, std::to_string(i)));   
        bool grew = (c.begin() != before);          // did the array move? -- true when i is 0 or a power of 2
        bool power_of_two = (i & (i - 1)) == 0;     // true for 0, 1, 2, 4, 8... ^^
        assert(grew == power_of_two);             

        assert(c.size() == i + 1);                  // size counts up by one
        assert(c.at(i).content() == std::to_string(i));   // newest one is right
        assert(c.at(0).content() == "0");           // old ones survived 
    }
}

// ---------------------------------------------------------------------------
// SentinelScanner TESTS !!!
// if anythign fails then itll crash after the last working test, 
// output messsage in main() below will indicate last working test
// ---------------------------------------------------------------------------

// 6. Scanner (Clean Text): Verify the scanner processes strings with no sentinel correctly.
void test_scanner_clean_text() {
    SentinelScanner s("<|end_conversation|>");
    auto r1 = s.feed("Hello, ");                // chunk 1
    auto r2 = s.feed("no stop marker here.");   // chunk 2
    auto r3 = s.flush();                        // end of stream
    assert(!r1.sentinel_found && !r2.sentinel_found);   // never a false match
   
    assert(r1.safe_text + r2.safe_text + r3.safe_text == "Hello, no stop marker here."); //test that everything back
}

// 7. Scanner (Split Sentinel): Prove the scanner catches the sentinel split across every possible
//    boundary (loop over all split points programmatically).
void test_scanner_split_sentinel() {
    const std::string sentinel = "<|end_conversation|>";
    const std::string text = "Goodbye." + sentinel;

    for (std::size_t split = 0; split <= text.size(); ++split) {
        SentinelScanner scanner(sentinel);                  
        auto out1 = scanner.feed(text.substr(0, split));  //first bits
        auto out2 = scanner.feed(text.substr(split));       // the rest
      
        assert((out1.sentinel_found || out2.sentinel_found) &&
               "sentinel must be caught regardless of split point");
        assert(out1.safe_text + out2.safe_text == "Goodbye.");   // sentinel never printed
    }
}

// 8. Scanner (False Alarms): Ensure the scanner doesn't trigger on partial matches (e.g., <|end_world|>).
// near-misses like <|end_world|> never trigger it
void test_scanner_false_alarms() {
    SentinelScanner s("<|end_conversation|>");
    // two near-misses: wrong word, and the real one missing its final '>'
    auto r1 = s.feed("<|end_world|> and <|end_conversation");   // no final '>'
    auto r2 = s.flush();                                        
    assert(!r1.sentinel_found && !r2.sentinel_found);           // never triggered
    assert(r1.safe_text + r2.safe_text == "<|end_world|> and <|end_conversation");   // not lost
}

// 9. Scanner (Bounded Memory): Assert pending_ never exceeds sentinel.size() - 1 while feeding a
//    large adversarial stream.
//  this one runs slow bc it calls feed a million times lol
void test_scanner_bounded_memory() {
    SentinelScanner s("<|end_conversation|>");
    std::string piece = "<|end_";
    std::size_t fed = 0, given_back = 0;      // chars in, chars returned
    for (int i = 0; i < 1000000; ++i) { 
        //lol 1000000 seemed reasonable
        auto r = s.feed(std::string(1, piece[i % piece.size()]));
        assert(!r.sentinel_found);
        fed += 1;
        given_back += r.safe_text.size();
        assert(fed - given_back <= 19);      // pending_.size() <= 19
    }
}

// ---------------------------------------------------------------------------
// HARNESS TESTS(provided code): running with my classes inside.
// These use scripts/greeting.script: a system message and 3 replies,
// the 3rd one ending in the sentinel.
// ---------------------------------------------------------------------------

// 10. Harness (Turn Limit): Confirm the provided loop stops with TurnLimit when your
//    Conversation is used underneath it.
void test_harness_turn_limit() {
    HarnessConfig cfg;
    cfg.max_turns = 2;                        // only allow 2 turns
    Harness h(std::make_unique<ScriptedModelClient>("scripts/greeting.script"), cfg);
    FakeInput in(10);                         // plenty of input
    FakeOutput out;

    assert(h.run(in, out).kind == StopReason::Kind::TurnLimit);
    assert(h.conversation().size() == 4);     
}

// 11. Harness (Sentinel Halt): Confirm the provided loop halts exactly when your SentinelScanner
//    reports the sentinel found.
// stops on the sentinel (reply 3, split into chunks of 6) and never prints the sentinel
void test_harness_sentinel_halt() {
    Harness h(std::make_unique<ScriptedModelClient>("scripts/greeting.script"),
              HarnessConfig{});
    FakeInput in(10);
    FakeOutput out;

    assert(h.run(in, out).kind == StopReason::Kind::Sentinel);
    assert(h.conversation().size() == 6);     
    // find() returns npos ("not found") when the text isn't there
    assert(out.text.find("Goodbye!") != std::string::npos);   //  real text printed
    assert(out.text.find("<|end") == std::string::npos);     
}

// 12. Transcript Round-Trip: Save fake conversation, load it via the provided ReplayModelClient,
//    assert identical playback - save conversation, replay it with ReplayModelClient
// tricky, have to make conversation, take output and then re-feed it
void test_transcript_round_trip() {
    // run 1: normal session from the script
    Harness h1(std::make_unique<ScriptedModelClient>("scripts/greeting.script"),
               HarnessConfig{});
    FakeInput in1(10);
    FakeOutput out1;
    h1.run(in1, out1);

    // write it in transcript format from spec:
    std::ofstream f("test_transcript.txt");
    for (const Message& m : h1.conversation()) {                 
        if (&m != h1.conversation().begin()) f << "---\n";      // separator
        if (m.role() == Role::User) f << "role: user\n";
        else                        f << "role: assistant\n";
        f << m.content() << "\n";
    }
    f.close();                                // finish writing before reading it back

    // run 2: replay the saved transcript instead of the script
    // (the last saved reply still ends in the sentinel,  it stops the same way)
    Harness h2(std::make_unique<ReplayModelClient>("test_transcript.txt"),
               HarnessConfig{});
    FakeInput in2(10);                        // same fake input as run 1
    FakeOutput out2;
    assert(h2.run(in2, out2).kind == StopReason::Kind::Sentinel);

    //make sure the outputs are the same
    assert(out1.text == out2.text);           // printed exactly the same
    assert(h1.conversation().size() == h2.conversation().size());   
    for (std::size_t i = 0; i < h1.conversation().size(); ++i) {    // and each one matches
        assert(h1.conversation().at(i).content() == h2.conversation().at(i).content());
    }

    std::remove("test_transcript.txt");       // clean up the temp file bc we dont need it anymore
}

// ---------------------------------------------------------------------------

// calls each test in order. 
int main() {
    std::cout << "Tests: " << std::endl;
    test_empty_conversation_bounds(); std::cout << "1. Empty Conversation Bounds: pass!\n";
    test_system_message_ordering();   std::cout << "2. System Message Ordering: pass!\n";
    test_rule_of_five_copy();         std::cout << "3. Rule of Five (Copy): pass!\n";
    test_rule_of_five_move();         std::cout << "4. Rule of Five (Move): pass!\n";
    test_growth_behavior();           std::cout << "5. Growth behavior: pass!\n";
    test_scanner_clean_text();        std::cout << "6. Scanner (Clean Text): pass!\n";
    test_scanner_split_sentinel();    std::cout << "7. Scanner (Split Sentinel): pass!\n";
    test_scanner_false_alarms();      std::cout << "8. Scanner (False Alarms): pass!\n";
    test_scanner_bounded_memory();    std::cout << "9. Scanner (Bounded Memory): pass!\n";
    test_harness_turn_limit();        std::cout << "10. Harness (Turn Limit): pass!\n";
    test_harness_sentinel_halt();     std::cout << "11. Harness (Sentinel Halt): pass!\n";
    test_transcript_round_trip();     std::cout << "12. Transcript Round-Trip: pass!\n";

    std::cout << "All tests passed.\n";
    return 0;
}
//

