//
//  main.c
//  compact-pwd
//
//  Created by Alex Popp on 14.11.15.
//  Copyright © 2015 Alex Popp. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#define MIN(a, b) ((a) < (b) ? a : b)
#define MAX(a, b) ((a) > (b) ? a : b)

#define MAX_FILE_LENGTH 48
#define USER_DIR "/Users/ap"

typedef struct {
    char pBeforeFile[MAX_FILE_LENGTH];
    char pAfterFile[MAX_FILE_LENGTH];
    unsigned long beforeFileLength;
    unsigned long afterFileLength;
} surrFiles;

void surroundingFiles(surrFiles *pSFiles, char *path, char *pPathEnd, char *pFileStart, unsigned long fileLength) {
    DIR *dfd;
    char curPathEndChar = *pPathEnd;
    *pPathEnd = '\0';
    dfd = opendir(path);
    *pPathEnd = curPathEndChar;

    struct dirent *dp;
    if (dfd != NULL) {
        bool foundCurFile = false;
        while ((dp = readdir(dfd)) != NULL) {
            // Copy file after current file
            if (foundCurFile) {
                pSFiles->afterFileLength = MIN(dp->d_namlen, MAX_FILE_LENGTH);
                memcpy(pSFiles->pAfterFile, dp->d_name, pSFiles->afterFileLength);
                break;
            }

            if (fileLength == dp->d_namlen && memcmp(dp->d_name, pFileStart, fileLength) == 0)
                foundCurFile = true;

            // Copy file before current file
            if (!foundCurFile) {
                pSFiles->beforeFileLength = MIN(dp->d_namlen, MAX_FILE_LENGTH);
                memcpy(pSFiles->pBeforeFile, dp->d_name, pSFiles->beforeFileLength);
            }
        }

        closedir(dfd);
    }

    if (pSFiles->beforeFileLength == 0) memset(pSFiles->pBeforeFile, 0, pSFiles->beforeFileLength = MAX_FILE_LENGTH);
    if (pSFiles->afterFileLength == 0) memset(pSFiles->pAfterFile, 0, pSFiles->afterFileLength = MAX_FILE_LENGTH);
}

void surroundingFilesReverse(surrFiles *pSFiles, char *path, char *pPathEnd, char *pFileStart, unsigned long fileLength) {
    DIR *dfd;
    char curPathEndChar = *pPathEnd;
    *pPathEnd = '\0';
    dfd = opendir(path);
    *pPathEnd = curPathEndChar;

    struct dirent *dp;
    if (dfd != NULL) {
        while ((dp = readdir(dfd)) != NULL) {
            int dpCmp     = 0;
            int beforeCmp = 0;
            int afterCmp  = 0;

            int cmpLength = (int)MAX(MAX(dp->d_namlen, fileLength), pSFiles->beforeFileLength);
            for (int i = 0; i < cmpLength; ++i) {
                char dpChar = (i > dp->d_namlen - 1) ? 0x0 : dp->d_name[dp->d_namlen - i - 1];

                if (dpCmp == 0) {
                    char fileChar = (i > fileLength - 1) ? 0x0 : pFileStart[fileLength - i - 1];

                    if (dpChar > fileChar) dpCmp = 1;
                    else if (dpChar < fileChar) dpCmp = -1;
                }

                if (beforeCmp == 0) {
                    char beforeChar = (i > (int)pSFiles->beforeFileLength - 1) ?
                                          0x0 :
                                          pSFiles->pBeforeFile[pSFiles->beforeFileLength - i - 1];

                    if (dpChar > beforeChar) beforeCmp = 1;
                    else if (dpChar < beforeChar) beforeCmp = -1;
                }

                if (afterCmp == 0) {
                    char afterChar = (i > (int)pSFiles->afterFileLength - 1) ?
                                         0x7F :
                                         pSFiles->pAfterFile[pSFiles->afterFileLength - i - 1];

                    if (dpChar > afterChar) afterCmp = 1;
                    else if (dpChar < afterChar) afterCmp = -1;
                }

                if (dpCmp != 0 && beforeCmp != 0 && afterCmp != 0) break;
            }

            if (dpCmp == -1 && beforeCmp >= 0) {
                pSFiles->beforeFileLength = MIN(dp->d_namlen, MAX_FILE_LENGTH);
                memcpy(pSFiles->pBeforeFile, dp->d_name, pSFiles->beforeFileLength);

            } else if (dpCmp == 1 && afterCmp <= 0) {
                pSFiles->afterFileLength = MIN(dp->d_namlen, MAX_FILE_LENGTH);
                memcpy(pSFiles->pAfterFile, dp->d_name, pSFiles->afterFileLength);
            }
        }

        if (pSFiles->beforeFileLength == 0)
            memset(pSFiles->pBeforeFile, 0, pSFiles->beforeFileLength = MAX_FILE_LENGTH);
        if (pSFiles->afterFileLength  == 0)
            memset(pSFiles->pAfterFile,  0, pSFiles->afterFileLength  = MAX_FILE_LENGTH);

        closedir(dfd);
    }
}

int uniqCharCount(surrFiles *pSFiles, char *pFileStart, unsigned long fileLength, bool fromFront) {
    int  cmpLength  = (int)MIN(MAX(pSFiles->beforeFileLength, pSFiles->afterFileLength), fileLength);
    bool beforeDiff = false;
    bool afterDiff  = false;

    int  uniqChars  = 0;
    for (; uniqChars < cmpLength; ++uniqChars) { // <=
        char fileChar = fromFront ? pFileStart[uniqChars] : pFileStart[fileLength - uniqChars - 1];
        char beforeChar;
        char afterChar;

        if (uniqChars < pSFiles->beforeFileLength)
            beforeChar = fromFront ?
                             pSFiles->pBeforeFile[uniqChars] :
                             pSFiles->pBeforeFile[pSFiles->beforeFileLength - uniqChars - 1];
        else beforeChar = 0;

        if (uniqChars < pSFiles->afterFileLength)
            afterChar = fromFront ?
                            pSFiles->pAfterFile[uniqChars] :
                            pSFiles->pAfterFile[pSFiles->afterFileLength - uniqChars - 1];
        else afterChar = 0;

        if (beforeChar != fileChar)  beforeDiff = true;
        if (afterChar  != fileChar)  afterDiff = true;
        if (beforeDiff && afterDiff) break;
    }

    uniqChars = MIN(uniqChars + 1, (int)fileLength);

    return uniqChars;
}

int main(int argc, const char * argv[]) {
    char path[1024];
    if (getcwd(path, sizeof(path)) == NULL) return EXIT_FAILURE;

    bool inUserDir = (memcmp(path, USER_DIR, sizeof(USER_DIR) - 1) == 0);
    if (inUserDir) putchar('~');

    char *pCurPathEnd   = path + (inUserDir ? sizeof(USER_DIR) - 1 : 0);
    char *pCurFileStart = path + (inUserDir ? sizeof(USER_DIR)     : 1);
    char *pCurFileEnd   = path + (inUserDir ? sizeof(USER_DIR)     : 1);
    unsigned long curFileLength;

    do {
        if (*pCurFileEnd == '/' || *(pCurFileEnd) == '\0') {
            pCurPathEnd = pCurFileStart;

            curFileLength = pCurFileEnd - pCurFileStart;

            surrFiles sFiles;
            sFiles.beforeFileLength = 0;
            sFiles.afterFileLength  = 0;
            surroundingFiles(&sFiles, path, pCurPathEnd, pCurFileStart, curFileLength);

            bool printFrontUniqChars = true;

            int frontUniqChars = uniqCharCount(&sFiles, pCurFileStart, curFileLength, true);
            int backUniqChars  = 0;

            // Try if shorter representation possible by counting unique characters from the back
            if ((float)frontUniqChars / (float)curFileLength > .5) {
                sFiles.beforeFileLength = 0;
                sFiles.afterFileLength  = 0;
                surroundingFilesReverse(&sFiles, path, pCurPathEnd, pCurFileStart, curFileLength);

                backUniqChars = uniqCharCount(&sFiles, pCurFileStart, curFileLength, false);
                if (backUniqChars < frontUniqChars - 3) printFrontUniqChars = false;
            }

            putchar('/');
            if (printFrontUniqChars) {
                fwrite(pCurFileStart, frontUniqChars, sizeof(char), stdout);
            } else {
                if (curFileLength >= backUniqChars + 1) putchar(*pCurFileStart);
                if (curFileLength >= backUniqChars + 2) putchar('*');
                fwrite(pCurFileEnd - backUniqChars, backUniqChars, sizeof(char), stdout);
            }

            pCurFileStart = pCurFileEnd + 1;
        }
    } while (*pCurFileEnd++ != '\0');

    putchar('\n');

    return EXIT_SUCCESS;
}
