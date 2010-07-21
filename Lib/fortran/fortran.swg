/* -----------------------------------------------------------------------------
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * fortran.swg
 * ----------------------------------------------------------------------------- */ 

%insert("runtime") %{
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
%}

%typemap(in, replaceparm="$type *$input", noblock=1) int, float, double {
    $1 = *$input;
}
%typemap(in) char "$1 = $input;"
%typemap(in) int *, float *, double * "$1 = $input;"
%typemap(in, extraparm="size_t $1_len", noblock=1) char * {
    $1 = Swig_null_terminate($input, $1_len);
}

// %typemap(argout) int, float, double "*$output = $input;"
%typemap(argout, noblock=1) char * {
    Swig_fortranify($1, $input, $1_len);
    free($1);
}

%insert(runtime) %{
char* Swig_null_terminate(char* inStr, int len) {
    int retVal = 0;
    char* newStr = NULL;
    char* current = NULL;

    if (inStr && (len > 0) ) {

        current = inStr+len-1;

        while ((len > 0) && (isspace(*(current)))) {
            // dont strip off newlines

            if ( (*(current) == '\f')
              || (*(current) == '\n')
              || (*(current) == '\r')
              || (*(current) == '\t')
              || (*(current) == '\v') )
            {
                break;
            }

            if (--len) {
                current--;
            }
        }

        newStr = (char*) calloc(len+1,(sizeof(char)));
        strncpy(newStr,inStr,len);
        *(newStr+len) = '\0';

        retVal++;
    }

    return newStr;
}
%}

%insert(runtime) %{
void Swig_fortranify(const char* inBuff, char* retText, int retTextLen) {

    int inBuffLen = 0;
    int i = 0;

    if (inBuff && retText && (retTextLen > 0)) {
        inBuffLen = strlen(inBuff);

        strncpy(retText, inBuff, retTextLen);

        // fortran-ify the string
        if (inBuffLen < retTextLen) {
            for (i = inBuffLen; i < retTextLen; i++) {
                retText[i] = ' ';
            }
        }
    }
}
%}
