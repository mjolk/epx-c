/**
 * File   : replica.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : za 02 feb 2019 15:06
 */

#include "../src/replica.h"

int main(){
    struct replica r;
    assert(new_replica(&r) == 0);
    return 0;
}
