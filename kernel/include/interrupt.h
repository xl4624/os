#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdbool.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

void interrupt_enable();
void interrupt_disable();
__attribute__((noreturn)) void halt_and_catch_fire();

__END_DECLS

#endif /* INTERRUPT_H */
