### Threaded Spell-Checker

## CIS*3110 Assignment 2

## Author
Shaan Saharan (1128702)  
Date: April 2nd, 2024

## Description

This program is a threaded spell-checker. It utilizes threads to independently run spell-checking tasks on text files using a provided dictionary. This approach allows for concurrent spell-checking operations, demonstrating the use of threads and thread synchronization in a C program.

## Features

- Concurrent spell-checking of multiple files.
- Text-based menu for interacting with the program.
- Outputs include the total number of spelling mistakes and the three most frequently occurring misspelled words.
- Results are saved to a file or displayed on the terminal based on user preference.

## Compilation

A Makefile is provided for easy compilation of the source code. To compile the program, use the following command:

- `make all`

This will create an executable named `A2checker`.

## Usage

To run the spell-checker, start the program with the following command:

- `./A2checker`

Optionally, to save the final summary to a file named `saharan_A2.out` instead of displaying it on the terminal, run the program with the `-l` flag:

- `./A2checker -l`

Follow the on-screen menu to start spell-checking tasks or exit the program.

## Cleaning Up

To remove the compiled executable and any object files, use the `clean` target:


## Implementation Details

- The program employs POSIX threads (`pthread`) for concurrent spell-checking.
- A hash table is used to efficiently look up words in the dictionary.
- Thread synchronization is achieved using mutexes to ensure thread-safe access to shared resources.

## Known Issues

- If a word from a file has special characters attached to it, the code treats the entire string as one word.

## Additional Instructions

- Ensure that the dictionary file is accessible and correctly formatted.
- The program has been tested with the standard American English dictionary file.
