#include "simple_sm.h"

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

    list_clean(&st->translist);

    free(st);
}

////////////////////////////////////////////////////////////////////////////////

tSmStatus sm_init(tSm* sm, char* name)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");

    snprintf(sm->name, SM_NAME_SIZE, "%s", name);

    sm->root = calloc(sizeof(tSmSt), 1);

    snprintf(sm->root->name, SM_NAME_SIZE, "%s-root", name);

    tree_init(&sm->st_tree, sm->root, _destroySt);

    queue_init(&sm->evqueue, sm->name, SM_MAX_EV_NUM, free, QUEUE_UNSUSPEND, QUEUE_UNSUSPEND);

    hash_init(&sm->ev_hash, SM_MAX_EV_NUM, free);
    sm->last_ev_id = 0;

    sm->curr_st = NULL;

    sm->is_busy = 0;
    sm->is_started = 0;
    sm->is_init = 1;

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

    hash_uninit(&sm->ev_hash);
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

    snprintf(st->name, SM_NAME_SIZE, "%s", name);
    st->on_entry = on_entry;
    st->on_exit = on_exit;
    st->sm = sm;

    list_init(&st->translist, _destroyTrans);

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
        tTreeHdr* hdr = (tTreeHdr*)parent_st;
        tListObj* obj;
        tSmSt* st;
        LIST_FOREACH(&hdr->childs, obj, st)
        {
            check_if(st->is_init_state != 0, return SM_ERROR, "parent (%s) has init_state (%s)", parent_st->name, st->name);
        }
    }

    tree_add(parent_st, child_st);

    child_st->is_init_state = is_init_state;

    return SM_OK;
}

tSmStatus sm_addEv(tSm* sm, char* ev_name)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");
    check_if(sm->is_init != 1, return SM_ERROR, "sm is not init yet");
    check_if(ev_name == NULL, return SM_ERROR, "ev_name is null");

    int* evid = hash_find(&sm->ev_hash, ev_name);
    check_if(evid != NULL, return SM_ERROR, "ev_name (%s) is already added", ev_name);

    evid = malloc(sizeof(int));
    sm->last_ev_id++;
    *evid  = sm->last_ev_id;

    hash_add(&sm->ev_hash, ev_name, evid);

    return SM_OK;
}

tSmTrans* sm_createTrans(tSm* sm, char* ev_name, tSmGuardFn on_guard, int guard_value, char* next_st_name, tSmTransAction on_act)
{
    check_if(sm == NULL, return NULL, "sm is null");
    check_if(sm->is_init != 1, return NULL, "sm is not init yet");
    check_if(ev_name == NULL, return NULL, "ev_name is null");
    check_if(next_st_name == NULL, return NULL, "next_st_name is null");

    int* evid = hash_find(&sm->ev_hash, ev_name);
    if (evid == NULL)
    {
        sm_addEv(sm, ev_name);
        evid = hash_find(&sm->ev_hash, ev_name);
    }

    tListObj* obj;
    tSmSt* st;
    LIST_FOREACH(&sm->st_tree.nodes, obj, st)
    {
        if (0 == strcmp(next_st_name, st->name))
        {
            break;
        }
    }
    check_if(st == NULL, return NULL, "st (%s) is not added to sm", next_st_name);

    tSmTrans* trans = calloc(sizeof(tSmTrans), 1);

    list_init(&trans->route, NULL);

    trans->evid = *evid;
    trans->on_guard = on_guard;
    trans->guard_value = guard_value;
    trans->next_st = st;
    trans->on_act = on_act;

    return  trans;
}

tSmStatus sm_addTrans(tSm* sm, tSmSt* start_st, tSmTrans* trans)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");
    check_if(sm->is_init != 1, return SM_ERROR, "sm is not init yet");
    check_if(start_st == NULL, return SM_ERROR, "start_st is null");
    check_if(trans == NULL, return SM_ERROR, "trans is null");

    tree_route(start_st, trans->next_st, &trans->route);

    tSmSt* ancestor = tree_commonAncestor(start_st, trans->next_st);
    check_if(ancestor == NULL, return SM_ERROR, "ancestor is null");

    trans->ancestor = ancestor;

    list_append(&start_st->translist, trans);

    return SM_OK;
}

static tSmSt* _findInitSubState(tSmSt* st)
{
    if (TREE_OK == tree_isLeaf(st))
    {
        return st;
    }

    tTreeHdr* hdr = (tTreeHdr*)st;
    tListObj* obj;
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

tSmStatus sm_start(tSm* sm)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");
    check_if(sm->is_init != 1, return SM_ERROR, "sm is not init yet");
    check_if(sm->is_started != 0, return SM_ERROR, "sm is already started");

    sm->curr_st = _findInitSubState(sm->root);
    check_if(sm->curr_st == NULL, return SM_ERROR, "_findInitSubState failed");

    tList init_trans;
    list_init(&init_trans, NULL);
    tree_route(sm->root, sm->curr_st, &init_trans);

    tListObj* obj;
    tSmSt* st;
    LIST_FOREACH(&init_trans, obj, st)
    {
        if (st->on_entry)
        {
            st->on_entry(sm, st);
        }
    }
    list_clean(&init_trans);

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

tSmTrans* _findTrans(tSm* sm, tSmSt* st, int evid, void* ev_data)
{
    tListObj* obj = NULL;
    tSmTrans* trans = NULL;
    LIST_FOREACH(&st->translist, obj, trans)
    {
        if (trans->evid == evid)
        {
            if (trans->on_guard)
            {
                if (trans->on_guard(sm, st, ev_data) == trans->guard_value)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    return trans;
}

tSmStatus _procEv(tSm* sm, tSmSt* curr_st, int evid, void* ev_data)
{
    tSmSt* parent = (tSmSt*)(curr_st->hdr.parent);
    if (parent && parent != sm->root)
    {
        _procEv(sm, parent, evid, ev_data);
    }

    tSmTrans* trans = _findTrans(sm, curr_st, evid, ev_data);
    if (trans == NULL)
    {
        return SM_OK;
    }

    static const int mode_exit = 0;
    static const int mode_enter = 1;

    int mode = mode_exit;
    tSmSt* pass_st;
    tListObj* obj;
    LIST_FOREACH(&trans->route, obj, pass_st)
    {
        if (pass_st == trans->ancestor)
        {
            trans->on_act(sm, ev_data);
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
        }
    }
    sm->curr_st = trans->next_st;

    return SM_OK;
}

tSmStatus sm_sendEv(tSm* sm, char* ev_name, void* ev_data)
{
    check_if(sm == NULL, return SM_ERROR, "sm is null");
    check_if(sm->is_init != 1, return SM_ERROR, "sm is not init yet");
    check_if(sm->is_started == 0, return SM_ERROR, "sm is stopped");

    int* evid = hash_find(&sm->ev_hash, ev_name);
    check_if(evid == NULL, return SM_ERROR, "ev (%s) is not added to sm", ev_name);

    if (sm->is_busy)
    {
        tSmEv* ev = calloc(sizeof(tSmEv), 1);
        ev->evid = *evid;
        ev->ev_data = ev_data;
        queue_put(&sm->evqueue, ev);
        return SM_OK;
    }

    sm->is_busy = 1;
    _procEv(sm, sm->curr_st, *evid, ev_data);

    tSmEv* ev;
    while (ev = queue_get(&sm->evqueue))
    {
        _procEv(sm, sm->curr_st, ev->evid, ev->ev_data);
        free(ev);
    }

    sm->is_busy = 0;
    return SM_OK;
}
