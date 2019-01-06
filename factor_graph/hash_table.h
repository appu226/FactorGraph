/*

Copyright 2019 Parakram Majumdar

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/




#pragma once

#include "factor_graph.h"

extern void hash_table_add(bdd_ptr * G, int Gs, bdd_ptr V, bdd_ptr * r, int rs);
extern void hash_table_add_fg(factor_graph * G, bdd_ptr V, bdd_ptr * r, int rs);
extern bdd_ptr * hash_table_lookup(bdd_ptr * G, int Gs, bdd_ptr V, int *rs);
extern bdd_ptr * hash_table_lookup_fg(factor_graph * G, bdd_ptr V, int *rs);
extern void hash_table_init(DdManager *m);
extern void hash_table_clean();
