#include <event_list.h>

namespace nano_edr {

EventList::~EventList() {
    Clear();
}

void EventList::PushBack(const Event& event) {
    if (capacity_ != 0 && size_ == capacity_) {
        PopFront();
    }

    EventNode* new_node = new EventNode(event);
    if (head_ == nullptr) {
        head_ = new_node;
    } else {
        tail_->next = new_node;
    }
    tail_ = new_node;
    size_++;
}

void EventList::PopFront() {
    if (head_ != nullptr) {
        EventNode* temp = head_;
        head_ = head_->next;
        delete temp;
        size_--;
        if (head_ == nullptr) {
            tail_ = nullptr;
        }
    }
}

void EventList::Clear() {
    while (head_ != nullptr) {
        PopFront();
    }
}

}  // namespace nano_edr