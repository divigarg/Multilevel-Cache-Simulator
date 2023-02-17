#pragma once

#include <cache.h>
#include <bits/stdc++.h>
#include <stdexcept>



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
    // fprintf(_debug,"%s: addr : %lld\n", __func__, _addr);
    return _addr;
}


void Cache::get_block(unsigned long long addr,
            struct cache_block *dst) {

    addr = addr >> block_bits;
    dst->index = addr & mask;
    dst->tag = addr >> index_bits;

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

    invalid_ways[_block->index].insert(_block->way);
    blocks[_block->index][_block->way].valid = false;

    //For belady_sort 
    if(repl_policy == BELADY){
        auto _victim_tag = blocks[_block->index][_block->way].tag;
        unsigned long long entry_no;
        if(prebeladyData[_victim_tag].first == prebeladyData[_victim_tag].second.size())
            entry_no = INT_MAX;
        else entry_no = prebeladyData[_victim_tag].second[prebeladyData[_victim_tag].first];
        belady_sort.erase(make_pair(entry_no, _block->way));
    }

    blocks[_block->index][_block->way].tag = 0;
}


/*
    Finds the best candidate for eviction and invalidates it
    Returns: way no of block which has been freed
*/
int Cache::invoke_repl_policy(int index, int counter) {
    //pass in the _entry->counter
    if(repl_policy == BELADY){
        //implement belady replacement
        //this will be in cache L3 only
        int max_distance = -1;
        int _victim_way = -1;
        for(int curr_way = 0; curr_way < ways; curr_way++){
            auto &tmpDataPair = prebeladyData[blocks[index][curr_way].tag];
            int &curr_idx = tmpDataPair.first;
            auto &tmpData = tmpDataPair.second;

            if(curr_idx == tmpData.size()){
                _victim_way = curr_way;
                goto redirect_way;
            }

            if(max_distance < (tmpData)[curr_idx] - counter){
                max_distance = (tmpData)[curr_idx] - counter;
                _victim_way = curr_way;
            }
            
        }
        // int _victim_way = belady_sort.rbegin()->second;
    redirect_way:
        if(_victim_way == -1){
            throw_error("error in belady replacement %d\n", index);
        }
        struct cache_block _tmp = blocks[index][_victim_way];
        victim = new struct cache_block(_tmp.index, _tmp.way, _tmp.tag);

        //For belady_sort 
        auto _victim_tag = blocks[_tmp.index][_tmp.way].tag;
        unsigned long long entry_no;
        if(prebeladyData[_victim_tag].first == prebeladyData[_victim_tag].second.size())
            entry_no = LONG_MAX;
        else entry_no = prebeladyData[_victim_tag].second[prebeladyData[_victim_tag].first];
        belady_sort.erase(make_pair(entry_no, _tmp.way));

        printf("victim: %d\n", _victim_way);

        return _victim_way;
    }
    // it should set the victim cache block
    // Victim will be the tail of list
    struct cache_block _tmp;

    struct list_item *_victim = lists[index].head->prev;

    if (is_null(_victim)) {
        throw_error("error in replacment list of index %d\n", index);
    }

    _tmp = blocks[index][_victim->way];

    
    victim = new struct cache_block(_tmp.index, _tmp.way, _tmp.tag);
    
    return _victim->way;
}


void Cache::update_repl_params(int index, int way) {

    if (repl_policy == BELADY)
        return;
    
    struct list_item *_item = lists[index].find_item(way);
    if (is_null(_item)) {
        _item = (struct list_item*)malloc(sizeof(struct list_item));
        _item->way = way;
        lists[index].add_item(_item);
        return;
    }
    
    // If way is previously accessed, move it to the head of list
    lists[index].move_to_front(_item);

}

void Cache::copy(struct cache_block *_block, int counter) {

    _block->way = get_target_way(_block->index);


    if (_block->way < 0) {
        _block->way = invoke_repl_policy(_block->index, counter);

    }

    invalid_ways[_block->index].erase(_block->way);
    _block->valid = 1; // to be sure
    blocks[_block->index][_block->way] = *_block;
    
    //For belady_sort 
    if(repl_policy == BELADY){
        auto _victim_tag = _block->tag;
        unsigned long long entry_no;
        if(prebeladyData[_victim_tag].first == prebeladyData[_victim_tag].second.size())
            entry_no = LONG_MAX;
        else entry_no = prebeladyData[_victim_tag].second[prebeladyData[_victim_tag].first];
        // printf("block Way: %d \n", _block->way);
        belady_sort.insert(make_pair(entry_no, _block->way));
    }
}

int Cache::get_target_way(int index) {

    if (invalid_ways[index].empty())
        return -1;

    return *invalid_ways[index].begin();

}

bool lookup_l1(struct entry *_entry) {
    
    return (_entry->type == 0);
}
