#include "event_list.h"

namespace nano_edr {

EventList::~EventList() {
    Clear();
}

void EventList::PushBack(const Event& event) {
    while (capacity_ != 0 && size_ >= capacity_) {
        PopFront();
    }

    EventNode* node = new EventNode(event);
    if (tail_ == nullptr) {
        head_ = node;
    } else {
        tail_->next = node;
    }
    tail_ = node;
    ++size_;
}

void EventList::PopFront() {
    if (head_ == nullptr) {
        return;
    }

    EventNode* old_head = head_;
    head_ = head_->next;
    delete old_head;
    --size_;

    if (head_ == nullptr) {
        tail_ = nullptr;
    }
}

void EventList::Clear() {
    while (!empty()) {
        PopFront();
    }
}

}  // namespace nano_edr
