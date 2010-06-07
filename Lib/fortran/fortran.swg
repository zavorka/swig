/* -----------------------------------------------------------------------------
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * fortran.swg
 * ----------------------------------------------------------------------------- */ 

// WARNING: passing function pointers from C as parameters of type (or as
// return values) SWIGTYPE (CLASS::*) causes cast of C function to type
// void(*)() and it is user's responsibility to properly handle this 
// function's arguments and return value. See 'cpp_basic' test for details.

//%insert("runtime") "clabels.swg"
//%insert("proxy_header") "cproxy.swg"

%insert("runtime") %{
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
%}

%typemap(in) int, float, double "$1 = *$input;"
%typemap(in) char "$1 = $input;"
%typemap(in) int *, float *, double * "$1 = $input;"
%typemap(in) char * "$1 = null_terminate($input, $length);"

// %typemap(argout) int, float, double "*$output = $input;"
%typemap(argout, noblock=1) char * {
    fortranify($input, $output, $length);
    free($input);
}

%insert(runtime) %{
char* null_terminate(char* inStr, int len) {
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
void fortranify(const char* inBuff, char* retText, int retTextLen) {

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
