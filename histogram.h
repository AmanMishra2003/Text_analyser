#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <stdio.h>
#include <math.h> 
#include <stdlib.h> // For qsort
#include <wchar.h> // For wint_t and wprintf
#include <wctype.h> // For iswprint
#include <stdbool.h> // For bool type
// Note: We rely on definitions like FrequencyData, TOTAL_BINS, CharMap, CharMapNode, HASH_TABLE_SIZE, and ACCENTED_CHARS from freq_counter.h
#include "freq_counter.h" 

#define MAX_BAR_LENGTH 50 

// Helper structure to link a letter/character with its count for sorting
// (Re-defined here as the external map_to_array function is no longer used)
typedef struct {
    wint_t character; 
    double count;
} CountEntry; 

// Comparison function for qsort: sorts in descending order (highest count first)
int compare_letter_counts(const void *a, const void *b) {
    CountEntry *lc_a = (CountEntry *)a;
    CountEntry *lc_b = (CountEntry *)b;
    
    if (lc_b->count > lc_a->count) return 1;
    if (lc_b->count < lc_a->count) return -1;
    return 0;
}

// Function to print a generic histogram block
static inline void print_histogram_block(const char *title, CountEntry *counts, int num_entries, double max_freq, bool is_char_map) {
    
    printf("\n======================================================\n");
    printf(" %s\n", title);
    printf("======================================================\n");
    
    for (int i = 0; i < num_entries; i++) {
        wint_t character = counts[i].character;
        double count = counts[i].count;
        
        int bar_length = (int)ceil((count / max_freq) * MAX_BAR_LENGTH); 
        
        if (is_char_map && !iswprint(character)) {
            printf("0x%04X | %6.0f | ", character, count);
        } else if (is_char_map && character == L' ') {
            printf("[SPC] | %6.0f | ", count);
        } else {
            wprintf(L"%lc | %6.0f | ", character, count);
        }

        for (int j = 0; j < bar_length; j++) {
            printf("*");
        }
        printf("\n");
    }
}


// --- FUNCTION: Print the Comprehensive Histogram (All Characters) ---
static inline void print_all_char_histogram(const FrequencyData *data) {
    
    // Get the exact number of unique characters from the hash map structure.
    int num_unique_chars = data->all_char_map.total_unique_chars;

    if (num_unique_chars == 0) return;

    // Dynamically allocate array to hold the count entries for sorting.
    CountEntry *all_char_counts = (CountEntry *)malloc(num_unique_chars * sizeof(CountEntry));
    if (all_char_counts == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for all-char histogram.\n");
        return;
    }

    // --- Direct Hash Map Traversal Logic (Fix for map_to_array) ---
    double max_freq = 0.0;
    int current_index = 0;
    
    // Iterate through all buckets of the hash table (HASH_TABLE_SIZE is defined in freq_counter.h)
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        CharMapNode *current = data->all_char_map.table[i];
        
        // Traverse the linked list (chain) in the current bucket
        while (current != NULL) {
            if (current_index < num_unique_chars) {
                all_char_counts[current_index].character = current->character;
                all_char_counts[current_index].count = current->count;
                
                if (current->count > max_freq) {
                    max_freq = current->count;
                }
                current_index++;
            }
            current = current->next;
        }
    }
    // --- End of Traversal Logic ---
    
    if (max_freq < EPS) {
        free(all_char_counts);
        return;
    }

    qsort(all_char_counts, num_unique_chars, sizeof(CountEntry), compare_letter_counts);

    print_histogram_block("FULL CHARACTER FREQUENCIES (Letters, Punctuation, Symbols)", 
                          all_char_counts, 
                          num_unique_chars, 
                          max_freq,
                          true); 

    free(all_char_counts);
}


// --- FUNCTION: Print the Letter Frequency Histogram (A-Z + 14 Accents) ---
static inline void print_letter_histogram(const FrequencyData *data) {
    
    if (data->total_letters < EPS) {
        printf("\nCannot generate Letter Frequency histogram: No valid letter data available.\n");
        return;
    }

    CountEntry letter_counts[TOTAL_BINS]; 
    double max_freq = 0.0;
    
    // 1. Prepare data and find the absolute maximum frequency
    for (int i = 0; i < TOTAL_BINS; i++) {
        wint_t character;
        if (i < 26) {
            character = L'a' + i; // a-z
        } else {
            character = ACCENTED_CHARS[i - 26]; 
        }
        
        letter_counts[i].character = character;
        letter_counts[i].count = data->observed_freq[i];
        if (data->observed_freq[i] > max_freq) {
            max_freq = data->observed_freq[i];
        }
    }
    
    if (max_freq < EPS) {
        printf("\nCannot generate Letter Frequency histogram: All observed frequencies are zero.\n");
        return;
    }

    // 2. --- TOP 5 HISTOGRAM (Sorted) ---
    qsort(letter_counts, TOTAL_BINS, sizeof(CountEntry), compare_letter_counts); 
    
    print_histogram_block("TOP 5 Letter Frequencies (A-Z + 14 Accents)", letter_counts, 5, max_freq, false);
}


// Main function called by main.c
static inline void print_all_histograms(const FrequencyData *data) {
    print_letter_histogram(data);
    print_all_char_histogram(data);
}

#endif // HISTOGRAM_H