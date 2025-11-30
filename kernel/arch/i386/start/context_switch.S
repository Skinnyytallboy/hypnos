.global start_task
.global switch_task

# void start_task(uint32_t esp, uint32_t eip);
start_task:
    # [esp+4] = esp, [esp+8] = eip
    mov 4(%esp), %eax       # new esp
    mov 8(%esp), %ebx       # new eip
    mov %eax, %esp
    jmp *%ebx               

# void switch_task(cpu_state_t *old, cpu_state_t *new);
# struct cpu_state { uint32_t esp; uint32_t ebp; uint32_t eip; };
switch_task:
    # [esp+4] = old
    # [esp+8] = new

    # save caller's context into *old
    mov 4(%esp), %eax       # eax = old
    mov %esp, 0(%eax)       # old->esp = current esp
    mov %ebp, 4(%eax)       # old->ebp = current ebp
    mov 0(%esp), %ecx       # return address (old EIP)
    mov %ecx, 8(%eax)       # old->eip = ret addr

    # load *new into registers and jump
    mov 8(%esp), %edx       # edx = new
    mov 0(%edx), %esp       # esp = new->esp
    mov 4(%edx), %ebp       # ebp = new->ebp
    mov 8(%edx), %ecx       # ecx = new->eip
    jmp *%ecx
