/* File : example.i */
%module example

%inline %{
extern void   sayhi(char *x, char *ret);
%}
