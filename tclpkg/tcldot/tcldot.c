/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "tcldot.h"
#include <stdlib.h>
#include <string.h>

static int dotnew(ClientData clientData, Tcl_Interp *interp,
#ifndef TCLOBJ
                  int argc, char *argv[]
#else  /* TCLOBJ */
                  int argc, Tcl_Obj *CONST objv[]
#endif /* TCLOBJ */
) {
  ictx_t *ictx = (ictx_t *)clientData;
  Agraph_t *g;
  int i;
  Agdesc_t kind;

  if ((argc < 2)) {
    Tcl_AppendResult(
        interp, "wrong # args: should be \"", argv[0],
        " graphtype ?graphname? ?attributename attributevalue? ?...?\"", NULL);
    return TCL_ERROR;
  }
  if (strcmp("digraph", argv[1]) == 0) {
    kind = Agdirected;
  } else if (strcmp("digraphstrict", argv[1]) == 0) {
    kind = Agstrictdirected;
  } else if (strcmp("graph", argv[1]) == 0) {
    kind = Agundirected;
  } else if (strcmp("graphstrict", argv[1]) == 0) {
    kind = Agstrictundirected;
  } else {
    Tcl_AppendResult(interp, "bad graphtype \"", argv[1], "\": must be one of:",
                     "\n\tdigraph, digraphstrict, graph, graphstrict.", NULL);
    return TCL_ERROR;
  }
  if (argc % 2) {
    /* if odd number of args then argv[2] is name */
    g = agopen(argv[2], kind, (Agdisc_t *)ictx);
    i = 3;
  } else {
    /* else use handle as name */
    g = agopen(Tcl_GetStringResult(interp), kind, (Agdisc_t *)ictx);
    i = 2;
  }
  if (!g) {
    Tcl_AppendResult(interp, "\nFailure to open graph.", NULL);
    return TCL_ERROR;
  }
  setgraphattributes(g, &argv[i], argc - i);
  Tcl_AppendResult(interp, obj2cmd(g), NULL);

  return TCL_OK;
}

static int dotread(ClientData clientData, Tcl_Interp *interp,
#ifndef TCLOBJ
                   int argc, char *argv[]
#else  /* TCLOBJ */
                   int argc, Tcl_Obj *CONST objv[]
#endif /* TCLOBJ */
) {
  Agraph_t *g;
  Tcl_Channel channel;
  int mode;
  ictx_t *ictx = (ictx_t *)clientData;

  ictx->myioDisc.afread =
      myiodisc_afread; /* replace afread to use Tcl Channels */

  if (argc < 2) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                     " fileHandle\"", NULL);
    return TCL_ERROR;
  }
  channel = Tcl_GetChannel(interp, argv[1], &mode);
  if (channel == NULL || !(mode & TCL_READABLE)) {
    Tcl_AppendResult(interp, "\nChannel \"", argv[1], "\"", "is unreadable.",
                     NULL);
    return TCL_ERROR;
  }
  /*
   * read a graph from the channel, the channel is left open
   *   ready to read the first line after the last line of
   *   a properly parsed graph. If the graph doesn't parse
   *   during reading then the channel will be left at EOF
   */
  g = agread((FILE *)channel, (Agdisc_t *)clientData);
  if (!g) {
    Tcl_AppendResult(interp, "\nFailure to read graph \"", argv[1], "\"", NULL);
    if (agerrors()) {
      Tcl_AppendResult(interp, " because of syntax errors.", NULL);
    }
    return TCL_ERROR;
  }
  if (agerrors()) {
    Tcl_AppendResult(interp, "\nSyntax errors in file \"", argv[1], " \"",
                     NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(interp, obj2cmd(g), NULL);
  return TCL_OK;
}

static int dotstring(ClientData clientData, Tcl_Interp *interp,
#ifndef TCLOBJ
                     int argc, char *argv[]
#else  /* TCLOBJ */
                     int argc, Tcl_Obj *CONST objv[]
#endif /* TCLOBJ */
) {
  Agraph_t *g;
  ictx_t *ictx = (ictx_t *)clientData;
  rdr_t rdr;

  if (argc < 2) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], " string\"",
                     NULL);
    return TCL_ERROR;
  }

  ictx->myioDisc.afread =
      myiodisc_memiofread; /* replace afread to use memory range */
  rdr.data = argv[1];
  rdr.len = strlen(rdr.data);
  rdr.cur = 0;

  /* agmemread() is broken for our use because it replaces the id disc */
  g = agread(&rdr, (Agdisc_t *)clientData);
  if (!g) {
    Tcl_AppendResult(interp, "\nFailure to read string \"", argv[1], "\"",
                     NULL);
    if (agerrors()) {
      Tcl_AppendResult(interp, " because of syntax errors.", NULL);
    }
    return TCL_ERROR;
  }
  if (agerrors()) {
    Tcl_AppendResult(interp, "\nSyntax errors in string \"", argv[1], " \"",
                     NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(interp, obj2cmd(g), NULL);
  return TCL_OK;
}

int Tcldot_Init(Tcl_Interp *interp) {
  ictx_t *ictx = calloc(1, sizeof(ictx_t));
  if (!ictx)
    return TCL_ERROR;

  ictx->interp = interp;
  /* build disciplines dynamically so we can selectively replace functions */

  ictx->myioDisc.afread =
      NULL; /* set in dotread() or dotstring() according to need */
  ictx->myioDisc.putstr = AgIoDisc.putstr; /* no change */
  ictx->myioDisc.flush = AgIoDisc.flush;   /* no change */

  ictx->mydisc.id = &myiddisc;         /* complete replacement */
  ictx->mydisc.io = &(ictx->myioDisc); /* change parts */

  ictx->ctr = 1; /* init to first odd number,  increment by 2 */

#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
    return TCL_ERROR;
  }
#else
  if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL) {
    return TCL_ERROR;
  }
#endif
  // inter-release Graphviz versions have a number including '~dev.' that does
  // not comply with TCL version number rules, so replace this with 'b'
  char adjusted_version[sizeof(PACKAGE_VERSION)] = PACKAGE_VERSION;
  char *tilde_dev = strstr(adjusted_version, "~dev.");
  if (tilde_dev != NULL) {
    *tilde_dev = 'b';
    memmove(tilde_dev + 1, tilde_dev + strlen("~dev."),
            strlen(tilde_dev + strlen("~dev.")) + 1);
  }
  if (Tcl_PkgProvide(interp, "Tcldot", adjusted_version) != TCL_OK) {
    return TCL_ERROR;
  }

#ifdef HAVE_LIBGD
  Gdtclft_Init(interp);
#endif

  /* create a GraphViz Context and pass a pointer to it in clientdata */
  ictx->gvc = gvContextPlugins(lt_preloaded_symbols, DEMAND_LOADING);

#ifndef TCLOBJ
  Tcl_CreateCommand(interp, "dotnew", dotnew, ictx, free);
  Tcl_CreateCommand(interp, "dotread", dotread, ictx, NULL);
  Tcl_CreateCommand(interp, "dotstring", dotstring, ictx, NULL);
#else  /* TCLOBJ */
  Tcl_CreateObjCommand(interp, "dotnew", dotnew, ictx, free);
  Tcl_CreateObjCommand(interp, "dotread", dotread, ictx, NULL);
  Tcl_CreateObjCommand(interp, "dotstring", dotstring, ictx, NULL);
#endif /* TCLOBJ */

  return TCL_OK;
}

int Tcldot_SafeInit(Tcl_Interp *interp) { return Tcldot_Init(interp); }

int Tcldot_builtin_Init(Tcl_Interp *interp) { return Tcldot_Init(interp); }
