/**************************************************************************
 * C S 429 system emulator
 * 
 * Student TODO: AE
 * 
 * hw_elts.c - Module for emulating hardware elements.
 * 
 * Copyright (c) 2022, 2023. 
 * Authors: S. Chatterjee, Z. Leeper. 
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/ 

#include <assert.h>
#include "hw_elts.h"
#include "mem.h"
#include "machine.h"
#include "forward.h"
#include "err_handler.h"

extern machine_t guest;

comb_logic_t 
imem(uint64_t imem_addr,
     uint32_t *imem_rval, bool *imem_err) {
    // imem_addr must be in "instruction memory" and a multiple of 4
    *imem_err = (!addr_in_imem(imem_addr) || (imem_addr & 0x3U));
    *imem_rval = (uint32_t) mem_read_I(imem_addr);
}

/*
 * Regfile logic.
 * STUDENT TO-DO:
 * Read from source registers and write to destination registers when enabled.
 */
comb_logic_t
regfile(uint8_t src1, uint8_t src2, uint8_t dst, uint64_t val_w,
        // bool src1_31isSP, bool src2_31isSP, bool dst_31isSP, 
        bool w_enable,
        uint64_t *val_a, uint64_t *val_b) {
        //src1 and src2 are 6 bits each for the register number
        //dst is register to be writen to
        //val_w is the value to write
        //w_enable is if the signal is good to write to idk?
        //*val_a and *val_b are values you get from source registers
        //how to access the nth register --> guest.proc->GPR[n]; additional info in proc.h
        if (src1 <= 30) {
            *val_a = guest.proc->GPR[src1];
        } else if (src1 == 31) {
            *val_a = guest.proc->SP;
        } else {
            *val_a = 0; //for xzr
        }

        if (src2 <= 30) {
            *val_b = guest.proc->GPR[src2];
        } else if (src2 == 31) {
            *val_b = guest.proc->SP;
        } else {
            *val_b = 0; //for xzr
        }
        
        if (w_enable) {
            if (dst <= 30) {
                guest.proc->GPR[dst] = val_w;
            } else if (dst == 31) {
                guest.proc -> SP = val_w;
            } else {
                //do nothing since all other registers are xzr
            }
        }

    
}

/*
 * cond_holds logic.
 * STUDENT TO-DO:
 * Determine if the condition is true or false based on the condition code values
 */
static bool 
cond_holds(cond_t cond, uint8_t ccval) {
    switch (cond) {
    case C_EQ:
        return GET_ZF(ccval);
    case C_NE:
        return !GET_ZF(ccval);
    case C_CS:
        return GET_CF(ccval);
    case C_CC:
        return !GET_CF(ccval);
    case C_MI:
        return GET_NF(ccval);
    case C_PL:
        return !GET_NF(ccval);
    case C_VS:
        return GET_VF(ccval);
    case C_VC:
        return !GET_VF(ccval);
    case C_HI:
        return GET_CF(ccval) && !GET_ZF(ccval);
    case C_LS:
        return !(GET_CF(ccval) && !GET_ZF(ccval));
    case C_GE:
        return GET_NF(ccval) == GET_VF(ccval);
    case C_LT:
        return !(GET_NF(ccval) == GET_VF(ccval));
    case C_GT:
        return GET_ZF(ccval) == 0 && GET_NF(ccval) == GET_VF(ccval);
    case C_LE:
        return !(GET_ZF(ccval) == 0 && GET_NF(ccval) == GET_VF(ccval));
    case C_AL:
    case C_NV:
        return true;
    default:
        // Handle unexpected condition
        return false;
}

}

/*
 * alu logic.
 * STUDENT TO-DO:
 * Compute the result of a bitwise or mathematial operation (all operations in alu_op_t).
 * Additionally, apply hw or compute condition code values as necessary.
 * Finally, compute condition values with cond_holds.
 */
comb_logic_t 
alu(uint64_t alu_vala, uint64_t alu_valb, uint8_t alu_valhw, alu_op_t ALUop, bool set_CC, cond_t cond, 
    uint64_t *val_e, bool *cond_val, uint8_t *nzcv) {
    uint64_t res = 0xFEEDFACEDEADBEEF;  // To make it easier to detect errors.
    switch(ALUop) {
        case PLUS_OP:
            *val_e = alu_vala + (alu_valb << alu_valhw);
            if (set_CC) {
                //set overflow
                uint64_t sign_a = (alu_vala >> 63) & 0x1; // sign bit of alu_vala
                uint64_t sign_b = (alu_valb >> 63) & 0x1; // sign bit of alu_valb
                uint64_t sign_e = (*val_e >> 63) & 0x1;   // sign bit of result

                *nzcv = (sign_a == sign_b && sign_a != sign_e) ? 0b0001 : 0b0000;

                //set carry, 0 for now
                *nzcv = (*val_e < alu_vala) ? (*nzcv | 0b0010) : (*nzcv & 0b1101);

                //*nzcv = *nzcv & 0b1111;
            }
            break;
        case MINUS_OP:
            *val_e = alu_vala - (alu_valb << alu_valhw);

            if (set_CC) {
                //set overflow
                uint64_t sign_a_sub = (alu_vala >> 63) & 0x1; // sign bit of alu_vala
                uint64_t sign_b_sub = (alu_valb >> 63) & 0x1; // sign bit of alu_valb
                uint64_t sign_e_sub = (*val_e >> 63) & 0x1;   // sign bit of result

                *nzcv = (sign_a_sub != sign_b_sub && sign_a_sub != sign_e_sub) ? (0b0001) : (0b0000);

                //set carry
                *nzcv = (alu_vala >= alu_valb) ? (*nzcv | 0b0010) : (*nzcv & 0b1101);
            }
            break;
        case INV_OP:
            *val_e = alu_vala | (~alu_valb);
            break;
        case OR_OP:
            *val_e = alu_vala | alu_valb;
            break;
        case EOR_OP:
            *val_e = alu_vala ^ alu_valb;
            break;
        case AND_OP:
            *val_e = alu_vala & alu_valb;
            break;
        case MOV_OP:
            *val_e = alu_vala | (alu_valb << alu_valhw);
            break;
        case LSL_OP:
            *val_e = alu_vala << (alu_valb & 0x3FUL);
            break;
        case LSR_OP:
            *val_e = alu_vala >> (alu_valb & 0x3FUL);
            break;
        case ASR_OP:
            //NOT DONE YET
            *val_e = alu_vala >> (alu_valb & 0x3FUL);
            break;
        case CSEL_OP:
            if (cond_holds(cond, *nzcv)) {
                *val_e = alu_vala;
            } else {
                *val_e = alu_valb;
            }
            break;
        case CSINV_OP:
            if (cond_holds(cond, *nzcv)) {
                *val_e = alu_vala;
            } else {
                *val_e = ~alu_valb;
            }
            break;
        case CSINC_OP:
            if (cond_holds(cond, *nzcv)) {
                *val_e = alu_vala;
            } else {
                *val_e = alu_valb + 1;
            }
            break;
        case CSNEG_OP:
            if (cond_holds(cond, *nzcv)) {
                *val_e = alu_vala;
            } else {
                *val_e = -1 * alu_valb;
            }
            break;
        case CBZ_OP:
            if (alu_vala == 0) {
                //branch
                *cond_val = true;
            } else {
                //?
                *cond_val = false;
            }
            *val_e = alu_vala;
            break;
        case CBNZ_OP:
            if (alu_vala != 0) {
                //branch
                *cond_val = true;
            } else {
                *cond_val = false;
            }
            *val_e = alu_vala;
            break;
        case PASS_A_OP:
            *val_e = alu_vala;
            break;
    }
    if (set_CC) {
        //zero flag
        *nzcv = (*val_e == 0) ? *nzcv | 0b0100 : *nzcv & 0b1011;
        //negative flag
        if ((*val_e >> 63) == 1) {
            // The value is negative
            *nzcv |= 0b1000; // Set the N flag in nzcv
        }
        //carry flag done in add


        //overflow flag
        
       
    }
    if(ALUop != CBNZ_OP && ALUop != CBZ_OP){
        *cond_val = cond_holds(cond, *nzcv);
    }
}

comb_logic_t 
dmem(uint64_t dmem_addr, uint64_t dmem_wval, bool dmem_read, bool dmem_write, 
     uint64_t *dmem_rval, bool *dmem_err) {
    // dmem_addr must be in "data memory" and a multiple of 8
    *dmem_err = (!addr_in_dmem(dmem_addr) || (dmem_addr & 0x7U));
    if (is_special_addr(dmem_addr)) *dmem_err = false;
    if (dmem_read) *dmem_rval = (uint64_t) mem_read_L(dmem_addr);
    if (dmem_write) mem_write_L(dmem_addr, dmem_wval);
}