#ifndef FREQ_COUNTER_H
#define FREQ_COUNTER_H

#include <wchar.h> 
#include <wctype.h>
#include <stdbool.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdio.h>
#include <stdint.h> // For uint32_t for bigram key

#define EPS 1e-6
#define TOTAL_BINS 40 // 26 Base Letters + 14 Accented Letters
#define BIGRAM_SIZE 400 // Arbitrary safe size for Bigram Map

// Define the 14 accented characters (lowercase) to be tracked in indices 26-39.
static const wint_t ACCENTED_CHARS[TOTAL_BINS - 26] = {
    L'â', L'à', L'ç', L'ê', L'é', L'è', L'ë', L'ï', L'î', L'ô', L'œ', L'ü', L'û', L'ù'
};

// =======================================================
// HASH MAP IMPLEMENTATIONS
// =======================================================

// A safe size (power of 2) for the hash table array (buckets)
#define HASH_TABLE_SIZE 1024 

// --- 1. All Character Count Map (Unchanged from last fix) ---
typedef struct CharCountEntry { 
    wint_t character; 
    double count;     
} CharCountEntry;

typedef struct CharMapNode {
    wint_t character; 
    double count; 
    struct CharMapNode *next;
} CharMapNode;

typedef struct CharMap {
    CharMapNode *table[HASH_TABLE_SIZE];
    int total_unique_chars;
} CharMap;

// --- 2. Bigram Count Map (NEW) ---
// Key is a single 32-bit integer combining two lowercase letters
typedef struct BigramNode {
    uint32_t key; // (char1 << 16) | char2
    double count; 
    struct BigramNode *next;
} BigramNode;

typedef struct BigramMap {
    BigramNode *table[HASH_TABLE_SIZE];
    int total_unique_bigrams;
    double total_bigrams;
} BigramMap;

// Structure to hold all counting results
typedef struct FrequencyData {
    double observed_freq[TOTAL_BINS]; 
    double total_letters; 
    double total_words; 
    
    CharMap all_char_map; 
    BigramMap bigram_map; // NEW: The bigram hash map
    
    int error_code; 
} FrequencyData;


// =======================================================
// HELPER FUNCTIONS
// =======================================================

// Simple modulo hashing
static inline unsigned int hash_key(uint32_t key) {
    return key % HASH_TABLE_SIZE;
}

// Maps a wide character to its 0-39 bin index (A-Z or accented)
static inline int map_letter_to_index(wint_t wc) {
    wint_t lower = towlower(wc);
    
    if (lower >= L'a' && lower <= L'z') {
        return lower - L'a'; // 0-25 for A-Z
    }

    // Check accented characters (26-39)
    for (int i = 0; i < TOTAL_BINS - 26; i++) {
        if (lower == ACCENTED_CHARS[i]) {
            return 26 + i;
        }
    }
    return -1; // Not a tracked character
}

// Updates the 40-bin letter frequency (for Monograph Chi-Square)
static inline void process_letter_frequency(wint_t wc, FrequencyData *data) {
    if (iswalpha(wc)) {
        int index = map_letter_to_index(wc); 
        
        if (index != -1) {
            data->observed_freq[index]++;
            data->total_letters++; 
        }
    }
}

// =======================================================
// CORE HASH MAP COUNTING LOGIC (BIGRAMS - NEW)
// =======================================================
static inline void process_bigram_count(wint_t wc_prev, wint_t wc_curr, FrequencyData *data) {
    
    // Only count bigrams of two alphabetical characters (normalize case)
    if (!iswalpha(wc_prev) || !iswalpha(wc_curr)) {
        return;
    }

    wint_t char1 = towlower(wc_prev);
    wint_t char2 = towlower(wc_curr);
    
    // Combine two 16-bit wide characters into a 32-bit key
    // This is the unique identifier for the bigram "char1-char2"
    uint32_t key = ((uint32_t)char1 << 16) | (uint32_t)char2;
    unsigned int index = hash_key(key);

    BigramNode *current = data->bigram_map.table[index];

    // 1. Search (Lookup in bucket)
    while (current != NULL) {
        if (current->key == key) {
            current->count++; // Found, increment count
            data->bigram_map.total_bigrams++;
            return;
        }
        current = current->next;
    }

    // 2. Insert (Not found, create new node)
    BigramNode *new_node = (BigramNode *)malloc(sizeof(BigramNode));
    if (new_node == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for bigram map node.\n");
        return;
    }

    new_node->key = key;
    new_node->count = 1.0;
    
    // Insert at the head of the list
    new_node->next = data->bigram_map.table[index];
    data->bigram_map.table[index] = new_node;

    data->bigram_map.total_unique_bigrams++;
    data->bigram_map.total_bigrams++;
}


// =======================================================
// CORE HASH MAP COUNTING LOGIC (ALL CHARACTERS - Unchanged)
// =======================================================
static inline void process_all_character_count(wint_t wc, FrequencyData *data) {
    
    // Normalize letters to lowercase for aggregation in the Full Character Map
    wint_t char_to_count = wc;
    if (iswalpha(wc)) {
        char_to_count = towlower(wc);
    }
    
    unsigned int index = hash_key((uint32_t)char_to_count); 
    CharMapNode *current = data->all_char_map.table[index];

    // 1. Search (Lookup in bucket)
    while (current != NULL) {
        if (current->character == char_to_count) {
            current->count++; 
            return;
        }
        current = current->next;
    }

    // 2. Insert (Not found, create new node)
    CharMapNode *new_node = (CharMapNode *)malloc(sizeof(CharMapNode));
    if (new_node == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for hash map node.\n");
        return;
    }

    new_node->character = char_to_count; 
    new_node->count = 1.0;
    
    new_node->next = data->all_char_map.table[index];
    data->all_char_map.table[index] = new_node;

    data->all_char_map.total_unique_chars++;
}

// =======================================================
// CLEANUP LOGIC (Updated to free Bigram map nodes)
// =======================================================
static inline void cleanup_frequency_data(FrequencyData *data) {
    // 1. Free all CharMap nodes
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        CharMapNode *current = data->all_char_map.table[i];
        CharMapNode *temp;
        while (current != NULL) {
            temp = current;
            current = current->next;
            free(temp);
        }
        data->all_char_map.table[i] = NULL;
    }

    // 2. Free all BigramMap nodes (NEW)
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        BigramNode *current = data->bigram_map.table[i];
        BigramNode *temp;
        while (current != NULL) {
            temp = current;
            current = current->next;
            free(temp);
        }
        data->bigram_map.table[i] = NULL;
    }
}

#endif // FREQ_COUNTER_H