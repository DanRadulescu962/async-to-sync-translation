"""
main_logic: mbox_removal.py

mbox_removal does different node elimination based on the information from the
config file
"""
from pycparser.c_ast import *

from utils.generators import NullAssignVisitor, UselessFuncVisitor, EmptyInstrVisitor, ExtractRoundVisitor
from utils.utils import find_parent, get_extern_while_body


def remove_mbox_assign_to_zero(extern_while_body, mbox_name):
    """
    assign mbox = 0 or mess_name will be removed from the tree
    :param extern_while_body:
    :return:
    """

    v = NullAssignVisitor(mbox_name)

    if (not extern_while_body) or (not extern_while_body.block_items):
        return
    v.visit(extern_while_body)

    for el in v.result_list:
        p = find_parent(extern_while_body, el)

        if isinstance(p, Compound):
            p.block_items.remove(el)

        if isinstance(p, If):
            if p.iftrue == el:
                p.iftrue = None
            elif p.iffalse == el:
                p.iffalse = None


def remove_list_dispose(extern_while_body, func_names):

    v = UselessFuncVisitor(func_names)
    v.visit(extern_while_body)

    for el in v.result_list:
        p = find_parent(extern_while_body, el)

        if isinstance(p, Compound):
            p.block_items.remove(el)

        if isinstance(p, If):
            if p.iftrue == el:
                p.iftrue = None
            elif p.iffalse == el:
                p.iffalse = None


def remove_null_if(extern_while_body):

    keep_removing = True

    # Captures nested deletions
    while keep_removing:
        v = EmptyInstrVisitor()
        v.visit(extern_while_body)

        if len(v.result_list) == 0:
            keep_removing = False

        for el in v.result_list:
            p = find_parent(extern_while_body, el)

            if isinstance(p, Compound):
                p.block_items.remove(el)

            if isinstance(p, If):
                if p.iftrue == el:
                    p.iftrue = None
                elif p.iffalse == el:
                    p.iffalse = None


def remove_mbox(extern_while_body, mess_names, func_names):
    """
    removes mbox assigns to 0 and ifs where it is freed
    :param extern_while_body:
    :return:
    """
    remove_mbox_assign_to_zero(extern_while_body, mess_names)
    remove_list_dispose(extern_while_body, func_names)
    remove_null_if(extern_while_body)


def clean_round_assigs(trees_dict, trees_paths_dict, round_name):
    """
    Used to clean round assigs and then eliminate empty ifs
    :param trees_dict:
    :param trees_paths_dict:
    :return:
    """
    for _list_ in trees_dict.values():
        for _ast_ in _list_:
            body = get_extern_while_body(_ast_)
            v = ExtractRoundVisitor(round_name)
            v.visit(body)

            for el in v.result:
                p = find_parent(body, el)
                if not isinstance(p, Compound):
                    continue
                ind = p.block_items.index(el)
                del p.block_items[ind]

            remove_null_if(body)

    for td in trees_paths_dict.values():
        for aux_td in td:
            for comp_tup in aux_td:
                v = ExtractRoundVisitor(round_name)
                comp = comp_tup[0]
                v.visit(comp)

                for el in v.result:
                    p = find_parent(comp, el)
                    if not isinstance(p, Compound):
                        continue
                    ind = p.block_items.index(el)
                    del p.block_items[ind]

                remove_null_if(comp)
