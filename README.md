

# 📘 TEXT-ANALYSER  
### *English–French Language Identification System (C Programming Project)*

A statistical text-analysis system written in **C** that identifies whether a given text is written in **English** or **French**.  
The program performs:

- Unicode-based letter extraction  
- Accented character processing  
- Bigram hashing  
- Sliding-window analysis  
- Chi-square statistical comparison  

It integrates both monograph (single-letter) and bigram frequency analysis to produce a highly accurate prediction.

---

## 🧑‍🏫 Course
**Programming and Problem Solving (CS31101)**  
**Motilal Nehru National Institute of Technology (MNNIT), Allahabad**  
Submitted to: **Prof. M. M. Gore**

---

## 👥 Team Members

| Name | Roll No. | Role |
|------|----------|------|
| Aman Mishra | 2025ca012 | Captain |
| Akhil Kumar Gupta | 2025ca010 | Vice Captain |
| Suraj Kumar Gupta | 2025ca100 | Member |
| Muskaan Kushwaha | 2025ca059 | Member |
| Harsh Rajput | 2025ca039 | Member |
| Harshit Maurya | 2025ca040 | Member |
| Himanshu Ranjan | 2025ca044 | Member |
| Ayush Singh Rajput | 2025ca027 | Member |
| Astitwa Kumar | 2025ca025 | Member |
| Krishna Kumar | 2025ca052 | Member |
| Md Suhail | 2025ca056 | Member |

---

## 📄 Abstract

This project implements a complete **English–French language classifier** using statistical analysis and Unicode text processing.  
It extracts:

- Letter frequencies  
- Accented-character statistics  
- Bigram (two-letter) patterns  
- Word counts  
- Sliding-window segment analysis  

Using chi-square comparison against predefined English and French frequency tables, the system identifies the closest matching language with high accuracy.

---

# 🧩 Features

### ✔ Unicode support (UTF-8 wide-character processing)  
### ✔ Counts 26 English letters + 14 French accented letters  
### ✔ Bigram extraction using 32-bit hashing  
### ✔ Sliding-window segmentation  
### ✔ Histogram generation  
### ✔ Chi-square language testing  
### ✔ Final combined prediction (monograph + bigram)  

---

# 🛠 System Architecture

### **Core Modules**

### 1. Main Flow (`main.c`)
Handles:
- Reading file into buffer  
- UTF-8 → wide-character conversion  
- Segmenting input into windows  
- Extracting frequency data  
- Performing chi-square tests  
- Generating histograms  
- Final language decision  

### 2. Character Frequency Mapping
Tracks:
- 26 English letters  
- 14 French accented characters  
- Maps each to a unique frequency bin  

### Code Snippet:
```c
static inline int map_letter_to_index(wint_t wc) 
{
    wint_t lower = towlower(wc);

    if (lower >= L'a' && lower <= L'z') {
        return lower - L'a';
    }
    for (int i = 0; i < TOTAL_BINS - 26; i++) {
        if (lower == ACCENTED_CHARS[i]) {
            return 26 + i;
        }
    }
    return -1;
}
````

---

### 3. Bigram Counting

Captures 2-character patterns to improve accuracy.

#### Bigram Key Generation:

```c
uint32_t key = ((uint32_t)char1 << 16) | (uint32_t)char2;
unsigned int index = hash_key(key);
```

---

### 4. Chi-Square Algorithm

Performs:

* Monograph chi-square
* Bigram chi-square
* Weighted score combination

---

# 🧪 Results

The program outputs:

* Letter frequency histograms
* Bigram counts
* Monograph chi-square values
* Bigram chi-square values
* Sliding-window classification
* Final predicted language

---

# 🏁 Conclusion

The **TEXT-ANALYSER** system successfully demonstrates statistical language recognition using C.
Unicode processing + frequency extraction + bigram hashing + chi-square scoring = **Accurate English–French classification**.

Its modular structure allows easy expansion to new languages and integration into larger NLP systems.

---

# 📚 References

* Word Frequency Data
* Chi-Square Test
* Sliding Window Technique

---


```

