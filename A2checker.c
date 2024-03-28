/****************************
Shaan Saharan (1128702)
CIS*3110 Assignment 2
April 2nd, 2024
*****************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <limits.h>

// Define constants for buffer sizes and hash table size for dictionary
#define FILE_LENGTH 256
#define WORDS_IN_DICT 150000
#define DICTIONARY_HASH_SIZE 150000 // Hash table size for dictionary entries

// Global variables for tracking program state
int numFilesProcessed = 0; // Count of files processed
int numSpellingErrors = 0; // Count of spelling errors found across all files
bool sendToFile = false; // Flag to determine if output should be logged to a file
pthread_mutex_t mutexInputFile; // Mutex for synchronizing access to input files
pthread_mutex_t mutexOutputFile; // Mutex for synchronizing access to output files

// Structure to hold details of misspelled words
typedef struct {
    int numWords;
    char wordFound[50];
} NumWords;

NumWords allMisspelledWords[3] = {0}; // Array to track the top 3 misspelled words

// Structure to store results of spell checking a file
typedef struct {
    int totalMisspelledWords; // Total number of misspelled words in the file
    char topMisspelledWords[3][50]; // Top 3 misspelled words
    int misspelledWordsCount[3]; // Count of occurrences for each top misspelled word
} SpellCheckTask;

// Structure to pass input and dictionary file paths to threadCreated
typedef struct {
    char inFile[FILE_LENGTH];
    char dictFile[FILE_LENGTH];
} TaskCheck;

// Node structure for the hash table (dictionary)
typedef struct dict_entry {
    char wordFound[50]; // Word stored in the dictionary
    struct dict_entry *next; // Pointer to the next entry (for handling collisions)
} DictEntry;

DictEntry *dictionaryHash[DICTIONARY_HASH_SIZE]; // Hash table for the dictionary

// Declarations of functions
void *spellCheckerFuntion(void *arg);
void textToFile(const char *inFile, SpellCheckTask *outputResult);
void logToFile(const char *inFile, SpellCheckTask *outputResult);
void updateGlobalSummary(SpellCheckTask *outputResult);
void updateCount(SpellCheckTask *outputResult, const char *wordFound);
void updateMisspelling(const char *misspelledWord, int count);
bool checkDictionary(const char *dictFile);
bool findWord(const char *wordFound);
unsigned long hash_word(const char *wordFound);
void insert_word_into_hash(const char *wordFound);
void free_dictionary_hash();

// Log the final summary to either a file or standard output
void logSummary(FILE* output) {
    fprintf(output, "\n****************** Final Summary ******************\n");
    fprintf(output, "Number of files processed: %d\n", numFilesProcessed);
    fprintf(output, "Number of spelling errors: %d\n", numSpellingErrors);
    fprintf(output, "Three most common misspellings:\n");
    for (int i = 0; i < 3; i++) {
        if (allMisspelledWords[i].numWords > 0) {
            fprintf(output, "%s (%d times) ", allMisspelledWords[i].wordFound, allMisspelledWords[i].numWords);
        }
    }
    fprintf(output, "\n***************************************************\n");
}

// Hash function to generate an index for a given word
unsigned long hash_word(const char *wordFound) {
    unsigned long hash = 5381;
    int c;
    while ((c = *wordFound++))
        hash = ((hash << 5) + hash) + c;
    return hash % DICTIONARY_HASH_SIZE;
}

// Insert a word into the hash table (dictionary)
void insert_word_into_hash(const char *wordFound) {
    DictEntry *newEntry = (DictEntry *)malloc(sizeof(DictEntry));
    strcpy(newEntry->wordFound, wordFound);
    unsigned long index = hash_word(wordFound);
    newEntry->next = dictionaryHash[index];
    dictionaryHash[index] = newEntry;
}

// Attempts to open and load a dictionary file into a hash table for efficient lookups
bool checkDictionary(const char *dictFile) {
    // Clear existing hash table if reloading
    free_dictionary_hash();
    memset(dictionaryHash, 0, sizeof(dictionaryHash));

    FILE *file = fopen(dictFile, "r");
    if (!file) return false; // If file cannot be opened, return false
    char wordFound[50];
    while (fgets(wordFound, 50, file)) {
        wordFound[strcspn(wordFound, "\n")] = 0; // Remove newline character from the end of the word
        insert_word_into_hash(wordFound); // Insert the word into the hash table
    }
    fclose(file);
    return true; // Return true if the dictionary was successfully loaded
}

// Checks if a word is present in the dictionary hash table
bool findWord(const char *wordFound) {
    unsigned long index = hash_word(wordFound); // Calculate hash index of the word
    for (DictEntry *entry = dictionaryHash[index]; entry != NULL; entry = entry->next) {
        if (strcmp(entry->wordFound, wordFound) == 0) return true; // If word is found, return true
    }
    return false; // Word not found, return false
}

// Frees memory allocated for the dictionary hash table
void free_dictionary_hash() {
    for (int i = 0; i < DICTIONARY_HASH_SIZE; i++) {
        DictEntry *entry = dictionaryHash[i];
        while (entry != NULL) {
            DictEntry *temp = entry; // Temp pointer to current entry
            entry = entry->next; // Move to next entry
            free(temp); // Free current entry
        }
    }
}

// The main function executed by each thread to perform spell checking on a given file
void *spellCheckerFuntion(void *arg) {
    TaskCheck *taskCheck = (TaskCheck *)arg; // Cast argument to TaskCheck structure
    SpellCheckTask outputResult = {0}; // Initialize output result structure

    textToFile(taskCheck->inFile, &outputResult); // Process the input file and update outputResult

    logToFile(taskCheck->inFile, &outputResult); // Log the results of spell checking

    updateGlobalSummary(&outputResult); // Update the global summary with the results

    free(taskCheck); // Free the dynamically allocated TaskCheck structure
    pthread_exit(NULL); // Terminate the thread
}

// Processes a text file, identifying misspelled words based on the loaded dictionary
void textToFile(const char *inFile, SpellCheckTask *outputResult) {
    FILE *file = fopen(inFile, "r");
    if (!file) {
        printf("Unable to find file: %s\n\n", inFile);
        numFilesProcessed--;
        return;
    }
    char wordFound[50];
    while (fscanf(file, "%49s", wordFound) == 1) {
        if (!findWord(wordFound)) { // If word is not found in the dictionary
            outputResult->totalMisspelledWords++; // Increment count of misspelled words
            updateCount(outputResult, wordFound); // Update count for this specific misspelled word
        }
    }
    fclose(file); // Close the file
}

// Logs the results of spell checking to a file or standard output
void logToFile(const char* inFile, SpellCheckTask* outputResult) {
    pthread_mutex_lock(&mutexInputFile); // Lock the mutex to ensure thread-safe access to the output file
    FILE *output = fopen("saharan_A2.out", "a");
    if (output) {
        fprintf(output, "%s %d", inFile, outputResult->totalMisspelledWords); // Log file name and count of misspelled words
        for (int i = 0; i < 3 && outputResult->misspelledWordsCount[i] > 0; i++) { // Log up to the top 3 misspelled words
            fprintf(output, " %s", outputResult->topMisspelledWords[i]);
        }
        fprintf(output, "\n");
        fclose(output); // Close the output file
    } else {
        printf("Unable to send to output file\n\n");
    }
    pthread_mutex_unlock(&mutexInputFile); // Unlock the mutex
}
// Updates the global summary with results from a spell checking task
void updateGlobalSummary(SpellCheckTask* outputResult) {
    pthread_mutex_lock(&mutexOutputFile); // Locks the mutex to ensure thread-safe access to global variables
    numFilesProcessed++; // Increment the count of files processed
    numSpellingErrors += outputResult->totalMisspelledWords; // Add to the global count of spelling errors
    // Iterate through the top misspelled words in the output result
    for (int i = 0; i < 3; i++) {
        // Update the global list of misspellings with the current result
        updateMisspelling(outputResult->topMisspelledWords[i], outputResult->misspelledWordsCount[i]);
    }
    pthread_mutex_unlock(&mutexOutputFile); // Unlocks the mutex
}

// Updates the count of misspelled words in the spell check task
void updateCount(SpellCheckTask *outputResult, const char *wordFound) {
    int index = -1, min = 0; // Index for found word and min for least frequent word
    int isNewWord = 1, countCompare = INT_MAX; // Flags for new word detection and comparison

    // Loop through the top misspelled words to find if the word is already listed or to find the least common word
    for (int i = 0; i < 3; i++) {
        if (strcmp(outputResult->topMisspelledWords[i], wordFound) == 0) {
            index = i; // Word is found
            isNewWord = 0; // Mark as not a new word
            break; // Stop searching
        }
        if (outputResult->misspelledWordsCount[i] < countCompare) {
            countCompare = outputResult->misspelledWordsCount[i]; // Update the lowest count found
            min = i; // Update index of the least common word
        }
    }

    // If the word is already in the list, increment its count
    if (!isNewWord) {
        outputResult->misspelledWordsCount[index]++;
    } else if (countCompare == 0 || outputResult->misspelledWordsCount[min] == countCompare) {
        // If it's a new word and there's a slot with zero count, or it ties for least common, add it
        strcpy(outputResult->topMisspelledWords[min], wordFound);
        outputResult->misspelledWordsCount[min] = 1;
    }

    // If there's a new word or a count has been incremented, resort the list
    if (isNewWord || index != -1) {
        // Perform a simple sort to keep the list ordered by word count
        for (int i = 0; i < 3 - 1; i++) {
            int max = i;
            for (int j = i + 1; j < 3; j++) {
                if (outputResult->misspelledWordsCount[j] > outputResult->misspelledWordsCount[max]) {
                    max = j;
                }
            }
            if (max != i) {
                // Swap the words and their counts to maintain order
                int tempCount = outputResult->misspelledWordsCount[i];
                outputResult->misspelledWordsCount[i] = outputResult->misspelledWordsCount[max];
                outputResult->misspelledWordsCount[max] = tempCount;

                char tempWord[50];
                strcpy(tempWord, outputResult->topMisspelledWords[i]);
                strcpy(outputResult->topMisspelledWords[i], outputResult->topMisspelledWords[max]);
                strcpy(outputResult->topMisspelledWords[max], tempWord);
            }
        }
    }
}

// Updates the global list of misspellings based on new results from a spell check operation
void updateMisspelling(const char *misspelledWord, int count) {
    // Early exit if count is zero, indicating no occurrences of this misspelling in the current context
    if (count == 0) return;

    int min = 0, targetAquired = 0; // Initialize variables for tracking the index of least frequent word and if the target word is found
    int countCompare = INT_MAX; // Set to maximum possible value to ensure any real count is lower

    // Loop through the global list of top misspelled words
    for (int i = 0; i < 3; i++) {
        // Check if the current misspelled word matches the word in the list
        if (strcmp(allMisspelledWords[i].wordFound, misspelledWord) == 0) {
            allMisspelledWords[i].numWords += count; 
            targetAquired = 1; // Mark that we have found the target word in the global list
            break; 
        }
        // If the current word in the list has a count lower than our current comparison point
        if (allMisspelledWords[i].numWords < countCompare) {
            countCompare = allMisspelledWords[i].numWords; // Update this comparison point to the new lowest count
            min = i; // Remember the index of this least frequently occurring word
        }
    }

    // If the target word was not found in the list and its count is higher than the least frequently occurring word,
    // or if that least frequent word has a count of zero, replace that word with the new misspelling
    if (!targetAquired && (count > countCompare || allMisspelledWords[min].numWords == 0)) {
        strcpy(allMisspelledWords[min].wordFound, misspelledWord); // Copy the new word into the position of the least frequent word
        allMisspelledWords[min].numWords = count; // Update the count to that of the new word
    }

    // Perform an insertion sort on the updated list to ensure it remains sorted by frequency of misspellings
    for (int i = 1; i < 3; i++) {
        NumWords key = allMisspelledWords[i]; // Temporarily store the word to be possibly moved
        int j = i - 1;

        // Move each element one position ahead of its current position if it has fewer misspellings than the key word
        while (j >= 0 && allMisspelledWords[j].numWords < key.numWords) {
            allMisspelledWords[j + 1] = allMisspelledWords[j]; 
            j--; 
        }
        allMisspelledWords[j + 1] = key; 
    }
}

//*********************************MAIN*********************************
// Main function where the program execution begins
int main(int argc, char **argv) {
    // Initialize mutexes for input and output file access synchronization
    pthread_mutex_init(&mutexInputFile, NULL);
    pthread_mutex_init(&mutexOutputFile, NULL);
    pthread_t threadCreated[100]; // Array to store thread identifiers

    // Variables to store dictionary and text file paths
    char dictFile[FILE_LENGTH];
    char textFile[FILE_LENGTH];
    int userInput = 0; // Variable to capture user input
    int activeThread = 0; // Counter for active threadCreated

    // Check command-line arguments to see if output should be logged to a file
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            sendToFile = true; // Set flag to log output to file if "-l" option is found
            break;
        }
    }

    // Main loop for user interaction
    while (1) {
        // Display the main menu and prompt for user input
        printf("******************** Main Menu ********************\n");
        printf("1. Start a new spellchecking task\n");
        printf("2. Exit\n");
        printf("***************************************************\n");
        printf("Choose an operation to execute: ");
        scanf("%d", &userInput);

        // Handle user's choice
        if (userInput == 1) {
            // Prompt for and read dictionary file path
            printf("Enter dictionary file to read from (enter 0 to exit back to main menu): ");
            scanf("%s", dictFile);
            if (strcmp(dictFile, "0") == 0) {
                continue; // Exit to main menu if user enters "0"
            }

            // Prompt for and read text file path to spell check
            printf("Enter text file to read from (enter 0 to exit back to main menu): ");
            scanf("%s", textFile);
            if (strcmp(textFile, "0") == 0) {
                continue; // Exit to main menu if user enters "0"
            }
            printf("\n");

            // Load dictionary; if unsuccessful, notify the user and continue
            if (!checkDictionary(dictFile)) {
                printf("Failed to load dictionary\n\n");
                continue;
            }

            // Allocate memory for a TaskCheck structure to pass to the new thread
            TaskCheck *taskCheck = (TaskCheck *)malloc(sizeof(TaskCheck));
            if (taskCheck == NULL) {
                fprintf(stderr, "Failed to allocate memory for TaskCheck\n\n");
                exit(EXIT_FAILURE); // Terminate if memory allocation fails
            } else {
                // Initialize the TaskCheck structure with file paths
                strcpy(taskCheck->dictFile, dictFile);
                strcpy(taskCheck->inFile, textFile);

                // Create a new thread to perform the spell checking
                if (pthread_create(&threadCreated[activeThread], NULL, spellCheckerFuntion, (void *)taskCheck) != 0) {
                    fprintf(stderr, "Failed to create a new thread\n\n");
                    free(taskCheck); // Free the allocated memory on failure
                } else {
                    activeThread++; // Increment the active thread counter on success
                }
            }
        } else if (userInput == 2) {
            // Wait for all active threadCreated to complete
            for (int i = 0; i < activeThread; i++) {
                pthread_join(threadCreated[i], NULL);
            }

            // Lock the output mutex and log the final summary
            pthread_mutex_lock(&mutexOutputFile);
            if (sendToFile) {
                FILE *output = fopen("saharan_A2.out", "a");
                if (output != NULL) {
                    logSummary(output); // Log summary to file
                    fclose(output);
                } else {
                    printf("Unable to send to output file\n\n");
                }
            } else {
                logSummary(stdout); // Log summary to standard output
            }
            pthread_mutex_unlock(&mutexOutputFile);

            // Clean up and terminate the program
            pthread_mutex_destroy(&mutexInputFile);
            pthread_mutex_destroy(&mutexOutputFile);
            exit(0);
        } else {
            // Clear input buffer to prevent infinite loop on invalid input
            while (fgetc(stdin) != '\n' && !feof(stdin)) {}
            printf("Invalid operation. Please reread the options and choose again\n\n");
        }
    }
    // End of program
    return 0;
}
