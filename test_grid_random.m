addpath('ibfs');
addpath('bk');

grid_size = 20;
num_nodes = grid_size^2;
num_edges = grid_size*(grid_size-1)*2;

potential_unary = rand(2, num_nodes);
potential_pair = rand(4, num_edges);
potential_pair(1,:) = 0;
potential_pair(4,:) = 0;

edges = zeros(2, num_edges);
idx = 1;
for i=1:grid_size
    for j=1:grid_size
        % right
        if (j < grid_size)
            s = (i-1)+(j-1)*grid_size;
            t = (i-1)+j*grid_size;
            edges(1,idx) = s+1;
            edges(2,idx) = t+1;
            idx = idx+1;
        end

        % down
        if (i < grid_size)
            s = (i-1)+(j-1)*grid_size;
            t = i+(j-1)*grid_size;
            edges(1,idx) = s+1;
            edges(2,idx) = t+1;
            idx = idx+1;
        end
    end
end


[label, e_ibfs] = mex_gc_ibfs(potential_unary, edges, potential_pair);
[label, e_bk] = mex_gc_bk(potential_unary, edges, potential_pair);
e_ibfs
e_bk

