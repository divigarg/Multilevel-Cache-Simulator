#pragma once 

#include <bits/stdc++.h>
#include <cache.h>

#define convert_l2_to_l3(_l2b, _l3b) \
({                                    \
    addr = l2_cache->get_addr(_l2b); \
    l3_cache->get_block(addr, _l3b); \
})

#define convert_l3_to_l2(_l3b, _l2b) \
({                                    \
    addr = l3_cache->get_addr(_l3b); \
    l2_cache->get_block(addr, _l2b); \
})


class simulator {

    Cache                   *l2_cache;
    Cache                   *l3_cache;

    struct cache_block      *l2_block;
    struct cache_block      *l3_block;
    struct cache_block      *_victim;

    unsigned long l1_misses;
    unsigned long l2_misses;
    unsigned long l3_misses;

    unsigned long cold_misses;
    unsigned long cold_and_capacity_misses;

    
    policy      cache_policy;
    bool        fully_assoc;

    public:
    void start_simulator(const char *filename, int numtraces);
    void init_caches(bool);
    void process_entry(struct entry*);
    void print_stats();
    void clean_memory();
    void reintialize();
    simulator(policy _policy, bool assc) {

        this->cache_policy = _policy;
        this->fully_assoc = assc;
        l2_block = new struct cache_block();
        l3_block = new struct cache_block;
        _victim = new struct cache_block;

        // printf("addr of L2_block: %p, L3_block:%p, Victim: %p\n", l2_block, l3_block, _victim);
        this->l1_misses = 0;
        this->l2_misses = 0;
        this->l3_misses = 0;
    
    };
};