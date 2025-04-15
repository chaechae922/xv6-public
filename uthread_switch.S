    .text
    .globl thread_switch
thread_switch:
    pushal

    movl current_thread, %eax 
    movl %esp, (%eax) 

    movl next_thread, %eax 
    movl (%eax), %esp 

    movl %eax, current_thread
    popal

    ret
