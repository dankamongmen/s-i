/* Macro definitions from <linux/keyboard.h>.  */
#define MAX_NR_KEYMAPS 256
#define MAX_NR_FUNC 256
#define MAX_DIACR 256

#define KG_SHIFT 0
#define KG_CTRL 2
#define KG_ALT 3
#define KG_ALTGR 1
#define KG_SHIFTL 4
#define KG_SHIFTR 5
#define KG_CTRLL 6
#define KG_CTRLR 7

#define KT_LATIN 0
#define KT_LETTER 11
#define KT_FN 1
#define KT_SPEC 2
#define KT_META 8

#define K(t,v) (((t)<<8)|(v))
#define KTYP(x) ((x) >> 8)
#define KVAL(x) ((x) & 0xff)

#define K_HOLE K(KT_SPEC,0)
