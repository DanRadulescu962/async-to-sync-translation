import copy

from take_old import *

# from cod_alex import show_func_calls
ast = parse_file(filename="examples/c_files/tpc_AMIT.c", use_cpp=False)


generator = c_generator.CGenerator()
# print(generator.visit(ast))
generator.visit(ast)
# main_function = ast.ext[1]

extern_while_body = None

x = get_extern_while_body(ast)

whiles_to_if(x)


print generator.visit(ast)




copie = copy.deepcopy(ast)

labels = get_labels("examples/c_files/tpc_AMIT.c", "lab")

print labels

trees_dict, paths_dict = get_paths_trees(copie, labels, "lab")
print len(trees_dict), len(paths_dict)



take_code_from_file(copie, "examples/c_files/tpc_AMIT.c", "lab")