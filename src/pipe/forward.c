/**************************************************************************
 * C S 429 system emulator
 * 
 * forward.c - The value forwarding logic in the pipelined processor.
 **************************************************************************/ 

#include <stdbool.h>
#include "forward.h"

/* STUDENT TO-DO:
 * Implement forwarding register values from
 * execute, memory, and writeback back to decode.
 */
comb_logic_t forward_reg(uint8_t D_src1, uint8_t D_src2, uint8_t X_dst, uint8_t M_dst, uint8_t W_dst,
                 uint64_t X_val_ex, uint64_t M_val_ex, uint64_t M_val_mem, uint64_t W_val_ex,
                 uint64_t W_val_mem, bool M_wval_sel, bool W_wval_sel, bool X_w_enable,
                 bool M_w_enable, bool W_w_enable,
                 uint64_t *val_a, uint64_t *val_b) {
    //12.3 slides
    // Check if there's a need for forwarding
    if(D_src1 == X_dst || D_src1 == M_dst || D_src1 == W_dst){
        if ((D_src1 == X_dst && X_w_enable)){
            *val_a = X_val_ex;
        }
        else if ((D_src1 == M_dst && M_w_enable)){
            if(M_wval_sel){
                *val_a = M_val_mem;
            } else {
                *val_a = M_val_ex;
            }
        }
        else if ((D_src1 == W_dst && W_w_enable)){
            if(W_wval_sel){
                *val_a = W_val_mem;
            } else {
                *val_a = W_val_ex;
            }
        }
    } else {
        // No forwarding needed for D_src1
    }

    
    //same thing for val b
    if(D_src2 == X_dst || D_src2 == M_dst || D_src2 == W_dst){
       if ((D_src2 == X_dst && X_w_enable)){
           *val_b = X_val_ex;
       }
       else if ((D_src2 == M_dst && M_w_enable)){
           if(M_wval_sel){
               *val_b = M_val_mem;
           } else {
               *val_b = M_val_ex;
           }
       }
       else if ((D_src2 == W_dst && W_w_enable)){
           if(W_wval_sel){
               *val_b = W_val_mem;
           } else {
               *val_b = W_val_ex;
           }
       }
   } else {
       // No forwarding needed for D_src2
   }

    return;
}
