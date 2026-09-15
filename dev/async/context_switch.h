#pragma once

/**
 *  Minimal stackful context switch for x86_64 (SysV) and aarch64 (AAPCS64),
 *  GCC or Clang, as inline naked functions so that the library stays header-only.
 *
 *      void sqlite_orm_switch_context(void** saveStackPointer, void* loadStackPointer);
 *
 *  Pushes the callee-saved registers of the current context onto its stack,
 *  stores the resulting stack pointer into *saveStackPointer, loads
 *  loadStackPointer, pops the callee-saved registers of the target context and
 *  returns into it.
 *
 *  A brand-new context is created in fiber.h by laying out an initial frame that
 *  "returns" into sqlite_orm_context_trampoline with the entry function and its
 *  argument in two callee-saved registers.
 */
extern "C" {

#if defined(__x86_64__)

__attribute__((naked, noinline)) inline void sqlite_orm_switch_context(void** /*saveStackPointer*/,
                                                                       void* /*loadStackPointer*/) {
    __asm__ volatile("pushq %rbp\n\t"
                     "pushq %rbx\n\t"
                     "pushq %r12\n\t"
                     "pushq %r13\n\t"
                     "pushq %r14\n\t"
                     "pushq %r15\n\t"
                     "movq  %rsp, (%rdi)\n\t"
                     "movq  %rsi, %rsp\n\t"
                     "popq  %r15\n\t"
                     "popq  %r14\n\t"
                     "popq  %r13\n\t"
                     "popq  %r12\n\t"
                     "popq  %rbx\n\t"
                     "popq  %rbp\n\t"
                     "ret\n\t");
}

/**
 *  r12 = entry, r13 = argument. rsp is 16-byte aligned on entry.
 */
__attribute__((naked, noinline)) inline void sqlite_orm_context_trampoline() {
    __asm__ volatile("movq  %r13, %rdi\n\t"
                     "callq *%r12\n\t"
                     "ud2\n\t");
}

#elif defined(__aarch64__)

__attribute__((naked, noinline)) inline void sqlite_orm_switch_context(void** /*saveStackPointer*/,
                                                                       void* /*loadStackPointer*/) {
    __asm__ volatile("sub  sp, sp, #176\n\t"
                     "stp  x19, x20, [sp, #0]\n\t"
                     "stp  x21, x22, [sp, #16]\n\t"
                     "stp  x23, x24, [sp, #32]\n\t"
                     "stp  x25, x26, [sp, #48]\n\t"
                     "stp  x27, x28, [sp, #64]\n\t"
                     "stp  x29, x30, [sp, #80]\n\t"
                     "stp  d8,  d9,  [sp, #96]\n\t"
                     "stp  d10, d11, [sp, #112]\n\t"
                     "stp  d12, d13, [sp, #128]\n\t"
                     "stp  d14, d15, [sp, #144]\n\t"
                     "mov  x2, sp\n\t"
                     "str  x2, [x0]\n\t"
                     "mov  sp, x1\n\t"
                     "ldp  x19, x20, [sp, #0]\n\t"
                     "ldp  x21, x22, [sp, #16]\n\t"
                     "ldp  x23, x24, [sp, #32]\n\t"
                     "ldp  x25, x26, [sp, #48]\n\t"
                     "ldp  x27, x28, [sp, #64]\n\t"
                     "ldp  x29, x30, [sp, #80]\n\t"
                     "ldp  d8,  d9,  [sp, #96]\n\t"
                     "ldp  d10, d11, [sp, #112]\n\t"
                     "ldp  d12, d13, [sp, #128]\n\t"
                     "ldp  d14, d15, [sp, #144]\n\t"
                     "add  sp, sp, #176\n\t"
                     "ret\n\t");
}

/**
 *  x19 = entry, x20 = argument.
 */
__attribute__((naked, noinline)) inline void sqlite_orm_context_trampoline() {
    __asm__ volatile("mov  x0, x20\n\t"
                     "blr  x19\n\t"
                     "brk  #0\n\t");
}

#else
#error "sqlite_orm async: unsupported architecture"
#endif
}
