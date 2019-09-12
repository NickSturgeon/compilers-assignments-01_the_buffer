#include <stdio.h>
#include "buffer.h"

Buffer * b_allocate (short init_capacity, char inc_factor, char o_mode) {
    Buffer * buffer;
    short capacity = (init_capacity == 0) ? DEFAULT_INIT_CAPACITY : init_capacity;

    /* begin guards */
    /* invalid init_capacity; capacity must be >= 0 and < max value of short */
    if (init_capacity < 0 || init_capacity > MAX_CAPACITY)
        return NULL;
    /* invalid o_mode; operational mode must be 'a', 'm', or 'f' */
    if (o_mode != MODE_ADD && o_mode != MODE_MUL && o_mode != MODE_FIX)
        return NULL;
    /* invalid inc_factor for 'm'; must not be greater than 100 */
    if (o_mode == MODE_MUL && inc_factor > 100)
        return NULL;
    /* end guards */

    buffer = calloc(1, sizeof(Buffer));
    if (buffer == NULL)
        return NULL;
    buffer->cb_head = malloc(capacity);

    if (init_capacity == 0) {
        buffer->inc_factor = (o_mode == MODE_FIX) ? 0 : DEFAULT_INC_FACTOR;
        switch (o_mode) {
            case MODE_FIX:
                buffer->mode = B_MODE_FIX;
                break;
            case MODE_ADD:
                buffer->mode = B_MODE_ADD;
                break;
            case MODE_MUL:
                buffer->mode = B_MODE_MUL;
                break;
            default: /* should never reach due to guard */
                printf("Unsupported mode: %c\n", o_mode);
                free(buffer->cb_head);
                free(buffer);
                return NULL;
        }
    } else if (o_mode == MODE_FIX || inc_factor == 0) {
        buffer->mode = B_MODE_FIX;
        buffer->inc_factor = 0;
    } else if (o_mode == MODE_ADD) {
        buffer->mode = B_MODE_ADD;
        buffer->inc_factor = inc_factor;
    } else if (o_mode == MODE_MUL && inc_factor <= 100) {
        buffer->mode = B_MODE_MUL;
        buffer->inc_factor = inc_factor;
    } else {
        free(buffer->cb_head);
        free(buffer);
        return NULL;
    }

    buffer->capacity = capacity;
    buffer->flags = DEFAULT_FLAGS;

    return buffer;
}

pBuffer b_addc (pBuffer const pBD, char symbol) {
    long new_capacity;
    char * new_mem_loc;

    if (pBD == NULL)
        return NULL;

    pBD->flags &= RESET_R_FLAG;

    if (pBD->addc_offset < pBD->capacity) {
        *(pBD->cb_head + pBD->addc_offset++) = symbol;
        return pBD;
    }

    if (pBD->mode == B_MODE_FIX)
        return NULL;

    if (pBD->mode == B_MODE_ADD) {
        new_capacity = pBD->capacity + pBD->inc_factor;
        if (new_capacity < 0)
            return NULL;
    } else if (pBD->mode == B_MODE_MUL) {
        short available_space = MAX_CAPACITY - pBD->capacity;
        if (available_space == 0)
            return NULL;

        new_capacity = pBD->capacity +
                (available_space * (pBD->inc_factor / 100.0));
    } else {
        printf("Unsupported mode: %d\n", pBD->mode);
        return NULL;
    }

    if (new_capacity <= MAX_CAPACITY) {
        pBD->capacity = (short)new_capacity;
    } else {
        pBD->capacity = MAX_CAPACITY;
    }

    new_mem_loc = realloc(pBD->cb_head, pBD->capacity);
    if (new_mem_loc == NULL) {
        return NULL;
    }

    if (pBD->cb_head != new_mem_loc)
        pBD->flags |= SET_R_FLAG;

    pBD->cb_head = new_mem_loc;
    *(pBD->cb_head + pBD->addc_offset++) = symbol;

    return pBD;
}

int b_clear (Buffer * const pBD) {
    if (pBD == NULL)
        return RT_FAIL_1;

    pBD->addc_offset = 0;
    pBD->getc_offset = 0;
    pBD->markc_offset = 0;

    return 0;
}

void b_free (Buffer * const pBD) {
    if (pBD != NULL) {
        free(pBD->cb_head);
        free(pBD);
    }
}

int b_isfull (Buffer * const pBD) {
    if (pBD == NULL)
        return RT_FAIL_1;

    if (pBD->addc_offset == pBD->capacity)
        return 1;

    return 0;
}

short b_limit (Buffer * const pBD) {
    if (pBD == NULL)
        return RT_FAIL_1;

    return pBD->addc_offset;
}

short b_capacity (Buffer * const pBD) {
    if (pBD == NULL)
        return RT_FAIL_1;

    return pBD->capacity;
}

short b_mark (pBuffer const pBD, short mark) {
    if (pBD == NULL)
        return RT_FAIL_1;

    if (mark < 0 || mark > pBD->addc_offset)
        return RT_FAIL_1;

    pBD->markc_offset = mark;

    return pBD->markc_offset;
}

int b_mode (Buffer * const pBD) {
    if (pBD == NULL)
        return RT_FAIL_2; /* Since -1 is a valid mode */

    return pBD->mode;
}

size_t b_incfactor (Buffer * const pBD) {
    if (pBD == NULL)
        return 0x100;

    if (pBD->inc_factor < 0)
        return 0x100;

    return pBD->inc_factor;
}

int b_load (FILE * const fi, Buffer * const pBD) {
    char c;

    if (fi == NULL || pBD == NULL || pBD->cb_head == NULL)
        return RT_FAIL_1;

    while (!feof(fi)) {
        if ((c = getc(fi)) != EOF) {
            if (b_addc(pBD, c) == NULL) {
                ungetc(c, fi);
                return LOAD_FAIL;
            }
        }
    }

    return 0;
}

int b_isempty (Buffer * const pBD) {
    if (pBD == NULL)
        return RT_FAIL_1;

    return pBD->addc_offset == 0;
}

char b_getc (Buffer * const pBD) {
    if (pBD == NULL || pBD->cb_head == NULL)
        return RT_FAIL_2;

    if (pBD->getc_offset == pBD->addc_offset) {
        pBD->flags |= SET_EOB;
        return 0;
    } else {
        pBD->flags &= RESET_EOB;
    }

    return *(pBD->cb_head + pBD->getc_offset++);
}

int b_eob (Buffer * const pBD) {
    if (pBD == NULL)
        return RT_FAIL_1;

    return (pBD->flags & CHECK_EOB) >> 1;
}

int b_print (Buffer * const pBD, char nl) {
    int count = 0;
    short cur_offset;

    if (pBD == NULL || pBD->cb_head == NULL)
        return RT_FAIL_1;

    cur_offset = pBD->getc_offset;

    pBD->getc_offset = 0;
    for (;;) {
        char c = b_getc(pBD);
        if (!b_eob(pBD))
            printf("%c", c);
        else
            break;
        count++;
    }
    if (nl != 0) {
        printf("\n");
    }

    pBD->getc_offset = cur_offset;

    return count;
}

Buffer * b_compact(Buffer * const pBD, char symbol) {
    char * new_mem_loc;

    if (pBD == NULL || pBD->cb_head == NULL)
        return NULL;

    pBD->flags &= RESET_R_FLAG;

    new_mem_loc = realloc(pBD->cb_head, pBD->addc_offset + 1);
    if (new_mem_loc == NULL) {
        return NULL;
    }

    if (pBD->cb_head != new_mem_loc)
        pBD->flags |= SET_R_FLAG;

    pBD->cb_head = new_mem_loc;
    pBD->capacity = pBD->addc_offset + 1;

    *(pBD->cb_head + pBD->addc_offset++) = symbol;

    return pBD;
}

int b_rewind(Buffer * const pBD) {
    if (pBD == NULL)
        return RT_FAIL_1;

    pBD->getc_offset = 0;
    pBD->markc_offset = 0;

    return 0;
}
