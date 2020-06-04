
/* This module provides the simulation of dynamic loading */

#include "Python.h"
#include "importdl.h"

const struct filedescr _PyImport_DynLoadFiletab[] = {
  {".a", "rb", C_EXTENSION},
  {0, 0}
};

extern struct _inittab _PyImport_Inittab[];

dl_funcptr _PyImport_GetDynLoadFunc(const char *fqname, const char *shortname,
                                    const char *pathname, FILE *fp)
{
  struct _inittab *tab = _PyImport_Inittab;
  while (tab->name && strcmp(shortname, tab->name)) tab++;

  return tab->initfunc;
}
