/**************************************************************************
 * C S 429 system emulator
 * 
 * instr_Execute.c - Execute stage of instruction processing pipeline.
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

extern bool X_condval;

extern comb_logic_t copy_m_ctl_sigs(m_ctl_sigs_t *, m_ctl_sigs_t *);
extern comb_logic_t copy_w_ctl_sigs(w_ctl_sigs_t *, w_ctl_sigs_t *);

/*
 * Execute stage logic.
 * STUDENT TO-DO:
 * Implement the execute stage.
 * 
 * Use in as the input pipeline register,
 * and update the out pipeline register as output.
 * 
 * You will need the following helper functions:
 * copy_m_ctl_signals, copy_w_ctl_signals, and alu.
 */

comb_logic_t execute_instr(x_instr_impl_t *in, m_instr_impl_t *out) {
    copy_m_ctl_sigs(&out->M_sigs, &in->M_sigs);
    copy_w_ctl_sigs(&out->W_sigs, &in->W_sigs);


        if (in->X_sigs.valb_sel == 1) {
            //read from register
            alu(in->val_a, in->val_b, in->val_hw, in->ALU_op, in->X_sigs.set_CC, in->cond, &out->val_ex, &out->cond_holds, &guest.proc->NZCV);
        } else {
            //do value from register and immediate
            if (in->op == OP_MOVK) {
                uint64_t prev = guest.proc->GPR[in->dst];
                uint64_t bit_mask = 0xFFFFFFFFFFFFFFFF;
                bit_mask = bit_mask ^ ((long)0xFFFF << in->val_hw);
                alu(in->val_a & bit_mask, in->val_imm, in->val_hw, in->ALU_op, in->X_sigs.set_CC, in->cond, &out->val_ex, &out->cond_holds, &guest.proc->NZCV);
            } else {
                alu(in->val_a, in->val_imm, in->val_hw, in->ALU_op, in->X_sigs.set_CC, in->cond, &out->val_ex, &out->cond_holds, &guest.proc->NZCV);
                if(in->op == OP_BL){
                    out->val_ex = in->seq_succ_PC;
                    out->dst = 30;
                } else if(in->op == OP_BLR) {
                    out->val_ex = in->seq_succ_PC;
                    out->dst = 30;
                }
            }
        }
    
        

    //copy sigs to next pipeline
    out->op = in->op;
    out->dst = in->dst;
    out->print_op = in->print_op;
    out->seq_succ_PC = in->seq_succ_PC;
    out->status = in->status;
    out->val_b = in->val_b;
    return;
}