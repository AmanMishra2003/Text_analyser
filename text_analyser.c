#include "chi_squared.h"   
#include "buffer_analyser.h" 
#include "freq_counter.h" 
#include "histogram.h" 
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <string.h>
#include <stddef.h>

// Declare the functions used from header files
FrequencyData extract_frequencies_from_buffer(const wchar_t *buffer, size_t length);
int perform_segment_test(const FrequencyData *data); 
void perform_final_analysis(const FrequencyData *data, size_t eng_chars, size_t fre_chars); 
void cleanup_frequency_data(FrequencyData *data);
void print_all_histograms(const FrequencyData *data); 

// --- Configuration ---
#define WINDOW_SIZE 500 
#define OVERLAP_SIZE 400 
#define STEP_SIZE (WINDOW_SIZE - OVERLAP_SIZE)
#define MIN_WINDOW_SIZE 100 

// --- Helper function to read the entire file into a wide character buffer ---
wchar_t* read_file_to_buffer(const char *filename, size_t *out_size) {
    
    FILE *fptr;
    long file_byte_size;
    
    // Set locale to handle wide characters (Unicode/UTF-8)
    if (setlocale(LC_CTYPE, "") == NULL) {
        fprintf(stderr, "Warning: Could not set system locale.\\n");
    }

    // Use fopen/rb for standard C I/O
    fptr = fopen(filename, "rb"); 
    
    if (fptr == NULL) {
        perror("Error opening file");
        *out_size = 0;
        return NULL;
    }
    
    // 1. Get file size in bytes
    fseek(fptr, 0, SEEK_END);
    file_byte_size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);

    if (file_byte_size <= 0) {
        fclose(fptr);
        *out_size = 0;
        return NULL;
    }
    
    // 2. Allocate buffer for bytes (plus null terminator)
    char *byte_buffer = (char*)malloc(file_byte_size + 1);
    if (byte_buffer == NULL) {
        fprintf(stderr, "Error: Failed to allocate byte buffer.\\n");
        fclose(fptr);
        return NULL;
    }

    // 3. Read file into byte buffer
    if (fread(byte_buffer, 1, file_byte_size, fptr) != file_byte_size) {
        fprintf(stderr, "Error reading file into byte buffer.\\n");
        free(byte_buffer);
        fclose(fptr);
        return NULL;
    }
    fclose(fptr);
    byte_buffer[file_byte_size] = '\0'; // Null-terminate the byte buffer

    // 4. Convert byte buffer (multi-byte UTF-8) to wchar_t buffer (wide characters)
    size_t wide_chars_needed = mbstowcs(NULL, byte_buffer, 0);

    if (wide_chars_needed == (size_t)-1) {
        fprintf(stderr, "Error converting multibyte characters to wide characters (mbstowcs failed).\\n");
        free(byte_buffer);
        *out_size = 0;
        return NULL;
    }
    
    wchar_t *wide_buffer = (wchar_t*)malloc((wide_chars_needed + 1) * sizeof(wchar_t));
    if (wide_buffer == NULL) {
        fprintf(stderr, "Error: Failed to allocate wide buffer.\\n");
        free(byte_buffer);
        *out_size = 0;
        return NULL;
    }

    mbstowcs(wide_buffer, byte_buffer, wide_chars_needed + 1);

    free(byte_buffer);
    *out_size = wide_chars_needed;
    return wide_buffer;
}


int main(int argc, char *argv[]) {
    
    // --- 1. File Reading and Setup ---
    const char *filename;

    if (argc < 2) {
        // Use "hello.txt" as default if no argument is provided
        filename = "hello.txt";
        printf("No filename provided. Using default file: %s\n", filename);
    } else {
        // Use the filename provided on the command line
        filename = argv[1];
    }
    
    size_t file_length = 0;
    wchar_t *file_buffer = read_file_to_buffer(filename, &file_length);

    if (file_buffer == NULL || file_length < MIN_WINDOW_SIZE) {
        fprintf(stderr, "Error: File '%s' is empty, cannot be read, or is too short (%zu chars) for analysis.\\n", filename, file_length);
        free(file_buffer);
        return EXIT_FAILURE;
    }

    printf("Analyzing file: %s (Total wide characters: %zu)\n", filename, file_length);
    printf("Window Size: %d | Overlap: %d | Step: %d\n", WINDOW_SIZE, OVERLAP_SIZE, STEP_SIZE);

    // --- 2. Variables for Segmentation Aggregation ---
    size_t eng_chars_from_segments = 0;
    size_t fre_chars_from_segments = 0;
    
    // --- 3. Sliding Window Loop (FINAL ROBUST LOGIC) ---
    size_t i = 0;
    
    while (i < file_length) {
        
        // Determine the window size (handle the final, possibly smaller segment)
        size_t current_window_size = (i + WINDOW_SIZE <= file_length) ? WINDOW_SIZE : file_length - i;
        
        // Determine the size of the non-overlapping segment for aggregation
        size_t count_to_add = (i + STEP_SIZE <= file_length) ? STEP_SIZE : file_length - i;
        
        // Determine where the window starts next
        size_t next_i = i + STEP_SIZE;
        
        // CRITICAL BREAK: Stop processing if the remaining available segment is too small 
        if (current_window_size < MIN_WINDOW_SIZE) {
            break; 
        }

        // If count_to_add is zero, break.
        if (count_to_add < 1) break; 
        
        // Extract frequencies for the current window slice
        FrequencyData segment_data = extract_frequencies_from_buffer(file_buffer + i, current_window_size);
        
        printf("Chars %05zu-%05zu: ", i, i + current_window_size - 1);
        
        if (segment_data.error_code == 0) {
            int lang_id = perform_segment_test(&segment_data);
            
            // --- CORE LOGIC: Accumulate the non-overlapping count ---
            if (lang_id == LANG_ENG) {
                printf("=> ENGLISH (Adding %zu chars)\n", count_to_add);
                eng_chars_from_segments += count_to_add;
            } else if (lang_id == LANG_FRE) {
                printf("=> FRENCH (Adding %zu chars)\n", count_to_add);
                fre_chars_from_segments += count_to_add;
            }
        } else {
            printf("=> SKIPPED (No letters found in segment)\n");
        }
        
        cleanup_frequency_data(&segment_data); 
        
        // Move the window to the next step
        i = next_i;
    }

    printf("\n--- Segmentation Complete ---\n");
    
    // --- 4. Final Aggregated Report (Uses Segment Proportions) ---
    // Extract frequencies for the entire document for the final Chi-Squared score.
    FrequencyData final_analysis_data = extract_frequencies_from_buffer(file_buffer, file_length);

    // Pass the non-overcounted character totals
    perform_final_analysis(&final_analysis_data, eng_chars_from_segments, fre_chars_from_segments);

    // --- 5. Histogram Reporting ---
    print_all_histograms(&final_analysis_data);

    // --- 6. Cleanup ---
    cleanup_frequency_data(&final_analysis_data);
    free(file_buffer);

    return EXIT_SUCCESS;
}