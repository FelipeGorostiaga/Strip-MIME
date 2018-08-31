//
// Created by sswinnen on 26/08/18.
//

char ** parseMediaRanges(const char * argument);
int mediaRangeIsValid(char * mediaRange);
void matchMediaTypes(char ** mediaRanges);
void freeResources(char ** strings);
char * readTypeFromStdin();
int mediaTypeCheck(char * mediaType, char ** mediaRanges);
