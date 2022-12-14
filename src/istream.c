#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <conf/istream.h>
#include <dtc/istream.h>

status dtc_istream_init(FILE *fistream, dtc_istream **out_stream)
{
    DTC_ASSERT_PARAM_PTR_VALID(fistream);
    DTC_ASSERT_PARAM_PTR_VALID(out_stream);

    status status_sub = 0;

    dtc_istream *istream = calloc(1, sizeof(dtc_istream));

    DTC_ASSERT_ALLOC_VALID(istream);

    istream->fistream = fistream;

    istream->head.head_pos = 0;
    istream->head.line = 1;
    istream->head.colm = 0;

    dtc_base_init_param base_params;

    base_params.name ="dtc_istream";
    base_params.f_init = (dtc_type_f_init) dtc_istream_init;
    base_params.f_copy = (dtc_type_f_copy) dtc_istream_copy;
    base_params.f_fini = (dtc_type_f_fini) dtc_istream_fini;

    DTC_CALL(status_sub, dtc_base_init(&base_params, &istream->base))
    {
        free(istream);
        return status_sub;
    }

    DTC_CALL(status_sub, dtc_list_init(0, &istream->saved_heads))
    {
        free(istream);
        return status_sub;
    }

    DTC_CALL(status_sub, dtc_str_init(0, &istream->data))
    {
        dtc_list_fini(istream->saved_heads);
        free(istream);
        return status_sub;
    }

    istream->top_mode = 0;
    istream->state = 0;

    *out_stream = istream;

    return DTC_STATUS_SUCCESS;
}
status dtc_istream_copy(dtc_istream *src, dtc_istream **out_stream)
{
    // TODO: Implement stream copy
    return DTC_STATUS_NIMPL;
}
status dtc_istream_fini(dtc_istream *istream)
{
    DTC_ASSERT_PARAM_PTR_VALID(istream);

    dtc_node *cnode;

    dtc_list_fini(istream->saved_heads);
    dtc_str_fini(istream->data);

    cnode = istream->top_mode;

    while(cnode)
    {
        dtc_node *nnode = cnode->next;
        dtc_node_fini(cnode);
        cnode = nnode;
    }

    free(istream);
    return DTC_STATUS_SUCCESS;
}

status dtc_istream_head_slot_init(
    dtc_istream *istream,
    dtc_istream_head **out_head
)
{
    DTC_ASSERT_PARAM_PTR_VALID(istream);
    DTC_ASSERT_PARAM_PTR_VALID(out_head);

    status status_sub = 0;
    size_t saved_heads_count;
    dtc_istream_head **saved_heads;

    dtc_list_arr(istream->saved_heads, (void ***)&saved_heads);
    dtc_list_len(istream->saved_heads, &saved_heads_count);

    for(size_t i = 0; i < saved_heads_count; ++i)
    {
        if(!saved_heads[i])
        {
            dtc_istream_head *nhead = calloc(1, sizeof(dtc_istream_head));
            DTC_ASSERT_ALLOC_VALID(nhead);
            nhead->head_pos = 0; // used
            saved_heads[i] = nhead;
            *out_head = nhead;
            return DTC_STATUS_SUCCESS;
        }
        if(saved_heads[i]->head_pos < 0)
        {
            saved_heads[i]->head_pos = 0;
            *out_head = saved_heads[i];
            return DTC_STATUS_SUCCESS;
        }
    }

    // We need to append the list of head pointers
    dtc_istream_head **nhptr;
    DTC_CALL(
        status_sub,
        dtc_list_append(istream->saved_heads, (void ***) &nhptr)
    )
        return DTC_STATUS_SUCCESS;

    dtc_istream_head *nhead = calloc(1, sizeof(dtc_istream_head));
    DTC_ASSERT_ALLOC_VALID(nhead);
    nhead->head_pos = 0; // used
    *nhptr = nhead;
    *out_head = nhead;

    return DTC_STATUS_SUCCESS;
}
status dtc_istream_head_slot_fini(
    dtc_istream_head *head
)
{
    DTC_ASSERT_PARAM_PTR_VALID(head);

    head->head_pos = -1;
    return DTC_STATUS_SUCCESS;
}
status dtc_istream_head_slot_save(
    dtc_istream *istream,
    dtc_istream_head *head
)
{
    DTC_ASSERT_PARAM_PTR_VALID(istream);
    DTC_ASSERT_PARAM_PTR_VALID(head);

    *head = istream->head;
    return DTC_STATUS_SUCCESS;
}
status dtc_istream_head_slot_load(
    dtc_istream *istream,
    dtc_istream_head *head
)
{
    DTC_ASSERT_PARAM_PTR_VALID(istream);
    DTC_ASSERT_PARAM_PTR_VALID(head);

    istream->head = *head;
    return DTC_STATUS_SUCCESS;
}

status dtc_istream_readc(dtc_istream *istream, int *nout_c)
{
    DTC_ASSERT_PARAM_PTR_VALID(istream);

    status status_sub = 0;
    while(1)
    {
        size_t readlen;
        DTC_CALL(status_sub, dtc_str_len(istream->data, &readlen))
            return status_sub;

        if(istream->head.head_pos >= readlen)
        {
            DTC_CALL(status_sub, dtc_istream_read_more(istream))
                return status_sub;
        }

        DTC_CALL(status_sub, dtc_str_len(istream->data, &readlen))
            return status_sub;

        int c;

        /* If readlen didn't change, then it means EOF was reached */
        if(istream->head.head_pos >= readlen)
            c = EOF;
        else
        {
            dtc_str_getc(istream->data, istream->head.head_pos, &c);
            ++istream->head.head_pos;
            if(c == '\n')
            {
                istream->head.colm = 0;
                ++istream->head.line;
            }
            else
                ++istream->head.colm;
        }

        if(istream->top_mode)
        {
            dtc_istream_f_mode f_mode = istream->top_mode->obj;
            if(f_mode)
            {
                int c2 = c;
                status s = f_mode(istream, istream->state, c, &c2);

                if (s == DTC_STATUS_ISTREAM_FMODE_SKIP)
                    continue;

                c = c2;
            }
        }

        DTC_SET_OUT(nout_c, c);

        if(c == EOF)
            return DTC_STATUS_ISTREAM_READC_EOF;
        return DTC_STATUS_SUCCESS;
    }
}
status dtc_istream_rawc(dtc_istream *istream, int *nout_c)
{
    DTC_ASSERT_PARAM_PTR_VALID(istream);

    status status_sub = 0;

    /* To not repeat the same code in readc
       I thought of the stupid idea of locally storing
       istream->top_mode and setting it to null, call readc, then
       reset top_mode to its original value */

    dtc_node *top_mode = istream->top_mode;

    istream->top_mode = 0;

    /* We don't need to do anything special in case of error,
       We do the same thing in case of error or not */
    DTC_CALL(status_sub, dtc_istream_readc(istream, nout_c));

    istream->top_mode = top_mode;
    return status_sub;
}

status dtc_istream_ghead(dtc_istream *stream, dtc_istream_head *out_head)
{
    DTC_ASSERT_PARAM_PTR_VALID(stream);
    DTC_ASSERT_PARAM_PTR_VALID(out_head);

    *out_head = stream->head;

    return DTC_STATUS_SUCCESS;
}
status dtc_istream_shead(dtc_istream *stream, dtc_istream_head *head)
{
    DTC_ASSERT_PARAM_PTR_VALID(stream);
    DTC_ASSERT_PARAM_PTR_VALID(head);

    stream->head = *head;

    return DTC_STATUS_SUCCESS;
}
status dtc_istream_file(dtc_istream *stream, FILE **out_fistream)
{
    DTC_ASSERT_PARAM_PTR_VALID(stream);
    DTC_ASSERT_PARAM_PTR_VALID(out_fistream);

    *out_fistream = stream->fistream;

    return DTC_STATUS_SUCCESS;
}
status dtc_istream_data(dtc_istream *istream, dtc_str **out_data)
{
    DTC_ASSERT_PARAM_PTR_VALID(istream);
    DTC_ASSERT_PARAM_PTR_VALID(out_data);

    *out_data = istream->data;

    return DTC_STATUS_SUCCESS;
}

status dtc_istream_mode_push(dtc_istream *istream, dtc_istream_f_mode f_mode)
{
    DTC_ASSERT_PARAM_PTR_VALID(istream);

    status status_sub = 0;

    dtc_node *ntop_mode;
    dtc_node_init_param nparam;

    nparam.n_prev = 0;
    nparam.n_next = istream->top_mode;
    nparam.n_obj = f_mode;

    DTC_CALL(status_sub, dtc_node_init(&nparam, &ntop_mode))
        return status_sub;

    istream->top_mode = ntop_mode;

    return DTC_STATUS_SUCCESS;
}
status dtc_istream_mode_pop(dtc_istream *istream)
{
    DTC_ASSERT_PARAM_PTR_VALID(istream);
    DTC_ASSERT_PARAM_PTR_VALID(istream->top_mode);

    status status_sub = 0;

    dtc_node *otop_mode = istream->top_mode;

    istream->top_mode = otop_mode->next;

    DTC_CALL(status_sub, dtc_node_fini(otop_mode))
        return status_sub;

    return DTC_STATUS_SUCCESS;
}

status dtc_istream_sstate(dtc_istream *istream, dtc_istream_state *state)
{
    DTC_ASSERT_PARAM_PTR_VALID(istream);
    DTC_ASSERT_PARAM_PTR_VALID(state);

    istream->state = state;

    return DTC_STATUS_SUCCESS;
}

status dtc_istream_read_more(dtc_istream *istream)
{
    DTC_ASSERT_PARAM_PTR_VALID(istream);

    status status_sub = 0;
    char buf[DTC_ISTREAM_READ_MORE_MAX_LEN + 1];

    memset(buf, 0, DTC_ISTREAM_READ_MORE_MAX_LEN + 1);

    fread(buf, 1, DTC_ISTREAM_READ_MORE_MAX_LEN, istream->fistream);

    DTC_CALL(status_sub, dtc_str_appends(istream->data, buf))
        return status_sub;

    DTC_ASSERT_IO_FNOERROR(istream->fistream);

    return DTC_STATUS_SUCCESS;
}
