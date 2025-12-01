.code32
.global switch_task
.global start_task

#
# cpu_state_t layout:
# 0   eax
# 4   ebx
# 8   ecx
# 12  edx
# 16  esi
# 20  edi
# 24  ebp
# 28  esp
# 32  eip
# 36  eflags
# 40  cs
# 44  ds
# 48  es
# 52  fs
# 56  gs
# 60  ss
#

switch_task:
    #
    # --- SAVE CONTEXT ---
    #

    .byte 0x9C        # pushfd
    .byte 0x60        # pushad

    # stack after pushad+pushfd:
    #
    # 0  edi
    # 4  esi
    # 8  ebp
    # 12 saved esp
    # 16 ebx
    # 20 edx
    # 24 ecx
    # 28 eax
    # 32 eflags
    # 36 retaddr
    # 40 old*
    # 44 new*

    mov 40(%esp), %eax       # eax = old

    # GP registers
    mov 28(%esp), %edx       # eax
    mov %edx, 0(%eax)

    mov 16(%esp), %edx       # ebx
    mov %edx, 4(%eax)

    mov 24(%esp), %edx       # ecx
    mov %edx, 8(%eax)

    mov 20(%esp), %edx       # edx
    mov %edx, 12(%eax)

    mov 4(%esp), %edx        # esi
    mov %edx, 16(%eax)

    mov 0(%esp), %edx        # edi
    mov %edx, 20(%eax)

    mov 8(%esp), %edx        # ebp
    mov %edx, 24(%eax)

    # esp after returning from switch_task:
    lea 40(%esp), %edx
    mov %edx, 28(%eax)

    # eip (return address)
    mov 36(%esp), %edx
    mov %edx, 32(%eax)

    # eflags
    mov 32(%esp), %edx
    mov %edx, 36(%eax)

    # segment selectors
    mov %cs, %dx
    mov %edx, 40(%eax)

    mov %ds, %dx
    mov %edx, 44(%eax)

    mov %es, %dx
    mov %edx, 48(%eax)

    mov %fs, %dx
    mov %edx, 52(%eax)

    mov %gs, %dx
    mov %edx, 56(%eax)

    mov %ss, %dx
    mov %edx, 60(%eax)

    #
    # --- LOAD NEW CONTEXT ---
    #

    mov 44(%esp), %ecx       # ecx = new

    mov 0(%ecx), %eax
    mov 4(%ecx), %ebx
    mov 8(%ecx), %edx
    mov 12(%ecx), %edx       # overwrite ecx later
    mov 16(%ecx), %esi
    mov 20(%ecx), %edi
    mov 24(%ecx), %ebp

    # segment registers
    mov 44(%ecx), %dx
    mov %dx, %ds

    mov 48(%ecx), %dx
    mov %dx, %es

    mov 52(%ecx), %dx
    mov %dx, %fs

    mov 56(%ecx), %dx
    mov %dx, %gs

    mov 60(%ecx), %dx
    mov %dx, %ss

    # load ESP
    mov 28(%ecx), %esp

    # popfd (restore flags)
    pushl 36(%ecx)
    .byte 0x9D              # popfd

    # GO!
    jmp *32(%ecx)


#
# START FIRST TASK
# void start_task(uint32_t esp, uint32_t eip)
#
start_task:
    mov 4(%esp), %eax
    mov 8(%esp), %ebx
    mov %eax, %esp
    jmp *%ebx
