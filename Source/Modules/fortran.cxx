/* -----------------------------------------------------------------------------
 * This file is part of SWIG, which is licensed as a whole under version 3-
 * (or any later version) of the GNU General Public License. Some additional
 * terms also apply to certain portions of SWIG. The full details of the SWIG
 * license and copyrights can be found in the LICENSE and COPYRIGHT files
 * included with the SWIG source code as distributed by the SWIG developers
 * and at http://www.swig.org/legal.html.
 *
 * fortran.cxx
 *
 * Fortran language module for SWIG.
 * ----------------------------------------------------------------------------- */

char cvsroot_fortran_cxx[] = "$Id:$";


#include "swigmod.h"

class FORTRAN:public Language {
protected:
  static const char *usage;

  /* General DOH objects used for holding the strings */
  File *f_begin;
  File *f_runtime;
  File *f_header;
  File *f_wrappers;
  File *f_proxyfxns;
  File *f_init;


public:

   virtual void main(int argc, char *argv[]);

  virtual int top(Node *n);
  virtual int functionWrapper(Node *n);
//     virtual int variableWrapper(Node *n);

};

void FORTRAN::main(int argc, char *argv[]) {
  printf("I'm the Fortran module.\n");
  /* parse command line options
   */
  for (int i = 1; i < argc; i++) {
    if (argv[i]) {
      if (strcmp(argv[i], "-help") == 0) {
        fputs(usage, stderr);
      }
    }
  }

  /* Set language-specific subdirectory in SWIG library */
  SWIG_library_directory("fortran");

  /* Set language-specific preprocessing symbol */
  Preprocessor_define("SWIGFORTRAN 1", 0);

  /* Set language-specific configuration file */
  SWIG_config_file("fortran.swg");

  /* Set typemap language (historical) */
  SWIG_typemap_lang("fortran");

}

int FORTRAN::top(Node *n) {

  printf("Generating code.\n");

  /* Get the module name */
  // String *module = Getattr(n,"name");

  /* Get the output file name */
  String *outfile = Getattr(n, "outfile");

  /* Initialize I/O */
  f_begin = NewFile(outfile, "w", SWIG_output_files());
  if (!f_begin) {
    FileErrorDisplay(outfile);
    SWIG_exit(EXIT_FAILURE);
  }
  f_runtime = NewString("");
  f_init = NewString("");
  f_header = NewString("");
  f_wrappers = NewString("");
  f_proxyfxns = NewString("");

  /* Register file targets with the SWIG file handler */
  Swig_register_filebyname("begin", f_begin);
  Swig_register_filebyname("header", f_header);
  Swig_register_filebyname("wrapper", f_wrappers);
  Swig_register_filebyname("proxyfxns", f_proxyfxns);
  Swig_register_filebyname("runtime", f_runtime);
  Swig_register_filebyname("init", f_init);

  /* Output module initialization code */
  Swig_banner(f_begin);

  /* Emit code for children */
  Language::top(n);

  /* Write all to the file */
  Dump(f_runtime, f_begin);
  Dump(f_header, f_begin);
  Dump(f_wrappers, f_begin);
  Dump(f_proxyfxns, f_begin);
  Wrapper_pretty_print(f_init, f_begin);

  /* Cleanup files */
  Delete(f_runtime);
  Delete(f_header);
  Delete(f_wrappers);
  Delete(f_proxyfxns);
  Delete(f_init);
  Close(f_begin);
  Delete(f_begin);

  return SWIG_OK;
}

int FORTRAN::functionWrapper(Node *n) {
  Printf(stdout, "creating function wrapper\n");
  String *symname = Getattr(n, "sym:name");
  String *type = Getattr(n, "type");

  Wrapper *f = NewWrapper();
  Wrapper *fproxy = NewWrapper();

  ParmList *parms = Getattr(n, "parms");

  // create new wrapper name
  String *wname = Swig_name_wrapper(symname);
  Setattr(n, "wrap:name", wname);

  // create the function definition
  String *return_type = SwigType_str(type, 0);

  /* Attach standard typemaps */
  emit_attach_parmmaps(parms, f);
  Setattr(n, "wrap:parms", parms);

  /* Generate prototype and parameter strings
     with extra parameters attached. extra parameters
     are not sent through the typemap system. */
  String *parmStr = emit_parm_str(parms);
  String *argsStr = emit_args_str(parms);

  Printv(f->def, return_type, " ", wname, "(", parmStr, ") {\n", NIL);

  // create alternative call functions (proxyfxns)
  // create proxy function with single underscore
  Printv(fproxy->def, return_type, " ", symname, "_", "(", parmStr, ") {\n", NIL);

  bool is_void_return = (SwigType_type(type) == T_VOID);
  if (!is_void_return) {
    Printf(fproxy->code, "return ");
  }

  Printv(fproxy->code, wname, "(", argsStr, ")", ";\n}", NIL);
  Wrapper_print(fproxy, f_proxyfxns);

  // create proxy function with double underscore
  // create proxy function in all caps

  // Emit all of the local variables for holding arguments.
  emit_parameter_variables(parms, f);


  // Emit variable holding return value.
  emit_return_variable(n, return_type, f);

  String *tm;
  Parm *p;

  /* Insert input typemap code */
  String *inarg = NewString("");
  p = parms;
  while (p) {
    if ((tm = Getattr(p, "tmap:in"))) {
      Replaceall(tm, "$1", Getattr(p, "lname"));
      Replaceall(tm, "$input", Getattr(p, "name"));
      Printv(inarg, tm, "\n", NIL);
      p = Getattr(p, "tmap:in:next");
    } else {
      p = nextSibling(p);
    }
  }

  /* Insert argument output code */
  String *outarg = NewString("");
  p = parms;
  while (p) {
    if ((tm = Getattr(p, "tmap:argout"))) {
      Replaceall(tm, "$1", Getattr(p, "lname"));
      Replaceall(tm, "$input", Getattr(p, "name"));
      Printv(outarg, tm, "\n", NIL);
      p = Getattr(p, "tmap:argout:next");
    } else {
      p = nextSibling(p);
    }
  }

  // attach local variables to parameters

  // print input typemap conversions to wrapper.
  Printv(f->code, inarg, "\n", NIL);
  Delete(inarg);

  if (!is_void_return) {
    Printf(f->code, "result = ");
  }

  // create function call
  // get function definition arguments
  String *empty_string = NewString("");
  String *arg_names = Swig_cfunction_call(empty_string, parms);
  Printv(f->code, symname, arg_names, ";\n", NIL);
  Delete(empty_string);
  Delete(arg_names);

  // attach output arguments
  Printv(f->code, "\n", outarg, "\n", NIL);
  Delete(outarg);

  if (!is_void_return) {
    Printf(f->code, "return result;\n");
  }
  Printf(f->code, "}");
  // write out the wrapper file
  Wrapper_print(f, f_wrappers);

#if 0
  Delete(symname);
  Delete(type);
  Delete(wname);
  Delete(return_type);
  Delete(parmStr);
  Delete(argsStr);
#endif

  return SWIG_OK;
}

extern "C" Language *swig_fortran(void) {
  return new FORTRAN();
}

/* -----------------------------------------------------------------------------
 * Static member variables
 * ----------------------------------------------------------------------------- */

const char *FORTRAN::usage = (char *) "\
\n";
