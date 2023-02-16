#pragma once

#include <cache.h>
#include <simulator.h>
#include <stdlib.h>
#include <iostream>
#include <bits/stdc++.h>
#include <chrono>

#define LOCK    pthread_mutex_lock(&_lock);
#define UNLOCK  pthread_mutex_unlock(&_lock);

using namespace std;

pthread_mutex_t _lock;

extern FILE *_debug;

map<policy, string> policyString = {{INCLUSIVE, "Inclusive"}, {EXCLUSIVE, "Exclusive"}, {NINE, "Nine"}};


void *run_thread(void* _args) {

    struct args *__args = (struct args*)_args;
    simulator *_simulator = new simulator(__args->_policy, __args->full_assoc);
    _simulator->init_caches(__args->full_assoc);
    chrono::time_point<chrono::system_clock> sTime, eTime;
        
    sTime = chrono::system_clock::now(); 
    _simulator->start_simulator(__args->filename, __args->num_traces);
    eTime = chrono::system_clock::now();

    _simulator->clean_memory();
    chrono::duration<double> timeTaken = eTime-sTime;

    pthread_mutex_lock(&_lock);
    printf("----------------STATS--------------------\n");
    _simulator->print_stats();
    printf("%c: Elapsed Time: %.3f secs\n", __args->_policy, timeTaken.count());
    printf("------------------------------------\n");

    pthread_mutex_unlock(&_lock); 

    return NULL;
}

void run(struct args *_args) {
    // create three threads
    pthread_t inclusive_t, exclusive_t, nine_t;

    if (pthread_mutex_init(&_lock, NULL)) {
        fprintf(_debug, "mutex init failed\n");
        exit (1);
    }

    struct args *_inc_args = new struct args();
    struct args *_exc_args = new struct args();
    struct args *_nine_args = new struct args();
    
    memcpy(_inc_args, _args, sizeof(struct args));
    memcpy(_exc_args, _args, sizeof(struct args));
    memcpy(_nine_args, _args, sizeof(struct args));

    printf("%s:creating threads\n", __func__);

    
    _inc_args->_policy = INCLUSIVE;
    
    pthread_create(&inclusive_t, NULL, run_thread, (void*)_inc_args);

    if (_args->full_assoc)
        goto exit_run;
    
    _exc_args->_policy = EXCLUSIVE;
    pthread_create(&exclusive_t, NULL, run_thread, (void*)_exc_args);
    _nine_args->_policy = NINE;
    pthread_create(&nine_t, NULL, run_thread, (void*)_nine_args);
    pthread_join(exclusive_t, NULL);
    pthread_join(nine_t, NULL);

exit_run:
    pthread_join(inclusive_t, NULL);
    free(_inc_args);
    free(_exc_args);
    free(_nine_args);
    pthread_mutex_destroy(&_lock);    
}


void simulator::start_simulator(const char* filename, int numtraces) {

    LOCK fprintf(_debug, "%s: starting simulator\n", __func__); UNLOCK

    char* tmpfilename = (char *) malloc(strlen(filename));
    strcpy(tmpfilename, filename);
    char *token = strtok(tmpfilename, "/");
    token = strtok(NULL, "/");
    token = strtok(NULL, "/");
    token = strtok(token, ".");
    
    FILE *fp;
    char input_name[256];

    struct entry *_entry = (struct entry*)malloc(sizeof(struct entry));


    for (int k=0; k<numtraces; k++) {
        sprintf(input_name, "%s_%d", filename, k);
        // fprintf(_debug,"inputname: %s\n", input_name);
        fp = fopen(input_name, "rb");
        assert(fp != NULL);

        while (!feof(fp)) {
            fread(&_entry->iord, sizeof(char), 1, fp);
            fread(&_entry->type, sizeof(char), 1, fp);
            fread(&_entry->addr, sizeof(unsigned long long), 1, fp);
            fread(&_entry->pc, sizeof(unsigned), 1, fp);

            // LOCK
            // fprintf(_debug,"%s: Processing addr: %p type: %c\n",__func__, _entry->addr, _entry->type);
            // UNLOCK;
            // fflush(_debug);
            process_entry(_entry);
            // Process the entry
        }
        fclose(fp);
        // fprintf(_debug,"Done reading file %d!\n", k);
    }

    free(_entry);


}

void simulator::init_caches(bool assoc) {

    l2_cache = new Cache (8, 64, 512 KB, L2);

    if (assoc) {
        l3_cache = new Cache (32768, 64, 2 MB, L3);
    }
    else l3_cache = new Cache (16, 64, 2 MB, L3);
    // l3_cache = new Cache (32768, 64, 2 MB, L3);

}

void simulator::process_entry(struct entry *_entry) {
    
    /*
        Process the entry based on memory hierarchy defined
        L1 -> L2 -> L3 -> Main Memory
        
    */

    if (lookup_l1(_entry))
        return;
    

    unsigned long long addr = 0;


    l1_misses++;

    // get block for L2 lookup
    l2_cache->get_block(_entry->addr, l2_block);
    // fprintf(_debug,"%s: after l2_block fetch - index: %d\n", __func__, l2_block->index); // clear

    l2_cache->lookup(l2_block);
    // fprintf(_debug,"%s: after lookup 2, valid: %d\n", __func__, l2_block->valid);
    if (l2_block->valid) {
        l2_cache->update_repl_params(l2_block->index, l2_block->way);
        // fprintf(_debug,"%s: L2 cache replacement list updated for index: %d\n",\
             __func__, l2_block->index);
            
        goto clean_l2;
    }

    l2_misses++;

    l3_cache->get_block(_entry->addr, l3_block);
    // fprintf(_debug,"%s: after l3_block fetch - index: %d\n", __func__, l3_block->index); // clear
    l3_cache->lookup(l3_block);
    // fprintf(_debug,"%s: after lookup 3\n", __func__);
    // fprintf(_debug,"%s: L3 way: %d, L3 index: %d, L3 Tag: %lld, Valid: %d\n", \
            __func__, l3_block->way, l3_block->index, l3_block->tag, l3_block->valid);
    
    if (l3_block->valid) {
        
        /*
        EXCLUSIVE: L2-M-L3-H
        * Invalidate the target block in L3
        * Copy the target block back to L2
        * If a block gets evicted from L2, put it in L3
        * If a block gets evicted from L3, enough of this now :(
        */
        if (cache_policy == EXCLUSIVE) {
            l3_cache->invalidate(l3_block);
                
            l2_cache->copy(l2_block); // L2 block is not valid here -> Wont be a problem though
            l2_cache->update_repl_params(l2_block->index, l2_block->way);
            
            if (l2_cache->victim != NULL){
                convert_l2_to_l3(l2_cache->victim, _victim);
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
        else if (cache_policy == INCLUSIVE) {
            // fprintf(_debug,"%s: inside inclusive L3 hit\n", __func__);

            l3_cache->update_repl_params(l3_block->index, l3_block->way);
            // fprintf(_debug,"%s: after update repl_params L3 hit\n", __func__);
            l2_cache->copy(l2_block);
            l2_cache->update_repl_params(l2_block->index, l2_block->way);
            // fprintf(_debug,"%s: afer copying block to L2 cache\n", __func__);
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

        goto clean_l3;
    }

    l3_misses++;

    if (fully_assoc)
        cold_and_capacity_misses ++;
    
    // Handle L3 miss
    // After fetching block from main memory

    /*
    EXCLUSIVE: L2-M-L3-M
    * Put the target block in L2
    * If a block gets evicted from L2, put it in L3
    * If a block gets evicted from L3, no problem !
    */
    if (cache_policy == EXCLUSIVE) {
        l2_cache->copy(l2_block);
        l2_cache->update_repl_params(l2_block->index, l2_block->way);

        /*
        Check if any block got replaced in L2 cache
        */
        if (l2_cache->victim != NULL) {
            convert_l2_to_l3(l2_cache->victim, _victim);
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
    else if (cache_policy == INCLUSIVE) {
        // fprintf(_debug,"%s: inside inclusive L3 miss\n", __func__);

        l3_cache->copy(l3_block);
        // fprintf(_debug,"%s: after copying block with index = %d, tag = %lld and valid = %d to L3 cache\n",
                // __func__, l3_block->index, l3_block->tag, l3_block->valid);
        l3_cache->update_repl_params(l3_block->index, l3_block->way);
        /*
            Check if any block got evicted from L3, invalidate it from L2 also
            Don't act stupid, you are being watched!
        */
        if (l3_cache->victim != NULL) {
            // fprintf(_debug, "%s: victim index : %d, way: %d, tag: %lld\n", __func__,\
                l3_cache->victim->index, l3_cache->victim->way, l3_cache->victim->tag);
            
            // fprintf(_debug,"%s: L3 victim not null\n", __func__);
            convert_l3_to_l2(l3_cache->victim, _victim);
            // fprintf(_debug,"%s: conversion to L2 block done. L2 Index: %d, Tag: %lld\n",\
                     __func__, _victim->index, _victim->tag);
            
            l2_cache->lookup(_victim);
            // fprintf(_debug,"%s: Lookup of L2 cache done for victim block\n", __func__);
            
            if (_victim->valid) {
                //throw_error("inclusive property violated for %lx\n", _victim->tag);
                l2_cache->invalidate(_victim);
                // fprintf(_debug,"%s: Invalidated L2 Index: %d, Tag: %lld,  Way: %d\n", \
                    __func__, _victim->index, _victim->tag, _victim->way);
            }
            
            
        }

        // fprintf(_debug, "%s: copy block to L2 index: %d, tag: %lld\n", __func__, l2_block->index, l2_block->tag);
        l2_cache->copy(l2_block);
        // fprintf(_debug, "%s: update repl params L2 with index = %d, way = %d\n", __func__, l2_block->index, l2_block->way);
        l2_cache->update_repl_params(l2_block->index, l2_block->way);
        // fprintf(_debug,"%s: after copying block with index = %d, tag = %lld and valid = %d to L2 cache\n",
        //         __func__, l2_block->index, l2_block->tag, l2_block->valid);
        
        // fprintf(_debug,"%s: End of function\n", __func__);

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
        // if (l2_cache->victim != NULL) {
        //     _victim = convert_l2_to_l3(l2_cache->victim);
        //     l3_cache->copy(_victim);
        // }

    }


clean_l3:
    l3_block->index = -1;
    l3_block->tag = -1;
    l3_block->valid = false;
    l3_block->way = -1;

    _victim->index = -1;
    _victim->tag = 0;
    _victim->valid = false;
    _victim->way = -1;

    if (!is_null(l2_cache->victim)) free(l2_cache->victim);
    if (!is_null(l3_cache->victim)) free(l3_cache->victim);
    
clean_l2:
    l2_block->index = -1;
    l2_block->tag = 0;
    l2_block->valid = false;
    l2_block->way = -1;
    l2_cache->victim = NULL;
    l3_cache->victim = NULL;
  
    // fprintf(_debug, "addr of L2_block: %p, L3_block:%p, Victim: %p\n", l2_block, l3_block, _victim);

    fflush(_debug);
    return;
}

void simulator::print_stats() {
    cout << "L1 Misses " << l1_misses << endl;
    cout << "L2 Misses " << l2_misses << endl;
    cout << "L3 Misses " << l3_misses << endl;
}

void simulator::clean_memory() {
    
    free(l2_cache);
    free(l3_cache);
    free(l2_block);
    free(l3_block);
    free(_victim);
}