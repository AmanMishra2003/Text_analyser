

#ifndef BUFFER_ANALYSER_H
#define BUFFER_ANALYSER_H

#include <stdio.h>
#include <wchar.h> 
#include <wctype.h>
#include <locale.h> 
#include <stdbool.h> 
#include "freq_counter.h" 
#include <stdlib.h> 
#include <stddef.h> // For size_t
#include <string.h> // For memset

// Function to process a memory block and extract letter frequencies and word count
// Signature changed to process buffer slice (start, length)
static inline FrequencyData extract_frequencies_from_buffer(const wchar_t *buffer, size_t length) {
    
    // Initialize all fields.
    FrequencyData data = { {0}, 0.0, 0.0, {0}, {0}, 0 }; // Initialize both hash maps to 0
    memset(&data, 0, sizeof(FrequencyData)); // Safety memset

    wint_t wc; 
    wint_t wc_prev = L'\0'; // Initialize previous character for bigram counting
    bool in_word = false; 

    // Loop directly over the memory buffer up to the specified length
    for (size_t i = 0; i < length; i++) {
        wc = buffer[i];
        
        // 1. Word Counting Logic
        bool is_word_char = iswalpha(wc) || wc == L'\'' || wc == L'-';

        if (is_word_char) {
            if (!in_word) {
                data.total_words++;
                in_word = true;
            }
        } 
        else {
            in_word = false; 
        }

        // 2. 40-Bin Letter Frequency Logic (for Monograph Chi-Square)
        if (iswalpha(wc)) {
            process_letter_frequency(wc, &data); 
        }

        // 3. BIGRAM Counting Logic (NEW)
        if (i > 0) {
            process_bigram_count(wc_prev, wc, &data);
        }

        // 4. ALL Character Counting Logic (for Comprehensive Histogram)
        if (!iswspace(wc)) {
            process_all_character_count(wc, &data);
        }

        // Update previous character for the next iteration (only track letters/hyphens/apostrophes)
        // Only track characters relevant to forming a bigram (alphabetical) for the statistical test
        if (iswalpha(wc)) {
             wc_prev = wc;
        } else {
             wc_prev = L'\0';
        }
    }
    
    if (data.total_letters < 5) {
        data.error_code = 1; // Mark segment as having insufficient data
    }
    
    return data;
}

#endif // BUFFER_ANALYSER_H