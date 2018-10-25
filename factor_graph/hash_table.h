#include"factor_graph.h"

extern void hash_table_add(bdd_ptr * G, int Gs, bdd_ptr V, bdd_ptr * r, int rs);
extern void hash_table_add_fg(factor_graph * G, bdd_ptr V, bdd_ptr * r, int rs);
extern bdd_ptr * hash_table_lookup(bdd_ptr * G, int Gs, bdd_ptr V, int *rs);
extern bdd_ptr * hash_table_lookup_fg(factor_graph * G, bdd_ptr V, int *rs);
extern void hash_table_init(DdManager *m);
extern void hash_table_clean();
