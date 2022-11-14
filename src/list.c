#include <stdlib.h>
#include <string.h>

#include <internal/utils.h>

#include <conf/list.h>
#include <dtc/list.h>

status dtc_list_init(dtc_list **out_list)
{
#ifdef DTC_SAFE_PARAM
    if(!elsize)
        return DTC_STATUS_VAL_INVALID;
    if(!out_list)
        return DTC_STATUS_PTR_NULL;
#endif
    dtc_list *list = calloc(1, sizeof(dtc_list));

#ifdef DTC_SAFE_ALLOC
    if(!list)
        return DTC_STATUS_ALLOC;
#endif

    list->len = 0;
    list->allocptr = DTC_LIST_INIT_ALLOC;
    list->ptrbuf = calloc(list->allocptr, sizeof(void *));
#ifdef DTC_SAFE_ALLOC
    if(!list->ptrbuf)
    {
        free(list);
        return DTC_STATUS_ALLOC;
    }
#endif

    *out_list = list;
    return DTC_STATUS_SUCCESS;
}
status dtc_list_fini(dtc_list *list)
{
#ifdef DTC_SAFE_PARAM
    if(!list)
        return DTC_STATUS_PTR_NULL;
#endif
    free(list->ptrbuf);
    free(list);
    return DTC_STATUS_SUCCESS;
}

status dtc_list_len(dtc_list *list, size_t *out_len)
{
#ifdef DTC_SAFE_PARAM
    if(!list || !out_len)
        return DTC_STATUS_PTR_NULL;
#endif
    *out_len = list->len;
    return DTC_STATUS_SUCCESS;
}
status dtc_list_arr(dtc_list *list, void ***out_arr)
{
#ifdef DTC_SAFE_PARAM
    if(!list || !out_arr)
        return DTC_STATUS_PTR_NULL;
#endif
    *out_arr = list->ptrbuf;
    return DTC_STATUS_SUCCESS;
}

status dtc_list_append(dtc_list *list, void ***nout_el)
{
#ifdef DTC_SAFE_PARAM
    if(!list)
        return DTC_STATUS_PTR_NULL;
#endif

    status status_sub;

    if(list->len >= list->allocptr)
    {
        DTC_CALL(status_sub, dtc_intenal_list_extend(list, 1 + DTC_LIST_INIT_ALLOC))
            return status_sub;
    }
    DTC_SET_OUT(nout_el, list->ptrbuf + list->len);
    ++list->len;
    return DTC_STATUS_SUCCESS;
}
status dtc_list_insert(dtc_list *list, size_t idx, void ***nout_el)
{
#ifdef DTC_SAFE_PARAM
    if(!list)
        return DTC_STATUS_PTR_NULL;
    if(idx > list->len)
        return DTC_STATUS_IDX_INVALID;
#endif

    status status_sub;

    if(list->len >= list->allocptr)
    {
        DTC_CALL(status_sub, dtc_intenal_list_extend(list, 1 + DTC_LIST_INIT_ALLOC))
            return status_sub;
    }

    memmove(list->ptrbuf + idx + 1, list->ptrbuf + idx, (list->len - idx) * sizeof(void *));
    DTC_SET_OUT(nout_el, list->ptrbuf + idx);
    return DTC_STATUS_SUCCESS;
}
status dtc_list_remove(dtc_list *list, size_t idx)
{
#ifdef DTC_SAFE_PARAM
    if(!list)
        return DTC_STATUS_PTR_NULL;
    if(idx >= list->len)
        return DTC_STATUS_IDX_INVALID;
#endif
    memmove(list->ptrbuf + idx, list->ptrbuf + idx + 1, (list->len - idx - 1) * sizeof(void *));
    return DTC_STATUS_SUCCESS;
}
status dtc_list_at(dtc_list *list, size_t idx, void ***out_el)
{
#ifdef DTC_SAFE_PARAM
    if(!list || !out_el)
        return DTC_STATUS_PTR_NULL;
    if(idx >= list->len)
        return DTC_STATUS_IDX_INVALID;
#endif
    *out_el = list->ptrbuf + idx;
    return DTC_STATUS_SUCCESS;
}

status dtc_intenal_list_extend(dtc_list *list, size_t add_allocptr)
{
#ifdef DTC_SAFE_PARAM
    if(!list)
        return DTC_STATUS_PTR_NULL;
    if(!add_allocptr)
        return DTC_STATUS_VAL_INVALID;
#endif
    void **nptrbuf = realloc(list->ptrbuf, (list->allocptr + add_allocptr) * sizeof(void *));
#ifdef DTC_SAFE_ALLOC
    if(!nptrbuf)
        return DTC_STATUS_ALLOC;
#endif
    list->ptrbuf = nptrbuf;
    memset(list->ptrbuf + list->allocptr, 0, add_allocptr * sizeof(void *));
    list->allocptr += add_allocptr;
    return DTC_STATUS_SUCCESS;
}
