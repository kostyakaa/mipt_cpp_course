#include "event_list.h"

void nano_edr::ListPopFront(EventList* list) {
    if (list->head == nullptr) {
        return;
    }

    EventNode* old_head = list->head;
    list->head = list->head->next;
    delete old_head;
    --list->size;

    if (list->head == nullptr) {
        list->tail = nullptr;
    }
}

void nano_edr::ListClear(EventList* list) {
    while (list->head != nullptr) {
        ListPopFront(list);
    }
}

void nano_edr::ListPushBack(EventList* list, const Event* event) {
    while (list->capacity != 0 && list->size >= list->capacity) {
        ListPopFront(list);
    }

    EventNode* node = new EventNode{*event};
    if (list->tail == nullptr) {
        list->head = node;
    } else {
        list->tail->next = node;
    }
    list->tail = node;
    ++list->size;
}

nano_edr::EventList::~EventList() {
    ListClear(this);
}
