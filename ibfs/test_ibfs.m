unary = [2 0; 0 2];
edge = [1; 2];
pair = [0; 0.5; 0.5; 0];

[label, e] = mex_gc_ibfs(unary, edge, pair);
