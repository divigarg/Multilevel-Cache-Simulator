#pragma once

#include <run.h>
#include <replacement.h>
/*
        Cache Hierarchy Reference
----------------------L2-------------------
Capacity: 512KB): undefined reference to `Cache::index_bits'
Ways: 8
Block Size: 64B
Sets: 1024
----------------------L2-------------------
----------------------L3-------------------
Capacity: 2MB
Ways: 16
Block Size: 64B
Sets: 2048
----------------------L3--------------------
*/


#define KB *1024
#define MB *1024*1024
#define L2 2
#define L3 3

extern FILE *_debug;

struct cache_block {
    int index;
    unsigned long long tag;
    int way;
    bool valid;
    cache_block() {
        index = -1;
        tag = 0;
        way = -1;
        valid = false;
    };

    cache_block (int i, int w, unsigned long long int t) {
        index = i;
        way =  w;
        tag = t;
    };

};


int get_sets(int w, unsigned long c, int b_size);
int get_log_2(int num);
int get_tag_bits(int b_size, int i_size);


class Cache {

    public:
    /*
        Only constant fields should be added here    
    */
    int level;
    int ways;
    unsigned long capacity;
    int block_size;
    int sets;
    int tag_bits;
    int index_bits;
    int block_bits;
    int mask;   // Mask to get Index from address
    policy repl_policy;
    
    /*
        constant fields section ends
    */

    struct cache_block **blocks;

    int         last_access_way;
    struct cache_block    *victim;

    struct access_list         *lists;

    void lookup(struct cache_block*);
    void copy(struct cache_block*);
    void invalidate(struct cache_block*);
    int  invoke_repl_policy(int index);
    void update_repl_params(int index, int way);
    struct cache_block* get_block(unsigned long long);
    int get_target_way(int index);
    unsigned long long get_addr(struct cache_block*);

    Cache (int w, int b_size, unsigned long c, int lvl) {

        fprintf(_debug, "%s: top \n", __func__);
        int i = 0;
        this->level = lvl;
        this->ways = w;
        this->capacity = c;
        this->block_size = b_size;

        this->sets =  get_sets(w, c, b_size);

        this->index_bits = get_log_2(this->sets);

        this->block_bits = get_log_2(this->block_size);
        this->tag_bits = get_tag_bits(this->block_bits, this->index_bits);
        this->mask = this->sets - 1; 
        printf("%s: level: %d, sets: %d, ways: %d, capacity: %ld, block_bits: %d,tag_bits: %d, index_bits: %d\n",\
                __func__, level, sets, ways, capacity, block_bits, tag_bits, index_bits);
        // initialize 2d array for tags
        this->blocks = (struct cache_block**)malloc(sizeof(struct cache_block*)*sets);
        fprintf(_debug, "%s: 2d array pointer allocated\n", __func__);

        for (i; i < sets; i++) 
            this->blocks[i] = (struct cache_block*)malloc(sizeof(struct cache_block)*ways);
        
        fprintf(_debug, "%s: sizeof blocks array: (%d, %d), ways: %d\n", __func__, sizeof(this->blocks)/sizeof(struct cache_block*), sizeof(this->blocks[0])/sizeof(struct cache_block), ways);

        this->lists = (struct access_list*)malloc(sizeof(struct access_list)*sets);
        
    };
      
};

bool lookup_l1(struct entry*);
