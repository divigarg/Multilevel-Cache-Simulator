#pragma once

#include <replacement.h>
#include <unistd.h>

extern FILE *_debug;

void access_list::add_item(struct list_item* _item) {
    fprintf(_debug, "%s: Added to LRU, way: %d\n", __func__, _item->way);
    if (head == NULL) {
        head = _item;
        head->next = head;
        head->prev = head;
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
    fprintf(_debug,"%s: Top. looking for way %d\n", __func__, _way);

    struct list_item *tmp = head;
    if (tmp == NULL)
        return NULL;
    
    // fprintf(_debug,"%s: tmp not null, tmp way: %d\n", __func__, tmp->way);
    
    do {
        if (tmp->way == _way)
            return tmp;
        // fprintf(_debug, "%s: do_while, way: %d\n", tmp->way)
        // usleep(100000); // 100ms
        // fprintf(_debug,"%s: No match: tmp->way = %d, isNULL %d\n", __func__, tmp->way, tmp->next == NULL);
        tmp = tmp->next;
        
    } while (tmp != head);

    fprintf(_debug,"%s: Bottom\n", __func__);

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
