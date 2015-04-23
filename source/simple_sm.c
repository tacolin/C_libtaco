#include "simple_sm.h"

////////////////////////////////////////////////////////////////////////////////

static void _destroyTrans(void* input)
{
    check_if(input == NULL, return, "input is null");

    tSmTrans* trans = (tSmTrans*)input;
    list_clean(&trans->route);
    free(trans);
}

static void _destroySt(void* input)
{
    check_if(input == NULL, return, "input is null");

    tSmSt* st = (tSmSt*)input;
    // list_clean(&st->translist);

    if (st->guard_array)
    {
        free(st->guard_array);
    }

    if (st->trans_array)
    {
        int num = 255 << st->sm->ev_bits;
        int i;
        for (i=0; i<num; i++)
        {
            if (st->trans_array[i])
            {
                _destroyTrans(st->trans_array[i]);
            }
        }
        free(st->trans_array);
    }
    free(st);
}

////////////////////////////////////////////////////////////////////////////////

tSmStatus sm_init(tSm* sm, char* name, int max_ev_value)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");
    check_if(max_ev_value <= 0, return SM_ERROR, "max_ev_value = %d invalid", max_ev_value);

    snprintf(sm->name, SM_NAME_SIZE, "%s", name);
    sm->max_ev_value = max_ev_value;

    int tmp = max_ev_value;
    for(sm->ev_bits=0; tmp>0; sm->ev_bits++)
    {
        tmp >>= 1;
    }

    sm->root = calloc(sizeof(tSmSt), 1);
    check_if(sm->root == NULL, return SM_ERROR, "calloc failed");

    snprintf(sm->root->name, SM_NAME_SIZE, "%s-root", name);

    tTreeStatus tree_ret   = tree_init(&sm->st_tree, sm->root, _destroySt);
    check_if(tree_ret != TREE_OK, return SM_ERROR, "tree_init failed");

    tQueueStatus queue_ret = queue_init(&sm->evqueue, SM_MAX_EVQUEUE_NUM, free, QUEUE_UNSUSPEND, QUEUE_UNSUSPEND);
    check_if(queue_ret != QUEUE_OK, return SM_ERROR, "queue_init failed");

    sm->is_busy    = 0;
    sm->is_started = 0;
    sm->is_init    = 1;

    return SM_OK;
}

tSmStatus sm_uninit(tSm* sm)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");
    check_if(sm->is_init != 1, return SM_ERROR, "sm is not init yet");

    if (sm->is_started)
    {
        sm_stop(sm);
    }

    sm->is_init = 0;

    queue_clean(&sm->evqueue);
    tree_clean(&sm->st_tree);

    sm->root = NULL;

    return SM_OK;
}

tSmSt* sm_rootSt(tSm* sm)
{
    check_if(sm == NULL, return NULL, "sm is null");
    return sm->root;
}

tSmSt* sm_createSt(tSm* sm, char* name, tSmStEntryFn on_entry, tSmStExitFn on_exit)
{
    check_if(sm == NULL, return NULL, "sm is null");
    check_if(sm->is_init != 1, return NULL, "sm is not init yet");
    check_if(name == NULL, return NULL, "name is null");

    tSmSt* st = calloc(sizeof(tSmSt), 1);
    check_if(st == NULL, return NULL, "calloc failed");

    snprintf(st->name, SM_NAME_SIZE, "%s", name);

    st->on_entry = on_entry;
    st->on_exit  = on_exit;
    st->sm       = sm;

    // list_init(&st->translist, _destroyTrans);

    st->guard_array = calloc(sizeof(tSmGuardFn), sm->max_ev_value);
    st->trans_array = calloc(sizeof(tSmTrans*), 255 << sm->ev_bits);

    return st;
}

tSmStatus sm_addSt(tSm* sm, tSmSt* parent_st, tSmSt* child_st, int is_init_state)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");
    check_if(sm->is_init != 1, return SM_ERROR, "sm is not init yet");
    check_if(parent_st == NULL, return SM_ERROR, "parent_st is null");
    check_if(child_st == NULL, return SM_ERROR, "child_st is null");

    if (is_init_state)
    {
        tList* childs = tree_childs(parent_st);
        check_if(childs == NULL, return SM_ERROR, "tree_childs failed");

        void* obj;
        tSmSt* st;
        LIST_FOREACH(childs, obj, st)
        {
            check_if(st->is_init_state != 0, return SM_ERROR, "parent (%s) has init_state (%s)", parent_st->name, st->name);
        }
    }

    tTreeStatus tree_ret = tree_add(parent_st, child_st);
    check_if(tree_ret != TREE_OK, return SM_ERROR, "tree_add failed");

    child_st->is_init_state = is_init_state;

    return SM_OK;
}

static tSmSt* _findSt(tSm* sm, char* st_name)
{
    check_if(sm == NULL, return NULL, "sm is null");
    check_if(st_name == NULL, return NULL, "st_name is null");
    check_if(sm->is_init != 1, return NULL, "sm is not init yet");

    void* obj = NULL;
    tSmSt* st = NULL;
    LIST_FOREACH(&sm->st_tree.nodes, obj, st)
    {
        if (0 == strcmp(st_name, st->name))
        {
            break;
        }
    }
    return st;
}

tSmTrans* sm_createTrans(tSm* sm, int evid, tSmGuardFn on_guard, int8_t guard_value, char* next_st_name, tSmTransAction on_act)
{
    check_if(sm == NULL, return NULL, "sm is null");
    check_if(evid > sm->max_ev_value, return NULL, "ev %d > max_ev_value %d", evid, sm->max_ev_value);
    check_if(sm->is_init != 1, return NULL, "sm is not init yet");
    check_if(next_st_name == NULL, return NULL, "next_st_name is null");

    tSmSt* st = _findSt(sm, next_st_name);
    check_if(st == NULL, return NULL, "state %s is not added to sm", next_st_name);

    tSmTrans* trans = calloc(sizeof(tSmTrans), 1);
    check_if(trans == NULL, return NULL, "calloc failed");

    trans->evid        = evid;
    trans->on_guard    = on_guard;
    trans->guard_value = guard_value;
    trans->next_st     = st;
    trans->on_act      = on_act;

    if (LIST_OK != list_init(&trans->route, NULL))
    {
        derror("list_init failed");
        free(trans);
        return NULL;
    }

    return trans;
}

static tSmStatus _doInitTrans(tSm* sm, tSmSt* st)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");
    check_if(sm->is_init != 1, return SM_ERROR, "sm is not init yet");
    check_if(st == NULL, return SM_ERROR, "st is null");

    tList* childs = tree_childs(st);
    check_if(childs == NULL, return SM_ERROR, "tree_childs failed");

    void* obj;
    tSmSt* ch;
    LIST_FOREACH(childs, obj, ch)
    {
        if (ch->is_init_state)
        {
            st->curr_sub_state = ch;
            if (ch->on_entry)
            {
                ch->on_entry(sm, ch);
            }
            _doInitTrans(sm, ch);
        }
    }

    return SM_OK;
}

static tSmSt* _findInitSubState(tSmSt* st)
{
    if (TREE_OK == tree_isLeaf(st))
    {
        return st;
    }

    tTreeHdr* hdr = (tTreeHdr*)st;
    void* obj;
    tSmSt* subst;
    LIST_FOREACH(&hdr->childs, obj, subst)
    {
        if (subst->is_init_state)
        {
            return _findInitSubState(subst);
        }
    }

    return NULL;
}

tSmStatus sm_addTrans(tSm* sm, tSmSt* start_st, tSmTrans* trans)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");
    check_if(sm->is_init != 1, return SM_ERROR, "sm is not init yet");
    check_if(start_st == NULL, return SM_ERROR, "start_st is null");
    check_if(trans == NULL, return SM_ERROR, "trans is null");

    if (start_st != trans->next_st)
    {
        tree_route(start_st, trans->next_st, &trans->route);

        trans->ancestor = tree_commonAncestor(start_st, trans->next_st);
        check_if(trans->ancestor == NULL, return SM_ERROR, "tree_commonAncestor failed");
    }
    else
    {
        list_append(&trans->route, start_st);
        list_append(&trans->route, tree_parent(start_st));
        list_append(&trans->route, trans->next_st);

        trans->ancestor = tree_parent(start_st);
        check_if(trans->ancestor == NULL, return SM_ERROR, "tree_commonAncestor failed");
    }

    tSmSt* init_sub = _findInitSubState(trans->next_st);
    if (init_sub != trans->next_st)
    {
        tList tmplist;
        list_init(&tmplist, NULL);
        tree_route(trans->next_st, init_sub, &tmplist);

        void* obj;
        tSmSt* tmpst;
        LIST_FOREACH(&tmplist, obj, tmpst)
        {
            if (tmpst != trans->next_st)
            {
                list_append(&trans->route, tmpst);
            }
        }
        list_clean(&tmplist);
    }

    // list_append(&start_st->translist, trans);

    start_st->guard_array[trans->evid] = trans->on_guard;
    int idx = 0;
    if (trans->on_guard)
    {
        idx = (trans->guard_value << sm->ev_bits) | trans->evid;
    }
    else
    {
        idx = trans->evid;
    }
    idx += 127;
    start_st->trans_array[idx] = trans;

    return SM_OK;
}

tSmStatus sm_start(tSm* sm)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");
    check_if(sm->is_init != 1, return SM_ERROR, "sm is not init yet");
    check_if(sm->is_started != 0, return SM_ERROR, "sm is already started");

    _doInitTrans(sm, sm->root);
    sm->is_started = 1;

    return SM_OK;
}

tSmStatus sm_stop(tSm* sm)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");
    check_if(sm->is_init != 1, return SM_ERROR, "sm is not init yet");
    check_if(sm->is_started == 0, return SM_ERROR, "sm is stopped");

    sm->is_started = 0;
    return SM_OK;
}

// static tSmTrans* _findTrans(tSm* sm, tSmSt* st, tSmEv* ev)
// {
//     void* obj = NULL;
//     tSmTrans* trans = NULL;
//     LIST_FOREACH(&st->translist, obj, trans)
//     {
//         if (trans->evid == ev->evid)
//         {
//             if (trans->on_guard)
//             {
//                 if (trans->on_guard(sm, st, ev) == trans->guard_value)
//                 {
//                     break;
//                 }
//             }
//             else
//             {
//                 break;
//             }
//         }
//     }

//     return trans;
// }

static tSmTrans* _findTrans(tSm* sm, tSmSt* st, tSmEv* ev)
{
    if (st->guard_array == NULL)
    {
        return NULL;
    }

    int idx;
    if (st->guard_array[ev->evid] == NULL)
    {
        idx = ev->evid;
    }
    else
    {
        int8_t guard_value = st->guard_array[ev->evid](sm, st, ev);
        idx = (guard_value << sm->ev_bits) | ev->evid;
    }
    idx += 127;
    return st->trans_array[idx];
}

static tSmStatus _handleEv(tSm* sm, tSmSt* st, tSmTrans* trans, tSmEv* ev)
{
    static const int mode_exit = 0;
    static const int mode_enter = 1;

    tList tmp;
    list_init(&tmp, NULL);
    tSmSt* tmpst;
    for (tmpst = st->curr_sub_state; tmpst; tmpst = tmpst->curr_sub_state)
    {
        list_insert(&tmp, tmpst);
    }
    void* obj;
    LIST_FOREACH(&tmp, obj, tmpst)
    {
        if (tmpst->on_exit)
        {
            tmpst->on_exit(sm, tmpst);
        }
    }
    list_clean(&tmp);

    int mode = mode_exit;
    tSmSt* pass_st;
    tSmSt* parent;
    // void* obj;
    LIST_FOREACH(&trans->route, obj, pass_st)
    {
        if (pass_st == trans->ancestor)
        {
            trans->on_act(sm, ev);
            mode = mode_enter;
        }
        else if (mode == mode_exit)
        {
            if (pass_st->on_exit)
            {
                pass_st->on_exit(sm, pass_st);
            }
        }
        else
        {
            if (pass_st->on_entry)
            {
                pass_st->on_entry(sm, pass_st);
            }

            parent = tree_parent(pass_st);
            if (parent)
            {
                parent->curr_sub_state = pass_st;
            }
        }
    }

    return SM_OK;
}

static tSmStatus _propogateEv(tSm* sm, tSmSt* st, tSmEv* ev)
{
    tSmTrans* trans = _findTrans(sm, st, ev);
    if (trans != NULL)
    {
        return _handleEv(sm, st, trans, ev);
    }

    if (st->curr_sub_state == NULL)
    {
        return SM_OK;
    }

    return _propogateEv(sm, st->curr_sub_state, ev);
}

tSmStatus sm_sendEv(tSm* sm, int evid, intptr_t arg1, intptr_t arg2)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");
    check_if(evid > sm->max_ev_value, return SM_ERROR, "ev %d > max_ev_value %d", evid, sm->max_ev_value);
    check_if(sm->is_init != 1, return SM_ERROR, "sm is not init yet");
    check_if(sm->is_started == 0, return SM_ERROR, "sm is stopped");

    if (sm->is_busy)
    {
        tSmEv* ev = calloc(sizeof(tSmEv), 1);
        ev->evid = evid;
        ev->arg1 = arg1;
        ev->arg2 = arg2;
        queue_push(&sm->evqueue, ev);
        return SM_OK;
    }

    sm->is_busy = 1;

    tSmEv tmpev = {.evid = evid, .arg1 = arg1, .arg2 = arg2};
    _propogateEv(sm, sm->root, &tmpev);

    tSmEv* ev;
    while (ev = queue_pop(&sm->evqueue))
    {
        _propogateEv(sm, sm->root, ev);
        free(ev);
    }

    sm->is_busy = 0;
    return SM_OK;
}
