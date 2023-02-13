#pragma once

#include <cache.h>
#include <stdlib.h>
#include <iostream>
#include <bits/stdc++.h>



using namespace std;

extern int partNo;
extern char replAlgo;


struct env *global_env;

Cache *l2_cache;
Cache *l3_cache;

FILE *_debug;

map<policy, string> policyString = {{INCLUSIVE, "Inclusive"}, {EXCLUSIVE, "Exclusive"}, {NINE, "Nine"}};

void start_simulator(char* filename, int numtraces, policy c_policy) {
    char* tmpfilename = (char *) malloc(strlen(filename));
    strcpy(tmpfilename, filename);
    char *token = strtok(tmpfilename, "/");
    token = strtok(NULL, "/");
    token = strtok(NULL, "/");
    token = strtok(token, ".");
    
    printf("%s: %s Policy\n", token, policyString[c_policy].c_str());

    _debug = fopen("logs/debug.log", "w");

    fprintf(_debug,"top of %s\n", __func__);
    FILE *fp, *fpbelady;
    
    char input_name[256];

    struct entry *_entry = (struct entry*)malloc(sizeof(struct entry));
    global_env = new env(filename, c_policy);
    
    init_caches();

    // start reading trace file
    fprintf(_debug,"Init Cache successful\n");

    //preprocessing in belady
    if(partNo == 2 && replAlgo == 'b'){
        char input_name_b[256];
        for (int k=0; k<numtraces; k++) {
            sprintf(input_name_b, "%s_%d", filename, k);
            fprintf(_debug,"belady preprocess: inputname: %s\n", input_name);
            fpbelady = fopen(input_name_b, "rb");
            assert(fpbelady != NULL);

            int counter = 0;
            while (!feof(fpbelady)) {
                counter++;
                fread(&_entry->iord, sizeof(char), 1, fpbelady);
                fread(&_entry->type, sizeof(char), 1, fpbelady);
                fread(&_entry->addr, sizeof(unsigned long long), 1, fpbelady);
                fread(&_entry->pc, sizeof(unsigned), 1, fpbelady);

                l3_cache->preprocess_belady(_entry, counter-1); 
            }
            fclose(fpbelady);
            fprintf(_debug,"Done belady processing file %d!\n", k);
        }
    }

    for (int k=0; k<numtraces; k++) {
        sprintf(input_name, "%s_%d", filename, k);
        fprintf(_debug,"inputname: %s\n", input_name);
        fp = fopen(input_name, "rb");
        assert(fp != NULL);

        //
        int counter = 0;
        //
        while (!feof(fp)) {
            counter++;
            fread(&_entry->iord, sizeof(char), 1, fp);
            fread(&_entry->type, sizeof(char), 1, fp);
            fread(&_entry->addr, sizeof(unsigned long long), 1, fp);
            fread(&_entry->pc, sizeof(unsigned), 1, fp);

            fprintf(_debug,"%s: counter : %d, Processing addr: %p type: %c\n",__func__, counter, _entry->addr, _entry->type);
            // fflush(_debug);
            process_entry(_entry);
            // Process the entry
        }
        fclose(fp);
        fprintf(_debug,"Done reading file %d!\n", k);
    }

    print_stats();
    // exit memeory cleanup
    free(_entry);
    // do_cache_cleanup();

}

void init_caches() {
    l2_cache = new Cache (8, 64, 512 KB, L2);

    if(partNo == 2)
        l3_cache = new Cache (32768, 64, 2 MB, L3);
    else
        l3_cache = new Cache (16, 64, 2 MB, L3);
}


struct cache_block* convert_l2_to_l3(struct cache_block *_l2b) {

        unsigned long long _addr = l2_cache->get_addr(_l2b);
        return l3_cache->get_block(_addr);

}

struct cache_block* convert_l3_to_l2(struct cache_block *_l3b) {

        unsigned long long _addr = l3_cache->get_addr(_l3b);
        fprintf(_debug,"%s: after get_addr. _addr = %lld\n", __func__, _addr);
        return l2_cache->get_block(_addr);
        // fprintf(_debug,"%s: bottom > assigned block to l2\n", __func__);

}

void process_entry(struct entry *_entry) {
    
    /*
        Process the entry based on memory hierarchy defined
        L1 -> L2 -> L3 -> Main Memory
        
    */

    struct cache_block *l2_block = NULL;
    struct cache_block *l3_block = NULL;
    struct cache_block *_victim = NULL;

    // fprintf(_debug,"%s: starting lookup\n", __func__);
    if (lookup_l1(_entry))
        return;

    // fprintf(_debug,"%s: after lookup 1\n",__func__);

    global_env->l1_misses++;

    // get block for L2 lookup
    l2_block = l2_cache->get_block(_entry->addr);
    // fprintf(_debug,"%s: after l2_block fetch - index: %d\n", __func__, l2_block->index); // clear

    l2_cache->lookup(l2_block);
    fprintf(_debug,"%s: after lookup 2, valid: %d\n", __func__, l2_block->valid);
    if (l2_block->valid) {
        l2_cache->update_repl_params(l2_block->index, l2_block->way);
        // fprintf(_debug,"%s: L2 cache replacement list updated for index: %d\n",\
             __func__, l2_block->index);
            
        goto clean;
    }

    global_env->l2_misses++;

    l3_block = l3_cache->get_block(_entry->addr);
    // fprintf(_debug,"%s: after l3_block fetch - index: %d\n", __func__, l3_block->index); // clear
    l3_cache->lookup(l3_block);
    fprintf(_debug,"%s: after lookup 3\n", __func__);
    fprintf(_debug,"%s: L3 way: %d, L3 index: %d, L3 Tag: %lld, Valid: %d\n", \
            __func__, l3_block->way, l3_block->index, l3_block->tag, l3_block->valid);
    
    if (l3_block->valid) {
        
        /*
        EXCLUSIVE: L2-M-L3-H
        * Invalidate the target block in L3
        * Copy the target block back to L2
        * If a block gets evicted from L2, put it in L3
        * If a block gets evicted from L3, enough of this now :(
        */
        if (global_env->cache_policy == EXCLUSIVE) {
            l3_cache->invalidate(l3_block);
                
            l2_cache->copy(l2_block); // L2 block is not valid here -> Wont be a problem though
            l2_cache->update_repl_params(l2_block->index, l2_block->way);
            
            if (l2_cache->victim != NULL){
                _victim = convert_l2_to_l3(l2_cache->victim);
                // l3_block->valid = true;
                //change made from l3_block to _victim since the victim needs to be added to l3
                l3_cache->copy(_victim);
                l3_cache->update_repl_params(_victim->index, _victim->way);
            }

        }

        /*
        INCLUSIVE: L2-M-L3-H
        * Update the LRU replacement params for target index in L3
        * Copy the target block back to L2
        * If a block gets evicted from L2, let it suffer :)
        */
        else if (global_env->cache_policy == INCLUSIVE) {
            // fprintf(_debug,"%s: inside inclusive L3 hit\n", __func__);

            l3_cache->update_repl_params(l3_block->index, l3_block->way);
            // fprintf(_debug,"%s: after update repl_params L3 hit\n", __func__);
            l2_cache->copy(l2_block);
            l2_cache->update_repl_params(l2_block->index, l2_block->way);
            fprintf(_debug,"%s: afer copying block to L2 cache\n", __func__);
        }

        /*
        NINE: L2-M-L3-H
        * Update the LRU replacement params for target index in L3
        * Copy the target block back to L2
        * If a block gets evicted from L2, put it in L3
        * If a block gets evicted from L3, I dont care :)
        */
        else { // NINE Policy

            l3_cache->update_repl_params(l3_block->index, l3_block->way);
            l2_cache->copy(l2_block);
            l2_cache->update_repl_params(l2_block->index, l2_block->way);

        }

        goto clean;
    }

    global_env->l3_misses++;

    // Handle L3 miss
    // After fetching block from main memory

    /*
    EXCLUSIVE: L2-M-L3-M
    * Put the target block in L2
    * If a block gets evicted from L2, put it in L3
    * If a block gets evicted from L3, no problem !
    */
    if (global_env->cache_policy == EXCLUSIVE) {
        l2_cache->copy(l2_block);
        l2_cache->update_repl_params(l2_block->index, l2_block->way);

        /*
        Check if any block got replaced in L2 cache
        */
        if (l2_cache->victim != NULL) {
            _victim = convert_l2_to_l3(l2_cache->victim);
            l3_cache->copy(_victim);
            l3_cache->update_repl_params(_victim->index, _victim->way);
        }
    }

    /*
    INCLUSIVE: L2-M-L3-M
    * Put the target block in L3 first
    * If the block gets evicted from L3, invalidate it from L2 if present
    * Put the target block in L2 now
    * If a block get evicted from L2, relax!
    */
    else if (global_env->cache_policy == INCLUSIVE) {
        fprintf(_debug,"%s: inside inclusive L3 miss\n", __func__);
        if(partNo == 2)
            if(l3_cache->l3_unique_blocks.find(l3_block->tag) == l3_cache->l3_unique_blocks.end()){
                l3_cache->l3_unique_blocks.insert(l3_block->tag);
                global_env->l3_cold_misses++;
            }
        l3_cache->copy(l3_block);
        fprintf(_debug,"%s: after copying block with index = %d, tag = %lld and valid = %d to L3 cache\n",
                __func__, l3_block->index, l3_block->tag, l3_block->valid);
        l3_cache->update_repl_params(l3_block->index, l3_block->way);
        /*
            Check if any block got evicted from L3, invalidate it from L2 also
            Don't act stupid, you are being watched!
        */
        if (l3_cache->victim != NULL) {
            fprintf(_debug, "%s: victim index : %d, way: %d, tag: %lld\n", __func__,\
                l3_cache->victim->index, l3_cache->victim->way, l3_cache->victim->tag);
            
            // fprintf(_debug,"%s: L3 victim not null\n", __func__);
            _victim = convert_l3_to_l2(l3_cache->victim);
            fprintf(_debug,"%s: conversion to L2 block done. L2 Index: %d, Tag: %lld\n",\
                     __func__, _victim->index, _victim->tag);
            
            l2_cache->lookup(_victim);
            // fprintf(_debug,"%s: Lookup of L2 cache done for victim block\n", __func__);
            
            if (_victim->valid) {
                //throw_error("inclusive property violated for %lx\n", _victim->tag);
                l2_cache->invalidate(_victim);
                fprintf(_debug,"%s: Invalidated L2 Index: %d, Tag: %lld,  Way: %d\n", \
                    __func__, _victim->index, _victim->tag, _victim->way);
            }
            
            
        }

        fprintf(_debug, "%s: copy block to L2 index: %d, tag: %lld\n", __func__, l2_block->index, l2_block->tag);
        l2_cache->copy(l2_block);
        l2_cache->update_repl_params(l2_block->index, l2_block->way);
        fprintf(_debug,"%s: after copying block with index = %d, tag = %lld and valid = %d to L2 cache\n",
                __func__, l2_block->index, l2_block->tag, l2_block->valid);
        
        fprintf(_debug,"%s: End of function\n", __func__);

    }

    /*
    NINE: L2-M-L3-M
    * Put the target block in L3 first
    * If a block gets evicted from L3, chill 
    * Put the target block in L2 now
    * If a block gets evicted from L2, put it in L3
    */
    else {
        l3_cache->copy(l3_block);
        l3_cache->update_repl_params(l3_block->index, l3_block->way);

        l2_cache->copy(l2_block);
        l2_cache->update_repl_params(l2_block->index, l2_block->way);

    }


clean:
    fprintf(_debug,"%s: cleaning up blocks\n", __func__);
    try {
        free(l2_block);
        if(!is_null(l3_block)) free(l3_block);
        if (!is_null(_victim)) free(_victim);
    }
    catch(std::exception ex) {
        fprintf(_debug,"exception caught in free\n");
    }
    if (!is_null(l2_cache->victim)) free(l2_cache->victim);
    if (!is_null(l3_cache->victim)) free(l3_cache->victim);
    l2_cache->victim = NULL;
    l3_cache->victim = NULL;
    fflush(_debug);
    return;
}

void print_stats() {
    cout << "L1 Misses " << global_env->l1_misses << endl;
    cout << "L2 Misses " << global_env->l2_misses << endl;
    cout << "L3 Misses " << global_env->l3_misses << endl;
    if(partNo == 2){
        cout << "L3 Cold Misses " << global_env->l3_cold_misses << endl;
        cout << "L3 Capacity Misses " << global_env->l3_misses - global_env->l3_cold_misses << endl;
    }
}