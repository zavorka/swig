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

Parm *getFirstParm(ParmList *p) {
  Parm *r = p;
  while (p) {
    p = previousSibling(p);
    if (p) {
      r = p;
    }
  }
  return r;
}

Parm *getLastParm(ParmList *p) {
  Parm *r = p;
  while (p) {
    p = nextSibling(p);
    if (p) {
      r = p;
    }
  }
  return r;
}

String *parmListAsString(ParmList *parm) {
  String *ret = NewString("");
  Parm *p = parm;
  while (p) {
    String *pstr = Getattr(p, "name");
    Append(ret, pstr);
    Delete(pstr);
    p = nextSibling(p);
    if (p) {
      Append(ret, ", ");
    }
  }
  return ret;
}

void protolenlisttostr(String *protolenstr, List *wargslenList) {
  int l = Len(wargslenList);
  int i = 0;
  if ((protolenstr == NULL) || (wargslenList == NULL)) {
    return;
  }
  while (i < l) {
    String *s = (String *) Getitem(wargslenList, i);
    Append(protolenstr, "int ");
    Append(protolenstr, s);
    i++;
    if (i < l) {
      Append(protolenstr, ", ");
    }
  }
}

void argslenlisttostr(String *protolenstr, List *wargslenList) {
  int l = Len(wargslenList);
  int i = 0;
  if ((protolenstr == NULL) || (wargslenList == NULL)) {
    return;
  }
  while (i < l) {
    String *s = (String *) Getitem(wargslenList, i);
    Append(protolenstr, s);
    i++;
    if (i < l) {
      Append(protolenstr, ", ");
    }
  }
}

int FORTRAN::functionWrapper(Node *n) {
  Printf(stdout, "creating function wrapper\n");
  String *name = Getattr(n, "sym:name");
  String *type = Getattr(n, "type");
  ParmList *parms = Getattr(n, "parms");

  Wrapper *f = NewWrapper();
  Wrapper *fproxy = NewWrapper();

  // create new wrapper name
  String *wname = Swig_name_wrapper(name);
  Setattr(n, "wrap:name", wname);

  // create the function definition
  String *return_type = SwigType_str(type, 0);
  String *wproto = NewString("");
//    String *wargs = NewString("");
//    String *protoLen = NewString("");
//    String *protoLens = NewString("");
  ParmList *wparms = CopyParmList(parms);
  Parm *p = wparms;
//    List *wprotoList = NewList();
  List *wargslenList = NewList();

  while (p) {
    String *ptype = Getattr(p, "type");
    String *ptypeResolved = SwigType_typedef_resolve_all(ptype);
    if (Strcmp(SwigType_str(ptypeResolved, 0), "char *") == 0) {
      // if argument resolves to a character array,
      // we need an additional parameter to describe
      // the size of the array
      String *npName = NewString("");
      Printv(npName, Getattr(p, "name"), "_len", NIL);
      Insert(wargslenList, DOH_END, npName);
      Delete(npName);
    } else if ((Strcmp(SwigType_str(ptypeResolved, 0), "int") == 0) ||
               (Strcmp(SwigType_str(ptypeResolved, 0), "short") == 0) ||
               (Strcmp(SwigType_str(ptypeResolved, 0), "float") == 0) ||
               (Strcmp(SwigType_str(ptypeResolved, 0), "double") == 0) ||
               (Strcmp(SwigType_str(ptypeResolved, 0), "long") == 0)) {
      // fortran sends the address of integers
      SwigType_add_pointer(ptype);
      Setattr(p, "type", ptype);
    }
    String *pstr = SwigType_str(ptype, Getattr(p, "name"));
    Append(wproto, pstr);
    p = nextSibling(p);
    if (p) {
      Append(wproto, ", ");
    }
    Delete(pstr);
    Delete(ptype);
    Delete(ptypeResolved);
  }
  if (Len(wargslenList) > 0) {
    String *s = NewString("");
    protolenlisttostr(s, wargslenList);
    Append(wproto, ", ");
    Append(wproto, s);
    Delete(s);
  }
  // argslenlisttostr(argslenstr, wargslenList);

  Printv(f->def, return_type, " ", wname, "(", wproto, ") {\n", NIL);

  // create alternative call functions (proxyfxns)
  // create proxy function with single underscore
  // p = getFirstParm(lastParm);
  Printv(fproxy->def, return_type, " ", name, "_", "(", wproto, ") {\n", NIL);
  bool is_void_return = (SwigType_type(type) == T_VOID);
  if (!is_void_return) {
    Printf(fproxy->code, "return ");
  }
  // String *wrapFxnCall = Swig_cfunction_call(wname, wparms);
  // Printv(fproxy->code, wrapFxnCall, ";\n}", NIL);
  String *wargs = parmListAsString(wparms);
  if (Len(wargslenList) > 0) {
    String *s = NewString("");
    argslenlisttostr(s, wargslenList);
    Append(wargs, ", ");
    Append(wargs, s);
    Delete(s);
  }
  Printv(fproxy->code, wname, "(", wargs, ")", ";\n}", NIL);
  Delete(wargs);
  Wrapper_print(fproxy, f_proxyfxns);

  // create proxy function with double underscore
  // create proxy function in all caps

  // Emit all of the local variables for holding arguments.
  emit_parameter_variables(parms, f);

  // Emit variable holding return value.
  emit_return_variable(n, return_type, f);


  /* Attach standard typemaps */
  emit_attach_parmmaps(parms, f);
  Setattr(n, "wrap:parms", parms);

  /* Get number of require and total arguments */
  int num_arguments = emit_num_arguments(parms);

  /* Unmarshal parameters */
  String *source;
  String *source_len;
  String *tm;
  String *incode = NewString("");
  // ParmList *p2 = NULL;
  Parm *p2;
  int i = 0;

  /* Insert input typemap code */
  for (i = 0, p2 = parms; i < num_arguments; i++) {
    /* Skip ignored arguments */

    while (checkAttribute(p2, "tmap:in:numinputs", "0")) {
      p2 = Getattr(p2, "tmap:in:next");
    }

    SwigType *pt = Getattr(p2, "type");
    String *ln = Getattr(p2, "lname");


    if ((tm = Getattr(p2, "tmap:in"))) {
      String *parse = Getattr(p2, "tmap:in:parse");
      if (!parse) {
        /* Produce string representations of the source and target arguments */
        source = Getattr(p2, "name");
        source_len = NewString("");
        Printv(source_len, Getattr(p2, "name"), "_len", NIL);

        Replaceall(tm, "$target", ln);
        Replaceall(tm, "$source", source);
        Replaceall(tm, "$input", source);
        Replaceall(tm, "$length", source_len);
        Setattr(p2, "emit:input", source);

        Printf(incode, "%s\n", tm);
        Delete(source_len);
      } else {
        printf("tmap:in:parse was null\n");
      }
      p2 = Getattr(p2, "tmap:in:next");
      continue;
    } else {
      Swig_warning(WARN_TYPEMAP_IN_UNDEF, input_file, line_number, "Unable to use type %s as a function argument.\n", SwigType_str(pt, 0));
    }
    p2 = nextSibling(p);
  }

  // print input typemap conversions to wrapper.
  Printv(f->code, incode, "\n", NIL);

  /* Insert constraint checking code */
/*
    for (p = parms; p;) {
      if ((tm = Getattr(p, "tmap:check"))) {
        Replaceall(tm, "$target", Getattr(p, "lname"));
        Printv(f->code, tm, "\n", NIL);
        p = Getattr(p, "tmap:check:next");
      } else {
        p = nextSibling(p);
      }
    }
*/

  /* Insert cleanup code */
/*
    for (i = 0, p = parms; p; i++) {
      if (!checkAttribute(p, "tmap:in:numinputs", "0")
          && !Getattr(p, "tmap:in:parse") && (tm = Getattr(p, "tmap:freearg"))) {
        if (Len(tm) != 0) {
          Replaceall(tm, "$source", Getattr(p, "lname"));
          Printv(cleanup, tm, "\n", NIL);
        }
        p = Getattr(p, "tmap:freearg:next");
      } else {
        p = nextSibling(p);
      }
    }
*/

  /* Insert argument output code */
  String *outarg = NewString("");
  for (i = 0, p = parms; p; i++) {
    if ((tm = Getattr(p, "tmap:argout"))) {

      source = Getattr(p, "name");
      source_len = NewString("");
      Printv(source_len, Getattr(p, "name"), "_len", NIL);

      Replaceall(tm, "$input", Getattr(p, "lname"));
      Replaceall(tm, "$output", source);
      Replaceall(tm, "$length", source_len);
      Printv(outarg, tm, "\n", NIL);
      p = Getattr(p, "tmap:argout:next");
      Delete(source_len);
    } else {
      p = nextSibling(p);
    }
  }

  // attach local variables to parameters

  // create function call
  // get function definition arguments
  String *empty_string = NewString("");
  String *arg_names = Swig_cfunction_call(empty_string, parms);
  if (arg_names) {
    // remove parenthesis before and after argument list
    Delitem(arg_names, 0);
    Delitem(arg_names, DOH_END);
  }

  if (!is_void_return) {
    Printf(f->code, "result = ");
  }
  Printv(f->code, name, "(", arg_names, ");\n", NIL);

  // attach output arguments
  Printv(f->code, "\n", outarg, "\n", NIL);

  if (!is_void_return) {
    Printf(f->code, "return result;\n");
  }
  Printf(f->code, "}");

  // write out the wrapper file
  Wrapper_print(f, f_wrappers);

  Delete(name);
//    Delete(type);
  Delete(wname);
  Delete(arg_names);
  Delete(return_type);
//    Delete(proto);
  Delete(empty_string);
  Delete(incode);
  Delete(outarg);

  return SWIG_OK;
}

extern "C" Language *swig_fortran(void) {
  return new FORTRAN();
}

/* -----------------------------------------------------------------------------
 * Static member variables
 * ----------------------------------------------------------------------------- */

const char *FORTRAN::usage = (char *) "\
C Options (available with -c)\n\
\n";
