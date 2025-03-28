/**************************************************************************
  * C S 429 system emulator
 * 
 * cache.c - A cache simulator that can replay traces from Valgrind
 *     and output statistics such as number of hits, misses, and
 *     evictions, both dirty and clean.  The replacement policy is LRU. 
 *     The cache is a writeback cache. 
 * 
 * Copyright (c) 2021, 2023. 
 * Authors: M. Hinton, Z. Leeper.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/ 
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include "cache.h"

#define ADDRESS_LENGTH 64

/* Counters used to record cache statistics in printSummary().
   test-cache uses these numbers to verify correctness of the cache. */

//Increment when a miss occurs
int miss_count = 0;

//Increment when a hit occurs
int hit_count = 0;

//Increment when a dirty eviction occurs
int dirty_eviction_count = 0;

//Increment when a clean eviction occurs
int clean_eviction_count = 0;

/* STUDENT TO-DO: add more globals, structs, macros if necessary */
uword_t next_lru;

//Student TODO Comment this in for CSIM
static size_t _log(size_t x) {
    size_t result = 0;
    while(x>>=1)  {
    result++;
    }
    return result;
}

/*
 * Initialize the cache according to specified arguments
 * Called by cache-runner so do not modify the function signature
 *
 * The code provided here shows you how to initialize a cache structure
 * defined above. It's not complete and feel free to modify/add code.
 */
cache_t *create_cache(int A_in, int B_in, int C_in, int d_in) {
    /* see cache-runner for the meaning of each argument */
    cache_t *cache = malloc(sizeof(cache_t));
    cache->A = A_in;
    cache->B = B_in;
    cache->C = C_in;
    cache->d = d_in;
    unsigned int S = cache->C / (cache->A * cache->B);

    cache->sets = (cache_set_t*) calloc(S, sizeof(cache_set_t));
    for (unsigned int i = 0; i < S; i++){
        cache->sets[i].lines = (cache_line_t*) calloc(cache->A, sizeof(cache_line_t));
        for (unsigned int j = 0; j < cache->A; j++){
            cache->sets[i].lines[j].valid = 0;
            cache->sets[i].lines[j].tag   = 0;
            cache->sets[i].lines[j].lru   = 0;
            cache->sets[i].lines[j].dirty = 0;
            cache->sets[i].lines[j].data  = calloc(cache->B, sizeof(byte_t));
        }
    }
    /* TODO: add more code for initialization */
    next_lru = 0;
    return cache;
}

cache_t *create_checkpoint(cache_t *cache) {
    unsigned int S = (unsigned int) cache->C / (cache->A * cache->B);
    cache_t *copy_cache = malloc(sizeof(cache_t));
    memcpy(copy_cache, cache, sizeof(cache_t));
    copy_cache->sets = (cache_set_t*) calloc(S, sizeof(cache_set_t));
    for (unsigned int i = 0; i < S; i++) {
        copy_cache->sets[i].lines = (cache_line_t*) calloc(cache->A, sizeof(cache_line_t));
        for (unsigned int j = 0; j < cache->A; j++) {
            memcpy(&copy_cache->sets[i].lines[j], &cache->sets[i].lines[j], sizeof(cache_line_t));
            copy_cache->sets[i].lines[j].data = calloc(cache->B, sizeof(byte_t));
            memcpy(copy_cache->sets[i].lines[j].data, cache->sets[i].lines[j].data, sizeof(byte_t));
        }
    }
    
    return copy_cache;
}

void display_set(cache_t *cache, unsigned int set_index) {
    unsigned int S = (unsigned int) cache->C / (cache->A * cache->B);
    if (set_index < S) {
        cache_set_t *set = &cache->sets[set_index];
        for (unsigned int i = 0; i < cache->A; i++) {
            printf ("Valid: %d Tag: %llx Lru: %lld Dirty: %d\n", set->lines[i].valid, 
                set->lines[i].tag, set->lines[i].lru, set->lines[i].dirty);
        }
    } else {
        printf ("Invalid Set %d. 0 <= Set < %d\n", set_index, S);
    }
}

/*
 * Free allocated memory. Feel free to modify it
 */
void free_cache(cache_t *cache) {
    unsigned int S = (unsigned int) cache->C / (cache->A * cache->B);
    for (unsigned int i = 0; i < S; i++){
        for (unsigned int j = 0; j < cache->A; j++) {
            free(cache->sets[i].lines[j].data);
        }
        free(cache->sets[i].lines);
    }
    free(cache->sets);
    free(cache);
}

/* STUDENT TO-DO:
 * Get the line for address contained in the cache
 * On hit, return the cache line holding the address
 * On miss, returns NULL
 */
cache_line_t *get_line(cache_t *cache, uword_t addr) {
    /* your implementation */   

    int b = _log(cache->B);
    int s = _log(cache->C / (cache->A * cache->B));
    uword_t index = (addr >> b) & (((1 << s) - 1));
    uword_t tag = addr >> (s + b);

    cache_set_t *temp_set = &(cache->sets[index]);
    for (int i = 0; i < cache->A; i++) {
        if (temp_set->lines[i].valid && temp_set->lines[i].tag == tag) {
            //hit
            return &(temp_set->lines[i]);
        }
    }
    return NULL;
}

/* STUDENT TO-DO:
 * Select the line to fill with the new cache line
 * Return the cache line selected to filled in by addr
 */
cache_line_t *select_line(cache_t *cache, uword_t addr) {
    next_lru++;
    /* your implementation */
    int b = _log(cache->B);
    int s = _log(cache->C / (cache->A * cache->B));
    uword_t index = (addr >> b) & (((1 << s) - 1));
    cache_set_t *temp_set = &(cache->sets[index]);
    //is there an empty line? if so, return it
    unsigned long long lowest_lru = -1;
    cache_line_t *return_line;
    for (int i = 0; i < cache->A; i++) {
        if (!temp_set->lines[i].valid) {
            //empty
            return_line =  &(temp_set->lines[i]);
            break;
        } else if (lowest_lru > temp_set->lines[i].lru ){
            return_line = &temp_set->lines[i];
            lowest_lru = temp_set->lines[i].lru;
        }
    }
    return return_line;
}

/*  STUDENT TO-DO:
 *  Check if the address is hit in the cache, updating hit and miss data.
 *  Return true if pos hits in the cache.
 */
bool check_hit(cache_t *cache, uword_t addr, operation_t operation) {
    next_lru++;
    int b = _log(cache->B);
    int s = _log(cache->C / (cache->A * cache->B));
    uword_t index = (addr >> b) & (((1 << s) - 1));
    uword_t tag = addr >> (s + b);

    cache_set_t *temp_set = &(cache->sets[index]);
    for (int i = 0; i < cache->A; i++) {
        if (temp_set->lines[i].valid && temp_set->lines[i].tag == tag) {
            //found
            temp_set->lines[i].lru = next_lru;
            hit_count++;
            if (operation == WRITE) {
                temp_set->lines[i].dirty = true;
            }
            return true;
        }
    }
    miss_count++;
    return false;
}

/*  STUDENT TO-DO:
 *  Handles Misses, evicting from the cache if necessary.
 *  Fill out the evicted_line_t struct with info regarding the evicted line.
 */
evicted_line_t *handle_miss(cache_t *cache, uword_t addr, operation_t operation, byte_t *incoming_data) {
    next_lru++;
    evicted_line_t *evicted_line = malloc(sizeof(evicted_line_t));
    evicted_line->data = (byte_t *) calloc(cache->B, sizeof(byte_t));
    /* your implementation */
    int b = _log(cache->B);
    int s = _log(cache->C / (cache->A * cache->B));
    uword_t index = (addr >> b) & (((1 << s) - 1));
    uword_t tag = addr >> (s + b);
   // uword_t offset = addr & ((1 << b) - 1);
    cache_line_t *line_to_replace = select_line(cache, addr);
    if(line_to_replace->valid){
        if (line_to_replace->dirty) {
            dirty_eviction_count++;
        } else {
            clean_eviction_count++;
        }
    }
    //get data, linetoreplace -> evicted
    if (line_to_replace->data != NULL) {
        memcpy(evicted_line->data, line_to_replace->data, cache->B);
    }
    evicted_line->valid = line_to_replace->valid;
    evicted_line->dirty = line_to_replace->dirty;
    evicted_line->addr = (line_to_replace->tag << (s+b)) + ((index) << b) + (addr & ((1 << b) - 1));
    //set data, incoming -> linetoreplace
    if (incoming_data != NULL) {
        memcpy(line_to_replace->data, incoming_data, cache->B);
    }
    line_to_replace->tag = tag;
    line_to_replace->valid = true;
    line_to_replace->dirty = (operation == WRITE);
    line_to_replace->lru = next_lru;
    return evicted_line;
}

/* STUDENT TO-DO:
 * Get 8 bytes from the cache and write it to dest.
 * Preconditon: addr is contained within the cache.
 */


void get_word_cache(cache_t *cache, uword_t addr, word_t *dest) {
    /* Your implementation */

    int b = _log(cache->B);
    cache_line_t *line = get_line(cache, addr);
    if (line != NULL && line->valid) 
    {
        uword_t block_offset = (addr & ((1 << b) - 1));
        memcpy(dest, (word_t*)(line->data + block_offset), 8);
        line->lru = next_lru;
    }
    
}

/* STUDENT TO-DO:
 * Set 8 bytes in the cache to val at pos.
 * Preconditon: addr is contained within the cache.
 */
void set_word_cache(cache_t *cache, uword_t addr, word_t val) {
    /* Your implementation */
    int b = _log(cache->B);
    cache_line_t *line = get_line(cache, addr);
    if (line != NULL  && line->valid) 
    {
        uword_t block_offset = (addr & ((1 << b) - 1));
        memcpy((word_t*)(line->data + block_offset), &val, 8);
        line->dirty = 1;
        line->valid = true;
        line->lru = next_lru;
    }
}

/*
 * Access data at memory address addr
 * If it is already in cache, increase hit_count
 * If it is not in cache, bring it in cache, increase miss count
 * Also increase eviction_count if a line is evicted
 *
 * Called by cache-runner; no need to modify it if you implement
 * check_hit() and handle_miss()
 */
void access_data(cache_t *cache, uword_t addr, operation_t operation)
{
    if(!check_hit(cache, addr, operation))
        free(handle_miss(cache, addr, operation, NULL));
}
