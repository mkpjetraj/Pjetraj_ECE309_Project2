#include "core/conversation.h"
#include <stdexcept>
#include <utility>

Conversation::Conversation() {}

Conversation::~Conversation() {
    // TODO
}

Conversation::Conversation(const Conversation& other) {
    // TODO deep copy
    (void)other;
}

Conversation& Conversation::operator=(const Conversation& other) {
    // TODO
    (void)other;
    return *this;
}

Conversation::Conversation(Conversation&& other) noexcept {
    // TODO steal + zero other
    (void)other;
}

Conversation& Conversation::operator=(Conversation&& other) noexcept {
    // TODO
    (void)other;
    return *this;
}

void Conversation::append(Message m) {
    // TODO grow if full
    (void)m;
}

std::size_t Conversation::size() const noexcept {
    return size_;
}

const Message& Conversation::at(std::size_t i) const {
    // TODO bounds check
    return data_[i];
}

const Message* Conversation::begin() const noexcept {
    return data_;
}

const Message* Conversation::end() const noexcept {
    return data_ + size_;
}
