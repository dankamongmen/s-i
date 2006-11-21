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
    char *fstype;
    unsigned int mbminsize;
    unsigned int mbmaxsize;

    pv_stack = lvm_lv_stack_new();

    assert(lvm_lv_stack_isempty(pv_stack));
    lvm_lv_stack_push(pv_stack, "vgname0", "lvname0", "fstype0", 10, 20);
    assert(!lvm_lv_stack_isempty(pv_stack));

    lvm_lv_stack_pop(pv_stack, &vgname, &lvname, &fstype, &mbminsize,
		     &mbmaxsize);
    assert(0 == strcmp(vgname, "vgname0"));
    free(vgname);
    free(lvname);

    lvm_lv_stack_push(pv_stack, "vgname0", "lvname0", "fstype0", 10, 21);
    lvm_lv_stack_push(pv_stack, "vgname1", "lvname1", "fstype1", 11, 22);

    lvm_lv_stack_pop(pv_stack, &vgname, &lvname, &fstype, &mbminsize,
		     &mbmaxsize);
    assert(0 == strcmp(vgname, "vgname1"));
    free(vgname);
    free(lvname);

    lvm_lv_stack_delete(pv_stack);
    pv_stack = NULL;
}

static void
test_split(void)
{
  char *info[4]; /* 0='lvm', 1=vgname, 2=lvname, 3=fstype */
  char *fstype = "lvm:tjener_vg:home0_lv:default";
  
  if (0 != lvm_split_fstype(fstype, ':', 4, info))
    autopartkit_log(0, "Failed to parse '%s'\n", fstype);
  else
    {
      autopartkit_log(2, "  Stacking LVM lv %s on vg %s "
		      "fstype %s\n", info[1], info[2], info[3]);
    }
}

int
main(int argc ATTRIBUTE_UNUSED, char **argv ATTRIBUTE_UNUSED)
{
    test_pv_stack();
    test_lv_stack();
    test_split();
    return 0;
}
