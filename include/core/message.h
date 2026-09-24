#pragma once
#include <string>

enum class Role { System, User, Assistant };

class Message {
public:
    // Default-constructs an empty System message with empty content.
    // Needed so Conversation can allocate raw array slots before
    // append() fills them in.
    Message();

    Message(Role role, std::string content);

    Role               role()    const noexcept;  // Who sent this message.
    const std::string& content() const noexcept;  // The message text.

private:
    Role        role_;
    std::string content_;
};

//inline so I can define here without duplicatae issue - Im not making message.cpp 
// default constructor: role starts as System, content_ is already an empty string
inline Message::Message() : role_(Role::System) {}

// copy role in , move string in (pass by value?)
inline Message::Message(Role role, std::string content)
    : role_(role), content_(std::move(content)) {}

//
inline Role Message::role() const noexcept { return role_; }

// const ref, read only
inline const std::string& Message::content() const noexcept { return content_; }

//std::string should manage itts own memory so I dont have to make destructor right...
//Rule of 0?