/**************************************************************************
 * C S 429 system emulator
 * 
 * Bubble and stall checking logic.
 * STUDENT TO-DO:
 * Implement logic for hazard handling.
 * 
 * handle_hazards is called from proc.c with the appropriate
 * parameters already set, you must implement the logic for it.
 * 
 * You may optionally use the other three helper functions to 
 * make it easier to follow your logic.
 **************************************************************************/ 

#include "machine.h"
#include "hazard_control.h"

extern machine_t guest;
extern mem_status_t dmem_status;

/* Use this method to actually bubble/stall a pipeline stage.
 * Call it in handle_hazards(). Do not modify this code. */
void pipe_control_stage(proc_stage_t stage, bool bubble, bool stall) {
    pipe_reg_t *pipe;
    switch(stage) {
        case S_FETCH: pipe = F_instr; break;
        case S_DECODE: pipe = D_instr; break;
        case S_EXECUTE: pipe = X_instr; break;
        case S_MEMORY: pipe = M_instr; break;
        case S_WBACK: pipe = W_instr; break;
        default: printf("Error: incorrect stage provided to pipe control.\n"); return;
    }
    if (bubble && stall) {
        printf("Error: cannot bubble and stall at the same time.\n");
        pipe->ctl = P_ERROR;
    }
    // If we were previously in an error state, stay there.
    if (pipe->ctl == P_ERROR) return;

    if (bubble) {
        pipe->ctl = P_BUBBLE;
    }
    else if (stall) {
        pipe->ctl = P_STALL;
    }
    else { 
        pipe->ctl = P_LOAD;
    }
}

bool check_ret_hazard(opcode_t D_opcode) {
    if (D_opcode == OP_RET || D_opcode == OP_BR || D_opcode == OP_BLR) {
        F_in->status = STAT_AOK;
        F_out->status = STAT_AOK;
        return true; // A return hazard is detected
    }
    return false; // No return hazard
}

bool check_mispred_branch_hazard(opcode_t X_opcode, bool X_condval) {
    if ((X_opcode == OP_B_COND || X_opcode == OP_CBZ || X_opcode == OP_CBNZ) && !X_condval) {
        return true; // A mispredicted branch hazard is detected
    }
    return false; // No mispredicted branch hazard
}

bool check_load_use_hazard(opcode_t D_opcode, uint8_t D_src1, uint8_t D_src2,
                           opcode_t X_opcode, uint8_t X_dst) {
    // Check if the current instruction in the decode stage is a load instruction
    // and if it requires data from the execute stage (dependency)
    if ((X_opcode == OP_LDUR) && (D_src1 == X_dst || D_src2 == X_dst)) {
        return true; // A load-use hazard is detected
    }
    return false; // No load-use hazard
}

comb_logic_t handle_hazards(opcode_t D_opcode, uint8_t D_src1, uint8_t D_src2, uint64_t D_val_a, 
                            opcode_t X_opcode, uint8_t X_dst, bool X_condval) {
    /* Students: Change this code */
    // This will need to be updated in week 2, good enough for week 1

    //exceptions first, if theres bad memory, stall it and everything before
    //then memory, then other hazards
    if (dmem_status == ERROR) {
        pipe_control_stage(S_FETCH, false, true);
        pipe_control_stage(S_DECODE, false, true);
        pipe_control_stage(S_EXECUTE, false, true);
        pipe_control_stage(S_MEMORY, false, true);
         pipe_control_stage(S_WBACK, false, false);
    } else {
        bool ret_haz = check_ret_hazard(D_opcode);
        bool load_haz = check_load_use_hazard(D_opcode, D_src1, D_src2, X_opcode, X_dst);
        bool branch_haz = check_mispred_branch_hazard(X_opcode, X_condval);
        bool mem_haz = (dmem_status == IN_FLIGHT);
        //backwards
        pipe_control_stage(S_WBACK, false, false);
        pipe_control_stage(S_MEMORY, false, mem_haz);
        if ((branch_haz || load_haz) && mem_haz) {
            pipe_control_stage(S_EXECUTE, false,  mem_haz);
        } else {
            pipe_control_stage(S_EXECUTE, (branch_haz || load_haz),  mem_haz);
        }
        if ((branch_haz || ret_haz) && (load_haz || mem_haz)) {
             pipe_control_stage(S_DECODE, false, load_haz || mem_haz);
        } else {
            pipe_control_stage(S_DECODE, (branch_haz || ret_haz), load_haz || mem_haz);
        }
        pipe_control_stage(S_FETCH, false, (load_haz || ret_haz || mem_haz));
    }



    
    

}


