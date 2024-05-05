;global process_context_switch

; Load struct Context (CPU GP-register + CR3) then jump
; Function Signature: void process_context_switch(struct Context ctx);
;process_context_switch:
    ; Using iret (return instruction for interrupt) technique for privilege change
 ;   lea  ecx, [esp+0x04] ; Save the base address for struct Context ctx
;    ...