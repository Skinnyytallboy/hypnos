.global start_task
.global switch_task

# void start_task(uint32_t new_esp, uint32_t new_ebp, uint32_t new_eip);
# Does not return; jumps directly into the first task.
start_task:
    # args are on the stack:
    # [esp+0] return address
    # [esp+4] new_esp
    # [esp+8] new_ebp
    # [esp+12] new_eip

    mov 4(%esp), %eax      # new_esp
    mov 8(%esp), %ebx      # new_ebp
    mov 12(%esp), %ecx     # new_eip

    mov %eax, %esp         # switch stack
    mov %ebx, %ebp         # switch base pointer
    jmp *%ecx              # jump to first task


# void switch_task(uint32_t* old_esp, uint32_t* old_ebp, uint32_t* old_eip,
#                  uint32_t  new_esp,  uint32_t  new_ebp,  uint32_t  new_eip);
switch_task:
    # Layout:
    # [esp+0]  return address
    # [esp+4]  &old_esp
    # [esp+8]  &old_ebp
    # [esp+12] &old_eip
    # [esp+16] new_esp
    # [esp+20] new_ebp
    # [esp+24] new_eip

    # Load all arguments into registers first
    mov 4(%esp), %eax      # &old_esp
    mov 8(%esp), %edx      # &old_ebp
    mov 12(%esp), %ecx     # &old_eip
    mov 16(%esp), %esi     # new_esp
    mov 20(%esp), %edi     # new_ebp
    mov 24(%esp), %ebx     # new_eip

    # Save old ESP / EBP / EIP into old_* locations
    mov %esp, (%eax)       # *old_esp = current esp
    mov %ebp, (%edx)       # *old_ebp = current ebp
    mov 0(%esp), %eax      # return address = old EIP
    mov %eax, (%ecx)       # *old_eip = return address

    # Load new context
    mov %esi, %esp         # esp = new_esp
    mov %edi, %ebp         # ebp = new_ebp
    jmp *%ebx              # jump to new_eip
