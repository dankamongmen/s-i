#include "autopartkit.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void
test_pv_stack(void)
{
    void *pv_stack;
    char *vgname;
    char *devpath;

    pv_stack = lvm_pv_stack_new();

    assert(lvm_pv_stack_isempty(pv_stack));
    lvm_pv_stack_push(pv_stack, "vgname0", "devpath0");
    assert(!lvm_pv_stack_isempty(pv_stack));

    lvm_pv_stack_pop(pv_stack, &vgname, &devpath);
    assert(0 == strcmp(vgname, "vgname0"));
    free(vgname);
    free(devpath);

    lvm_pv_stack_push(pv_stack, "vgname0", "devpath0");
    lvm_pv_stack_push(pv_stack, "vgname1", "devpath1");

    lvm_pv_stack_pop(pv_stack, &vgname, &devpath);
    assert(0 == strcmp(vgname, "vgname1"));
    free(vgname);
    free(devpath);

    lvm_pv_stack_delete(pv_stack);
    pv_stack = NULL;
}

static void
test_lv_stack(void)
{
    void *pv_stack;
    char *vgname;
    char *lvname;
    unsigned int mbsize;

    pv_stack = lvm_lv_stack_new();

    assert(lvm_lv_stack_isempty(pv_stack));
    lvm_lv_stack_push(pv_stack, "vgname0", "lvname0", 10);
    assert(!lvm_lv_stack_isempty(pv_stack));

    lvm_lv_stack_pop(pv_stack, &vgname, &lvname, &mbsize);
    assert(0 == strcmp(vgname, "vgname0"));
    free(vgname);
    free(lvname);

    lvm_lv_stack_push(pv_stack, "vgname0", "lvname0", 10);
    lvm_lv_stack_push(pv_stack, "vgname1", "lvname1", 11);

    lvm_lv_stack_pop(pv_stack, &vgname, &lvname, &mbsize);
    assert(0 == strcmp(vgname, "vgname1"));
    free(vgname);
    free(lvname);

    lvm_lv_stack_delete(pv_stack);
    pv_stack = NULL;
}

int
main(int argc, char **argv)
{
    test_pv_stack();
    test_lv_stack();
    return 0;
}
