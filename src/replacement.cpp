#pragma once

#include <replacement.h>
#include <unistd.h>
#include <run.h>


void access_list::add_item(struct list_item* _item) {
    if (is_null(this->head)) {
        this->head = _item;
        this->head->next = this->head;
        this->head->prev = this->head;

        goto exit_add_item;
    }


    _item->next = head;
    _item->prev = head->prev;
    head->prev->next = _item;
    head->prev = _item;
    head = _item;

exit_add_item:
    
    item_index[_item->way] = _item;
    return;
}

void access_list::remove_item(struct list_item *_item) {
    _item->prev->next = _item->next;
    _item->next->prev = _item->prev;
    free(_item);
}

struct list_item* access_list::find_item(int _way) {

    return item_index[_way];
}

void access_list::move_to_front(struct list_item *_item) {

    if (_item == head) {
        return;
    }

    _item->prev->next = _item->next;
    _item->next->prev = _item->prev;
    _item->next = head;
    _item->prev = head->prev;
    head->prev->next = _item;
    head->prev = _item;
    head = _item;

}
