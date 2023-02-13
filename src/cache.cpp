#pragma once

#include <cache.h>
#include <bits/stdc++.h>
#include <stdexcept>

extern int partNo;
extern char replAlgo;

extern FILE *_debug;

int get_sets(int w, unsigned long c, int b_size) {
    unsigned long set_s = (unsigned long)(w*b_size);
    return (int)(c/set_s);
}

int get_log_2(int num) {
    int _s = 0;

    while (num != 1) {
        num = num >> 1;
        // fprintf(_debug,"%s: num=%d\n", __func__, num);
        _s++;
    }
    
    return _s;
}

int get_tag_bits(int b_size, int i_size) {
    
    // for 64 bit address 
    return (64 - b_size - i_size);
}

unsigned long long Cache::get_addr(struct cache_block *_block) {
    unsigned long long _addr = 0;
    
    // fprintf(_debug, "%s: finding address of block->index = %d, block->tag = %lld\n",\
    //         __func__, _block->index, _block->tag);
    
    _addr = (_block->index) | (_block->tag << (index_bits));
    _addr = _addr << block_bits;
    fprintf(_debug,"%s: addr : %lld\n", __func__, _addr);
    return _addr;
}


struct cache_block* Cache::get_block(unsigned long long addr) {

    struct cache_block *_block = new struct cache_block;

    addr = addr >> block_bits;
    _block->index = addr & mask;
    _block->tag = addr >> index_bits;
    return _block;

}

void Cache::lookup(struct cache_block *_block) {
    
    int i;

    for (i = 0; i < ways; i++) {
        if (blocks[_block->index][i].valid && blocks[_block->index][i].tag == _block->tag) {
            _block->way = i;
            _block->valid = true;
            return;
        }
    }
    
}

void Cache::invalidate(struct cache_block *_block) {

    blocks[_block->index][_block->way].valid = false;
    blocks[_block->index][_block->way].tag = 0;
}


/*
    Finds the best candidate for eviction and invalidates it
    Returns: way no of block which has been freed
*/
int Cache::invoke_repl_policy(int index) {
    // it should set the victim cache block
    // Victim will be the tail of list
    if(partNo == 2 && replAlgo == 'b' && this->level == 3){
        //for belady replacement
        
    }
    else{
        struct cache_block _tmp;
        fprintf(_debug, "%s: before _victim index = %d, head = %p\n", __func__, index, lists[index].head);

        struct list_item *_victim = lists[index].head->prev;

        if (is_null(_victim)) {
            throw_error("error in replacment list of index %d\n", index);
        }
        fprintf(_debug, "%s: before _tmp\n", __func__);
        _tmp = blocks[index][_victim->way];
        // invalidate(&blocks[index][_victim->way]);
        victim = new struct cache_block(_tmp.index, _tmp.way, _tmp.tag);
        
        return _victim->way;
    }
}

//will not be used in belady;
void Cache::update_repl_params(int index, int way) {
    if(partNo == 2 && replAlgo == 'b' && this->level == 3) return;


    fprintf(_debug,"%s: top index : %d, way : %d\n", __func__, index, way);

    struct list_item *_item = lists[index].find_item(way);
    fprintf(_debug,"%s: after find_item func\n", __func__);
    if (is_null(_item)) {
        fprintf(_debug,"%s: item not found\n", __func__);
        _item = new struct list_item(way);
        lists[index].add_item(_item);
        fprintf(_debug,"%s: item added to list\n", __func__);
        return;
    }
    
    // If way is previously accessed, move it to the head of list
    lists[index].move_to_front(_item);
    // fprintf(_debug,"%s: item found and swapped with head\n", __func__);

}

void Cache::copy(struct cache_block *_block) {

    _block->way = get_target_way(_block->index);
    fprintf(_debug, "%s: got target way %d\n", __func__, _block->way);

    if (_block->way < 0) {
        _block->way = invoke_repl_policy(_block->index);
        fprintf(_debug, "%s: got target replaced way %d\n", __func__, _block->way);

    }

    _block->valid = 1; // to be sure
    blocks[_block->index][_block->way] = *_block;
}

int Cache::get_target_way(int index) {

    int i = 0;
    for (i; i < ways; i++) {
        if (!blocks[index][i].valid)
            return i;
    }
    
    return -1;

}
void Cache::preprocess_belady(struct entry *traceEntry, int index) {
    int address = traceEntry->addr;
    address = address >> block_bits;
    if (traceEntry->type) min_set[address].emplace_back(index);
}

bool lookup_l1(struct entry *_entry) {
    
    return (_entry->type == 0);
}
