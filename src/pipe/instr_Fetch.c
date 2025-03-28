/**************************************************************************
 * C S 429 system emulator
 *
 * instr_Fetch.c - Fetch stage of instruction processing pipeline.
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

extern uint64_t F_PC;

/*
 * Select PC logic.
 * STUDENT TO-DO:
 * Write the next PC to *current_PC.
 */

static comb_logic_t
select_PC(uint64_t pred_PC,                                       // The predicted PC
          opcode_t D_opcode, uint64_t val_a, uint64_t D_seq_succ, // Possible correction from RET
          opcode_t M_opcode, bool M_cond_val, uint64_t seq_succ,  // Possible correction from B.cond
          uint64_t *current_PC)
{
    /*
     * Students: Please leave this code
     * at the top of this function.
     * You may modify below it.
     */
    if (D_opcode == OP_RET && val_a == RET_FROM_MAIN_ADDR)
    {
        *current_PC = 0; // PC can't be 0 normally.
        return;
    }
    // Modify starting here.
    if (D_opcode == OP_RET || D_opcode == OP_BR || D_opcode == OP_BLR)
    {
        // return
        *current_PC = val_a;
    }
    else if ((M_opcode == OP_B_COND || M_opcode == OP_CBZ || M_opcode == OP_CBNZ) && M_cond_val == false)
    {
        *current_PC = seq_succ;
    }
    else if ((M_opcode == OP_B_COND || M_opcode == OP_CBZ || M_opcode == OP_CBNZ) && M_cond_val == true)
    {
        *current_PC = pred_PC;
    }
    else
    {
        // base case
        *current_PC = pred_PC;
    }
    return;
}

/*
 * Predict PC logic. Conditional branches are predicted taken.
 * STUDENT TO-DO:
 * Write the predicted next PC to *predicted_PC
 * and the next sequential pc to *seq_succ.
 */

static comb_logic_t
predict_PC(uint64_t current_PC, uint32_t insnbits, opcode_t op,
           uint64_t *predicted_PC, uint64_t *seq_succ)
{
    /*
     * Students: Please leave this code
     * at the top of this function.
     * You may modify below it.
     */
    if (!current_PC)
    {
        return; // We use this to generate a halt instruction.
    }
    // Modify starting here.
    // need base case, unconditional branch, conditional branch
    if (op == OP_B || op == OP_BL)
    {
        // unconditional branches
        *predicted_PC = current_PC + bitfield_s64(insnbits, 0, 26) * 4;
        *seq_succ = current_PC + 4;
        
    } else if (op == OP_BLR) {
        *predicted_PC = current_PC + 4;
        *seq_succ = current_PC + 4;
        if (op == OP_BLR) {
            guest.proc->GPR[30] = *seq_succ;
        }
    }
    else if (op == OP_B_COND)
    {
        *predicted_PC = current_PC + bitfield_s64(insnbits, 5, 19) * 4;
        *seq_succ = current_PC + 4;
    } else if (op == OP_CBNZ || op == OP_CBZ) {
        *predicted_PC = current_PC + bitfield_s64(insnbits, 5, 19) * 4;
        *seq_succ = current_PC + 4;
    } 
    else {
        // base, return also goes here; gets corrected in select_PC()
        *predicted_PC = current_PC + 4;
        *seq_succ = current_PC + 4;
    }
    return;
}

/*
 * Helper function to recognize the aliased instructions:
 * LSL, LSR, CMP, and TST. We do this only to simplify the
 * implementations of the shift operations (rather than having
 * to implement UBFM in full).
 * STUDENT TO-DO
 */

static void fix_instr_aliases(uint32_t insnbits, opcode_t *op)
{
    if (*op == OP_UBFM)
    {
        if (bitfield_u32(insnbits, 10, 6) == 0b111111)
        {
            // lsr
            *op = OP_LSR;
        } else
        {
            // lsl
            *op = OP_LSL;
        }
    }
    else if (*op == OP_SUBS_RR)
    {
        *op = OP_CMP_RR;
    }
    else if (*op == OP_ANDS_RR)
    {
        *op = OP_TST_RR;
    } else if (*op == OP_CSEL) {
        if (bitfield_u32(insnbits, 10, 2) == 0) {
            *op == OP_CSEL;
        } else {
            *op = OP_CSINC;
        }
    } else if (*op == OP_CSNEG) {
        if (bitfield_u32(insnbits, 10, 2) == 0) {
            *op = OP_CSINV;
        } else {
            *op = OP_CSNEG;
        }
    }
    return;
}

/*
 * Fetch stage logic.
 * STUDENT TO-DO:
 * Implement the fetch stage.
 *
 * Use in as the input pipeline register,
 * and update the out pipeline register as output.
 * Additionally, update F_PC for the next
 * cycle's predicted PC.
 *
 * You will also need the following helper functions:
 * select_pc, predict_pc, and imem.
 */

comb_logic_t fetch_instr(f_instr_impl_t *in, d_instr_impl_t *out)
{
    bool imem_err = 0;
    uint64_t current_PC;
    select_PC(in->pred_PC, X_out->op, X_out->val_a, X_out->seq_succ_PC,
              M_out->op, M_out->cond_holds, M_out->seq_succ_PC,
              &current_PC);
    /*
     * Students: This case is for generating HLT instructions
     * to stop the pipeline. Only write your code in the **else** case.
     */
    if (!current_PC || F_in->status == STAT_HLT)
    {
        out->insnbits = 0xD4400000U;
        out->op = OP_HLT;
        out->print_op = OP_HLT;
        imem_err = false;
    }
    else
    {
        // Students modify from here;
        imem(current_PC, &out->insnbits, &imem_err);
        out->op = itable[bitfield_u32(out->insnbits, 21, 11)];
        fix_instr_aliases(out->insnbits, &out->op);
        out->print_op = out->op;
        out->this_PC = current_PC;
        if (in->status != STAT_INS){
            predict_PC(current_PC, out->insnbits, out->op, &F_PC, &out->seq_succ_PC);
        } else { 
            F_PC = current_PC + 1;
        }
        // do adrp
        if (out->op == OP_ADRP)
        {
            // do adrp
            // out->adrp_val = bitfield_s64(out->insnbits, 5, 19);
            out->adrp_val = current_PC & 0xFFFFF000;
        }
        else
        {
            out->adrp_val = 0;
        }
    }
    if (imem_err || out->op == OP_ERROR)
    {
        in->status = STAT_INS;
        F_in->status = in->status;
    }
    else if (out->op == OP_HLT)
    {
        in->status = STAT_HLT;
        F_in->status = in->status;
    }
    else
    {
        in->status = STAT_AOK;
    }
    out->status = in->status;
    return;
}