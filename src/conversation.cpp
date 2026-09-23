#include "core/conversation.h"
#include <stdexcept>
#include <utility>

//empty conversation owns no memory, data_ = nullptr
Conversation::Conversation() {}

//free entire array
Conversation::~Conversation() {
    delete[] data_;
}

//deep copy
Conversation::Conversation(const Conversation& other){
    if (other.size_ == 0) return;

    data_ = new Message[other.capacity_]; // new array, same capacity as other's 
    capacity_ = other.capacity_;

    for (std::size_t i = 0; i < other.size_; ++i) { // copy every message over one at a time (might not be most efficient)
        data_[i] = other.data_[i];
    }

    // only now do I actually hold other.size_ messages
    size_ = other.size_;
}

//copy + swap  - assignment
Conversation& Conversation::operator=(const Conversation& other) {
    Conversation tmp(other);

    std::swap(data_, tmp.data_);
    std::swap(size_, tmp.size_);
    std::swap(capacity_, tmp.capacity_);


    return *this;
}

//move - need noexcept
Conversation::Conversation(Conversation&& other) noexcept {
    // std::exchange(x, y) sets x = y and returns the old x
    data_  = std::exchange(other.data_, nullptr);
    size_   = std::exchange(other.size_, 0);
    capacity_ = std::exchange(other.capacity_, 0);
   
}

//move assignment
Conversation& Conversation::operator=(Conversation&& other) noexcept {
    
    if (this == &other) return *this;

    // free 
    delete[] data_;

    // move
    data_  = std::exchange(other.data_, nullptr);
    size_   = std::exchange(other.size_, 0);
    capacity_ = std::exchange(other.capacity_, 0);
    return *this;
}

//APPEND - this is where the money is
// check for fullness and grow if needed
//multilies by 2 each time 
void Conversation::append(Message m) {
   
    if (size_ == capacity_) { //check if full, double if so

        std::size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2; //if 0, start at 1, if not, double

        Message* new_data = new Message[new_capacity]; //new array

        // move the existing messages over one by one (there may be a better way...)
        for (std::size_t i = 0; i < size_; ++i) {
            new_data[i] = std::move(data_[i]); //MOVE, not COPY
        }

        // free
        delete[] data_; //new[] must be matched with delete[]

        // switch over
        data_ = new_data;
        capacity_ = new_capacity;
    }

    // now take new message and put it in
    data_[size_] = std::move(m);
    ++size_;
}

std::size_t Conversation::size() const noexcept {
    return size_;
}

//bound chekcinggg
const Message& Conversation::at(std::size_t i) const {
     if (i >= size_) {
        throw std::out_of_range("Conversation::at: index out of range");
    }
    return data_[i];
}

const Message* Conversation::begin() const noexcept {
    return data_;
}

const Message* Conversation::end() const noexcept {
    return data_ + size_;
}


//this is the best I can do right now - there might be a more effecitve way to implement
//some of this...