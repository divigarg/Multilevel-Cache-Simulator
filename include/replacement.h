#include <bits/stdc++.h>

struct access_list {
    

    struct list_item *head;

    void add_item(struct list_item*);
    void remove_item(struct list_item*);
    struct list_item* find_item(int);
    void move_to_front(struct list_item*);

    access_list() {
        head = NULL;
    }

};

struct list_item {
    int way;
    struct list_item *next;
    struct list_item *prev;

    list_item(int w) {
        this->way = w;
        this->next = NULL;
        this->prev = NULL;
    }
    
};
