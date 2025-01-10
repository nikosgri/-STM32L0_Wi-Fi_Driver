/*
 * json_extracter.h
 *
 *  Created on: Nov 9, 2024
 *      Author: Nikolaos Grigoriadis
 *      Email : n.grigoriadis09@gmail.com
 *      Title : Embedded software engineer
 *      Degree: BSc and MSc in computer science, university of Ioannina
 */

#ifndef JSON_EXTRACTER_H_
#define JSON_EXTRACTER_H_


#include <main.h>
#include <jsmn.h>
#include <swo.h>

#define TOKEN_SIZE 256  /**< Defines the maximum number of tokens for the JSON parsing */

/**
 * @struct VariableHolder
 * @brief Structure to hold parsed data either as integers or strings.
 *
 * This structure stores both the integer and string representations of extracted values.
 * It also includes a flag (`is_int`) to indicate whether the value is an integer or a string.
 */
struct VariableHolder {
    int int_val;              /**< Integer value for storing parsed integers. */
    char string_val[256];     /**< String value for storing parsed strings. */
    int is_int;               /**< Flag to indicate whether the value is an integer (1) or a string (0). */
};

typedef struct VariableHolder VariableHolderType;

/**
 * @brief Extracts values from a JSON string for the given keys and stores them in the provided variables.
 *
 * This function uses the jsmn library to parse the JSON string. For each key in the `keys` array,
 * it finds the corresponding value in the JSON string and stores it either as an integer or a string
 * in the `variables` array.
 *
 * @param json_string The input JSON string.
 * @param keys An array of keys to search for in the JSON string.
 * @param key_count The number of keys in the `keys` array.
 * @param variables An array of `VariableHolderType` to store the extracted values.
 *
 * @return 0 on success, -1 if an error occurs.
 */
int extract_json_data(const char *json_string, const char *keys[], int key_count, VariableHolderType *variables)
{
    jsmn_parser parser;       // Parser for the jsmn library
    jsmntok_t tokens[TOKEN_SIZE]; // Array to store parsed tokens
    int token_count;           // The total number of tokens parsed

    /*Initialize jsmn parser*/
    jsmn_init(&parser);

    /*Parse JSON string using jsmn*/
    token_count = jsmn_parse(&parser, json_string, strlen(json_string), tokens, TOKEN_SIZE);
    if (token_count < 0)
    {
        printf("JSON Parsing error\n");
        return -1;  // Return error if parsing fails
    }

    /*Initialize VariableHolderType array to default values*/
    for (int i = 0; i < key_count; i++)
    {
        variables[i].int_val = 0;
        variables[i].string_val[0] = '\0';
        variables[i].is_int = 0;
    }

    /*Parse tokens and extract the corresponding values for each key*/
    for (int i = 0; i < key_count; i++)
    {
        const char *key = keys[i];  // The current key to search for in the JSON string
        int found = 0;  // Flag to indicate whether the key was found in the JSON string

        /*Loop over all tokens to find matching keys*/
        for (int j = 1; j < token_count; j++)
        {
            if (tokens[j].type == JSMN_STRING)
            {
                int key_length = tokens[j].end - tokens[j].start;
                if (strncmp(key, json_string + tokens[j].start, key_length) == 0 && strlen(key) == key_length)
                {
                    /*Key matched, now extract the value*/
                    jsmntok_t *value_token = &tokens[j + 1]; //Pointer to the token containing the value
                    found = 1;

                    /*Handle string value*/
                    if (value_token->type == JSMN_STRING)
                    {
                        int value_length = value_token->end - value_token->start;
                        strncpy(variables[i].string_val, json_string + value_token->start, value_length);
                        /*Null-terminate the string */
                        variables[i].string_val[value_length] = '\0';
                        /*Mark as a string */
                        variables[i].is_int = 0;
                    }
                    /* Handle primitive value (e.g., integer or hexadecimal)*/
                    else if (value_token->type == JSMN_PRIMITIVE)
                    {
                        char temp[256];
                        int value_length = value_token->end - value_token->start;
                        strncpy(temp, json_string + value_token->start, value_length);
                        temp[value_length] = '\0';  // Null-terminate the string

                        /*Check if the value is a hexadecimal number (starts with "0x")*/
                        if (temp[0] == '0' && temp[1] == 'x')
                        {
                        	/*Parse as hexadecimal integer */
                            sscanf(temp, "%x", &variables[i].int_val);
                        }
                        else
                        {
                        	/*Parse as decimal integer */
                            sscanf(temp, "%d", &variables[i].int_val);
                        }
                        /*Mark as an integer */
                        variables[i].is_int = 1;
                    }
                    /*Stop searching once the key is found */
                    break;
                }
            }
        }

        /*If key not found, print a message*/
        if (!found)
        {
            printf("Key %s not found in JSON.\n", key);
        }
    }

    /* Return success */
    return 0;
}

#endif /* JSON_EXTRACTER_H_ */
