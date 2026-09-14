#include <event_list.h>

namespace nano_edr {

EventList::~EventList() {
    ListClear(this);
}

void ListPushBack(EventList* list, const Event* event){

}

void ListPopFront(EventList* list){

}

void ListClear(EventList* list){
    
}

}