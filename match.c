//
// Created by sswinnen on 26/08/18.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "match.h"
#define BLOCK 10
#define ADMITTED 1
#define NOT_ADMITTED 0

int main(int argc, char * argv[]) {

    char ** mediaRanges;

    if(argc != 2) {
        printf("Invalid number of arguments!\n");
        return 0;
    }

    mediaRanges = parseMediaRanges(argv[1]);
    matchMediaTypes(mediaRanges);
    freeResources(mediaRanges);
}


/* Checks if the media ranges provided are valid*/
char ** parseMediaRanges(const char * argument) {

    char ** mediaRanges = malloc(BLOCK * sizeof(char*));
    int i, j, k;
    i = j = k = 0;

    while(argument[i] != 0) {
        if(argument[i] != ',') {
            if(j % BLOCK == 0) {
                mediaRanges[k] = realloc(mediaRanges[k],(size_t)(i+BLOCK* sizeof(char)));
            }
            mediaRanges[k][j++] = argument[i];
            mediaRanges[k][j] = 0;
        }
        else {
            if(!mediaRangeIsValid(mediaRanges[k++])) {
                printf("Invalid media range\n");
                freeResources(mediaRanges);
                return NULL;
            }
            j = 0;
        }
        i++;
    }
    if(!mediaRangeIsValid(mediaRanges[k])) {
        printf("Invalid media range\n");
        return NULL;
    }
    mediaRanges[k+1] = 0;
    printf("Media types successfully recognized\n");
    return mediaRanges;
}

int mediaRangeIsValid(char * mediaRange) {
    char * type;
    char * subtype;
    size_t mrLength;

    mrLength = strlen(mediaRange);
    type = malloc((mrLength+1)*sizeof(char));
    subtype = malloc((mrLength+1)*sizeof(char));
    sscanf(mediaRange,"%[^ /]/%s",type,subtype);
    if(strcmp(type,"application") == 0 || strcmp(type,"audio") == 0
       || strcmp(type,"example") == 0 || strcmp(type,"font") == 0
       || strcmp(type,"image") == 0 || strcmp(type,"message") == 0
       || strcmp(type,"model") == 0 || strcmp(type,"multipart") == 0
       || strcmp(type,"text") == 0 || strcmp(type,"video") == 0) {
        if(strlen(subtype) > 0) {
            return ADMITTED;
        }
    }
    printf("TYPE: %s\n",type);
    printf("SUBTYPE: %s\n",subtype);
    free(type);
    free(subtype);
    printf("Invalid media type range for media range: %s\n", mediaRange);
    return NOT_ADMITTED;
}

void matchMediaTypes(char ** mediaRanges) {

    char ** mediaTypes;
    int i = 0;
    int result;

    mediaTypes = malloc(BLOCK* sizeof(char *));
    while((mediaTypes[i] = readTypeFromStdin()) != NULL) {
        result = mediaTypeCheck(mediaTypes[i], mediaRanges);
        if(result == ADMITTED) {
            printf("true\n");
        }
        else if(result == NOT_ADMITTED) {
            printf("false\n");
        } else {
            printf("null \n");
        }

        if(i % BLOCK == 0) {
            mediaTypes = realloc(mediaTypes, (i + BLOCK) * sizeof(char *));
        }
    }
}

/* returns 1 if media type is admitted, 0 if not and -1 if format is not valid */
int mediaTypeCheck(char * mediaType, char ** mediaRanges) {

    char * type;
    char * subtype;
    char * rangeType;
    char * rangeSubtype;
    size_t mrLength;
    int i = 0;
    int admitted = NOT_ADMITTED;

    mrLength = strlen(mediaType);
    type = malloc((mrLength+1)*sizeof(char));
    subtype = malloc((mrLength+1)*sizeof(char));
    sscanf(mediaType,"%[^ /]/%s",type,subtype);
    rangeType = malloc((mrLength+1)*sizeof(char));
    rangeSubtype = malloc((mrLength+1)*sizeof(char));
    while(mediaRanges[i] != NULL && !admitted) {
        sscanf(mediaRanges[i],"%[^ /]/%s",rangeType, rangeSubtype);
        if(strcmp(type,rangeType) == 0 || strcmp(rangeType,"*") == 0) {
            if(strcmp(subtype,rangeSubtype) == 0 || strcmp(rangeSubtype,"*") == 0) {
                admitted = ADMITTED;
            }
        }

        i++;
    }
    free(type);
    free(subtype);
    free(rangeType);
    free(rangeSubtype);
    return admitted;

}

/* Frees previously allocated resources for accepted media ranges */
void freeResources(char ** strings) {

    int i = 0;

    while(strings[i] != 0) {
        free(strings[i++]);
    }
    free(strings);
}

char * readTypeFromStdin() {
    char * mediaType;
    int c, i;
    i = 0;

    mediaType = malloc(BLOCK* sizeof(char));
    while(((c = getchar()) != EOF) && (c != '\n')) {
        mediaType[i++] = (char)c;
        mediaType[i] = 0;
    }

    if(c == EOF) {
        return NULL;
    }

    return mediaType;
}