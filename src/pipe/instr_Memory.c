/**************************************************************************
 * C S 429 system emulator
 * 
 * instr_Memory.c - Memory stage of instruction processing pipeline.
 **************************************************************************/ 

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "err_handler.h"
#include "instr.h"
#include "instr_pipeline.h"
#include "machine.h"
#include "hw_elts.h"

extern machine_t guest;
extern mem_status_t dmem_status;

extern comb_logic_t copy_w_ctl_sigs(w_ctl_sigs_t *, w_ctl_sigs_t *);

/*
 * Memory stage logic.
 * STUDENT TO-DO:
 * Implement the memory stage.
 * 
 * Use in as the input pipeline register,
 * and update the out pipeline register as output.
 * 
 * You will need the following helper functions:
 * copy_w_ctl_signals and dmem.
 */

comb_logic_t memory_instr(m_instr_impl_t *in, w_instr_impl_t *out) {
    copy_w_ctl_sigs(&out->W_sigs, &in->W_sigs);
    out->dst = in->dst;
    out->op = in->op;
    out->print_op = in->print_op;
    out->status = in->status;
    out->val_ex = in->val_ex;
    out->val_b = in->val_b;
    bool dmem_err = false;
    if (in->M_sigs.dmem_read == 1) {
        //read
        dmem(in->val_ex, 0, 1, 0, &out->val_mem, &dmem_err);
        printf("dmemstatus %d", dmem_status);
    } else if (in->M_sigs.dmem_write) {
        //write
        dmem(in->val_ex, in->val_b, 0, 1, NULL, &dmem_err);
        printf("dmemstatus %d", dmem_status);
    }
    if((out-> op == OP_STUR || out-> op == OP_LDUR) && dmem_err){ //dmem_status is dmem_err --> this is kinda weird, but ok
        out->status = STAT_ADR;
    }
    //dmem(in->val_ex, in->val_b, in->M_sigs.dmem_read, in->M_sigs.dmem_write, &out->val_mem, &dmem_status);
    return;
}