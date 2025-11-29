    .global tss_flush

tss_flush:
    # argument: uint32_t tss_selector in [esp+4]
    mov 4(%esp), %ax
    ltr %ax
    ret
