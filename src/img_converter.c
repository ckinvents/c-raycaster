// Quick solution for converting Piskel output into something useable in engine
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define BUFF_SIZE 6000

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        puts("Error: Not enough arguments. Please enter an input file name and output file name! Exiting...");
        return 1;
    }
    FILE* imgFile = fopen(argv[1], "r");
    if (!imgFile)
    {
        puts("Error: File not found! Exiting...");
        return 1;
    }
    FILE* convFile = fopen(argv[2], "w+");
    char lineBuffer[BUFF_SIZE];
    char colBuffer[8];
    uint32_t charCount;
    uint32_t lineCount;
    int currentChar;
    fgets(lineBuffer, BUFF_SIZE, imgFile);
    // While not at data...
    while (feof(imgFile) == 0 && lineBuffer[1] != 'x')
    {
        fputs(lineBuffer, convFile);
        memset(lineBuffer, 0, sizeof(lineBuffer));
        fgets(lineBuffer, BUFF_SIZE, imgFile);
    }
    // While reading data...
    while (feof(imgFile) == 0 && lineBuffer[0] != '}')
    {
        // reset character pointer & pointer value
        charCount = 0;
        currentChar = lineBuffer[charCount];
        while (charCount < BUFF_SIZE && currentChar != '\n')
        {
            currentChar = lineBuffer[charCount];
            if (currentChar == 'x')
            {
                charCount++;
                for (int i = 0; i < 4; i++)
                {
                    colBuffer[7 - (2 * i + 1)] = lineBuffer[charCount + (2 * i)];
                }
                for (int i = 0; i < 4; i++)
                {
                    colBuffer[7 - (2 * i)] = lineBuffer[charCount + (2 * i + 1)];
                }
                for (int i = 0; i < 8; i++)
                {
                    lineBuffer[charCount + i] = colBuffer[i];
                }
                charCount += 7;
            }
            else
            {
                charCount++;
            }
        }
        fputs(lineBuffer, convFile);
        memset(lineBuffer, 0, sizeof(lineBuffer));
        fgets(lineBuffer, BUFF_SIZE, imgFile);
        lineCount++;
        //printf("Line %d",lineCount);
    }
    // For rest of file
    while (feof(imgFile) == 0)
    {
        fputs(lineBuffer, convFile);
        memset(lineBuffer, 0, sizeof(lineBuffer));
        fgets(lineBuffer, BUFF_SIZE, imgFile);
    }
    fclose(imgFile);
    fclose(convFile);
}
