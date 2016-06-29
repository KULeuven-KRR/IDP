/*
 * saucy.c
 * Searching for Automorphisms in Underlying CNF, yes?
 *
 * by Paul T. Darga <pdarga@umich.edu>
 * and Mark Liffiton <liffiton@umich.edu>
 * and Hadi Katebi <hadik@eecs.umich.edu>
 * 
 * Copyright (C) 2004, The Regents of the University of Michigan
 * See the LICENSE file for details.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef int saucy_consumer(int, const int *, int, int *, void *);

struct saucy;

struct saucy_stats {
  double grpsize_base;
  int grpsize_exp;
  int levels;
  int nodes;
  int bads;
  int gens;
  int support;
};

struct saucy_graph {
  int n; // number of nodes
  int e; // number of edges
  int *adj; // size is n+1, adj[0]=0 and adj[n]=2*e, edg[adj[i]]-edg[adj[i+1]-1] contains the nodes with which node i is connected
  int *edg; // size is 2*e, nodes connected to other nodes, see adj, lengh
  int *colors; // size is n, colors[i] gives the color of node i
};

struct saucy *saucy_alloc(int n);

void saucy_search(
  struct saucy *s,
  const struct saucy_graph *graph,
  int directed,
  saucy_consumer *consumer,
  void *arg,
  struct saucy_stats *stats);

void saucy_free(struct saucy *s);


#ifdef __cplusplus
}
#endif

