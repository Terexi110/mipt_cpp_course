#include <event_list.h>

namespace nano_edr {

EventList::~EventList() {
    ListClear(this);
}

void ListPushBack(EventList* list, const Event* event) {
    if (list->capacity != 0 && list->size == list->capacity) {
        ListPopFront(list);
    }

    EventNode* new_node = new EventNode{.event = *event, .next = nullptr};
    if (list->head == nullptr) {
        list->head = new_node;
    } else {
        list->tail->next = new_node;
    }
    list->tail = new_node;
    list->size++;
}

void ListPopFront(EventList* list) {
    if (list->head != nullptr) {
        EventNode* temp = list->head;
        list->head = list->head->next;
        delete temp;
        list->size--;
        if (list->head == nullptr) {
            list->tail = nullptr;
        }
    }
}

void ListClear(EventList* list) {
    while (list->head != nullptr) {
        ListPopFront(list);
    }
}

}  // namespace nano_edr