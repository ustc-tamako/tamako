#include "console.h"
#include "debug.h"

int kern_entry()
{
    debug_init();

    console_clear();

    print_cur_status();

    panic("test");

    return 0;
}