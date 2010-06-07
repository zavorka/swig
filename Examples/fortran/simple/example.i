/* File : example.i */
%module example

%inline %{
extern int    gcd(int x, int y);
extern void   sayhi(char *x, int y, char *ret);
//extern double Foo;
%}
