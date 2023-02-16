#pragma once

#include <replacement.h>
#include <unistd.h>
#include <run.h>


void access_list::add_item(struct list_item* _item) {
    if (is_null(this->head)) {
        this->head = _item;
        this->head->next = this->head;
        this->head->prev = this->head;
        return;
    }


    _item->next = head;
    _item->prev = head->prev;
    head->prev->next = _item;
    head->prev = _item;
    head = _item;
}

void access_list::remove_item(struct list_item *_item) {
    _item->prev->next = _item->next;
    _item->next->prev = _item->prev;
    free(_item);
}

struct list_item* access_list::find_item(int _way) {

    struct list_item *tmp = head;
    if (is_null(tmp))
        return NULL;
    
    
    do {
        if (tmp->way == _way)
            return tmp;
        

        tmp = tmp->next;
        
    } while (tmp != head);


    return NULL;
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
