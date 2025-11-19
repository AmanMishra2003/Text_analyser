#ifndef CHI_SQUARED_H
#define CHI_SQUARED_H

#include <stdio.h>
#include <math.h>
#include "buffer_analyser.h" 
#include "freq_counter.h" 
#include <stdlib.h> 
#include <stddef.h> // For size_t
#include <stdint.h> // For uint32_t
#include <wchar.h> // For wint_t

// Language IDs to be returned
#define LANG_ENG 0
#define LANG_FRE 1
#define LANG_ERROR -1

// Define the number of top bigrams to track and use for the statistical test
#define TOP_BIGRAMS 20 

// --- Reference Monograph Frequencies (TOTAL_BINS = 40) - Unchanged ---
static const double ENGLISH_FREQ[TOTAL_BINS]={ 
    // A-Z (26 slots)
    8.167, 1.492, 2.782, 4.253, 12.702, 2.228, 2.015, 6.094, 6.966, 0.153, 
    0.772, 4.025, 2.406, 6.749, 7.507, 1.929, 0.095, 5.987, 6.327, 9.056, 
    2.758, 0.978, 2.360, 0.150, 1.974, 0.074, 
    // Accented letters (14 slots) - STATISTICAL FLOOR
    EPS, EPS, EPS, EPS, EPS, EPS, EPS, EPS, EPS, EPS, EPS, EPS, EPS, EPS
};

static const double FRENCH_FREQ[TOTAL_BINS]={
    // A-Z (26 slots)
    7.636, 0.901, 3.260, 3.669, 14.715, 1.066, 0.866, 0.737, 7.529, 0.545, 
    0.049, 5.456, 2.804, 7.095, 5.378, 3.021, 1.362, 6.692, 8.140, 7.244, 
    5.484, 1.087, 0.063, 0.417, 0.230, 0.121, 
    // Accented letters (14 slots)
    0.057, 0.536, 0.854, 0.165, 1.955, 0.271, 0.125, 0.203, 0.053, 0.051, 
    0.007, 0.063, 0.080, 0.060
};

// --- Reference Bigram Frequencies (NEW) ---
// Key is a 32-bit integer: (char1 << 16) | char2
// Value is the percentage frequency
typedef struct {
    uint32_t key;
    double freq;
} BigramRef;

// Helper macro to generate the 32-bit key from two chars
#define MAKE_BIGRAM_KEY(c1, c2) (((uint32_t)L##c1 << 16) | (uint32_t)L##c2)

// Top 20 Bigrams for English (Source: Standard Linguistic Data)
static const BigramRef ENGLISH_BIGRAM_FREQ[TOP_BIGRAMS] = {
    {MAKE_BIGRAM_KEY('t','h'), 3.49}, {MAKE_BIGRAM_KEY('h','e'), 3.09}, {MAKE_BIGRAM_KEY('i','n'), 2.43}, 
    {MAKE_BIGRAM_KEY('e','r'), 2.10}, {MAKE_BIGRAM_KEY('a','n'), 2.01}, {MAKE_BIGRAM_KEY('r','e'), 1.85}, 
    {MAKE_BIGRAM_KEY('o','n'), 1.71}, {MAKE_BIGRAM_KEY('a','t'), 1.49}, {MAKE_BIGRAM_KEY('n','d'), 1.34}, 
    {MAKE_BIGRAM_KEY('t','i'), 1.25}, {MAKE_BIGRAM_KEY('e','s'), 1.20}, {MAKE_BIGRAM_KEY('o','f'), 1.18}, 
    {MAKE_BIGRAM_KEY('e','n'), 1.17}, {MAKE_BIGRAM_KEY('e','d'), 1.16}, {MAKE_BIGRAM_KEY('i','s'), 1.13}, 
    {MAKE_BIGRAM_KEY('t','o'), 1.09}, {MAKE_BIGRAM_KEY('o','u'), 1.05}, {MAKE_BIGRAM_KEY('a','l'), 1.04}, 
    {MAKE_BIGRAM_KEY('c','e'), 1.03}, {MAKE_BIGRAM_KEY('s','t'), 1.01}
};

// Top 20 Bigrams for French (Source: Standard Linguistic Data)
static const BigramRef FRENCH_BIGRAM_FREQ[TOP_BIGRAMS] = {
    {MAKE_BIGRAM_KEY('e','s'), 3.65}, {MAKE_BIGRAM_KEY('l','e'), 2.62}, {MAKE_BIGRAM_KEY('d','e'), 2.58}, 
    {MAKE_BIGRAM_KEY('e','n'), 2.37}, {MAKE_BIGRAM_KEY('l','a'), 2.32}, {MAKE_BIGRAM_KEY('n','t'), 2.29}, 
    {MAKE_BIGRAM_KEY('e','r'), 2.13}, {MAKE_BIGRAM_KEY('o','n'), 1.83}, {MAKE_BIGRAM_KEY('a','i'), 1.79}, 
    {MAKE_BIGRAM_KEY('t','e'), 1.77}, {MAKE_BIGRAM_KEY('q','u'), 1.73}, {MAKE_BIGRAM_KEY('a','s'), 1.69}, 
    {MAKE_BIGRAM_KEY('o','n'), 1.57}, {MAKE_BIGRAM_KEY('e','l'), 1.55}, {MAKE_BIGRAM_KEY('n','s'), 1.51}, 
    {MAKE_BIGRAM_KEY('p','a'), 1.48}, {MAKE_BIGRAM_KEY('r','e'), 1.47}, {MAKE_BIGRAM_KEY('i','o'), 1.45}, 
    {MAKE_BIGRAM_KEY('e','t'), 1.44}, {MAKE_BIGRAM_KEY('v','o'), 1.41}
};

// --- Bigram Chi-Squared Calculation ---
static inline double calculate_bigram_chi(const BigramMap *map, const BigramRef *ref_freq) {
    if (map->total_bigrams < EPS) {
        return 99999.0; // Return a huge score if no bigrams were found
    }

    double chi = 0.0;
    double total_bigrams = map->total_bigrams;
    
    // Iterate through the top reference bigrams
    for (int i = 0; i < TOP_BIGRAMS; i++) {
        uint32_t ref_key = ref_freq[i].key;
        double ref_pct = ref_freq[i].freq;
        
        // 1. Find the observed count for this bigram in the input text
        unsigned int index = hash_key(ref_key);
        const BigramNode *current = map->table[index];
        double observed_count = 0.0;
        
        while (current != NULL) {
            if (current->key == ref_key) {
                observed_count = current->count;
                break;
            }
            current = current->next;
        }

        // 2. Calculate Expected Count
        double expected_count = (ref_pct / 100.0) * total_bigrams;

        // 3. Chi-Squared formula (adding a small EPS to the denominator)
        double diff = observed_count - expected_count;
        chi += (diff * diff) / (expected_count + EPS);
    }
    
    // Weight the bigram score less than the monograph score (e.g., 20% weight)
    return chi * 0.20; 
}


// --- Segment Test Function (Combined Score) ---
int perform_segment_test(const FrequencyData *data) {
    
    if (data->total_letters < 5) {
        return LANG_ERROR; 
    }

    // --- 1. Monograph (Single-Letter) Chi-Squared ---
    double chi1_mono = 0.0; // English Chi-Squared
    double chi2_mono = 0.0; // French Chi-Squared
    double total_letters_pct = data->total_letters / 100.0; // Scaling factor

    for (int i = 0; i < TOTAL_BINS; i++) {
        double expected_eng = ENGLISH_FREQ[i] * total_letters_pct;
        double expected_french = FRENCH_FREQ[i] * total_letters_pct;
        
        double diff1 = data->observed_freq[i] - expected_eng;
        chi1_mono += (diff1 * diff1) / (expected_eng + EPS); 
        
        double diff2 = data->observed_freq[i] - expected_french;
        chi2_mono += (diff2 * diff2) / (expected_french + EPS); 
    }
    
    // --- 2. Bigram (Two-Letter) Chi-Squared (NEW) ---
    double chi1_bigram = calculate_bigram_chi(&data->bigram_map, ENGLISH_BIGRAM_FREQ);
    double chi2_bigram = calculate_bigram_chi(&data->bigram_map, FRENCH_BIGRAM_FREQ);

    // --- 3. Combined Final Score ---
    // Total score is the sum of Monograph and Bigram scores.
    double chi1_final = chi1_mono + chi1_bigram;
    double chi2_final = chi2_mono + chi2_bigram;

    if (chi1_final < chi2_final) {
        return LANG_ENG;
    } else {
        return LANG_FRE;
    }
}

// --- Final Analysis Function (Updated to report both scores) ---
void perform_final_analysis(const FrequencyData *data, size_t eng_chars, size_t fre_chars) {
    
    // --- 1. Monograph (Single-Letter) Chi-Squared ---
    double chi1_mono = 0.0;
    double chi2_mono = 0.0;
    double total_letters_pct = data->total_letters / 100.0;

    for (int i = 0; i < TOTAL_BINS; i++) {
        double expected_eng = ENGLISH_FREQ[i] * total_letters_pct;
        double expected_french = FRENCH_FREQ[i] * total_letters_pct;
        
        double diff1 = data->observed_freq[i] - expected_eng;
        chi1_mono += (diff1 * diff1) / (expected_eng + EPS); 
        
        double diff2 = data->observed_freq[i] - expected_french;
        chi2_mono += (diff2 * diff2) / (expected_french + EPS); 
    }
    
    // --- 2. Bigram (Two-Letter) Chi-Squared (NEW) ---
    double chi1_bigram = calculate_bigram_chi(&data->bigram_map, ENGLISH_BIGRAM_FREQ);
    double chi2_bigram = calculate_bigram_chi(&data->bigram_map, FRENCH_BIGRAM_FREQ);
    
    // --- 3. Combined Final Score ---
    double chi1_final = chi1_mono + chi1_bigram;
    double chi2_final = chi2_mono + chi2_bigram;
    
    // Calculate final proportions for reporting
    double total_seg_chars = (double)(eng_chars + fre_chars);
    double prob_english = (total_seg_chars > 0.0) ? ((double)eng_chars / total_seg_chars) * 100.0 : 0.0;
    double prob_french = (total_seg_chars > 0.0) ? ((double)fre_chars / total_seg_chars) * 100.0 : 0.0;


    printf("\n\n======================================================\n");
    printf("     FINAL AGGREGATE LANGUAGE CONCLUSION\n");
    printf("======================================================\n");
    
    printf("Total words counted: %.0f\n", data->total_words); 
    printf("Total letters counted: %.0f\n", data->total_letters);
    
    printf("\nChi-Squared Results (Full Document Aggregate):\n");
    printf("   English Monograph Score (Mono): %.4f\n", chi1_mono);
    printf("   French Monograph Score (Mono):  %.4f\n", chi2_mono);
    printf("   English Bigram Score (Bigram):  %.4f\n", chi1_bigram);
    printf("   French Bigram Score (Bigram):   %.4f\n", chi2_bigram);
    printf("   ---------------------------------------------------\n");
    printf("   English COMBINED Score:         %.4f\n", chi1_final);
    printf("   French COMBINED Score:          %.4f\n", chi2_final);
    
    printf("\nLanguage Proportions (Based on Segmentation):\n");
    printf("Proportion of ENGLISH: %.2f%% (Total %zu segment characters)\n", prob_english, eng_chars);
    printf("Proportion of FRENCH:  %.2f%% (Total %zu segment characters)\n", prob_french, fre_chars); 
    
    printf("\nDOMINANT LANGUAGE OF TEXT:\n");

    if (chi1_final < chi2_final) {
        printf(">>> ENGLISH language (Best Fit by Combined Score) <<<\n");
    } else {
        printf(">>> FRENCH language (Best Fit by Combined Score) <<<\n");
    }
}

#endif // CHI_SQUARED_H