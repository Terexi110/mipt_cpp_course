#include <event_list.h>

namespace nano_edr {

EventList::~EventList() {
    ListClear(this);
}

void ListPushBack(EventList* list, const Event* event){
    if (list->capacity > 0){
        if (list->size == list->capacity){
            ListPopFront(list);
        }
        list->size++;
        EventNode* new_node = new EventNode{*event, nullptr};
        if (list->head == nullptr){
            list->head = new_node;
            list->tail = new_node;
        } else {
            list->tail->next = new_node;
            list->tail = new_node;
        }
    } else if (list->capacity == 0){
        if (list->size == 0){
            list->head = new EventNode{*event, nullptr};
            list->tail = list->head;
        } else {
            list->tail->next = new EventNode{*event, nullptr};
            list->tail = list->tail->next;
        }
        list->size++;
    }
}

void ListPopFront(EventList* list){
    if (list->head != nullptr){
        EventNode* temp = list->head;
        list->head = list->head->next;
        delete temp;
        list->size--;
        if (list->head == nullptr){
            list->tail = nullptr;
        }
    }
}

void ListClear(EventList* list){
    while (list->head != nullptr){
        ListPopFront(list);
    }    
}

}