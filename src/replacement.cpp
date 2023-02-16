#pragma once

#include <replacement.h>
#include <unistd.h>
#include <run.h>

extern FILE *_debug;

void access_list::add_item(struct list_item* _item) {
    if (is_null(this->head)) {
        // fprintf(_debug, "%s head is null for the way %d\n", __func__, _item->way);
        this->head = _item;
        this->head->next = this->head;
        this->head->prev = this->head;
        return;
    }

    // fprintf(_debug, "%s: head addr: %p\n", __func__, this->head);

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
    // fprintf(_debug,"%s: Top. looking for way %d\n", __func__, _way);

    struct list_item *tmp = head;
    // fprintf(_debug, "%s: addr head=%p,way=%d\n", __func__, head, _way);
    if (is_null(tmp))
        return NULL;
    
    // fprintf(_debug,"%s: tmp not null\n", __func__);
    
    do {
        if (tmp->way == _way)
            return tmp;
        
        // fprintf(_debug,"%s: No match: tmp->way = %d\n", __func__, tmp->way);
        // usleep(100000); // 100ms
        tmp = tmp->next;
        
    } while (tmp != head);

    // fprintf(_debug,"%s: Bottom\n", __func__);

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
