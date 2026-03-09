#include <m_pd.h>

static t_class *gemrepeat_class=NULL;

typedef struct _gemrepeat {
  t_object x_obj;
  t_float fcount;
} t_gemrepeat;

static void gemrepeat_anything(t_gemrepeat *x, t_symbol *s, int argc,
                            t_atom *argv)
{
  if(s != gensym("gem_state")) return;
  
  int i;
  i=x->fcount;
  if (i<0) {
    i=1;
  }
  while(i--) {
    outlet_anything(x->x_obj.ob_outlet, s, argc, argv);
  }
}

static void *gemrepeat_new(t_symbol* s, int argc, t_atom*argv)
{
    t_gemrepeat *x = (t_gemrepeat *)pd_new(gemrepeat_class);
  if(argc) {
    if(A_FLOAT==argv->a_type) {
      x->fcount = atom_getfloat(argv);
    } else {
      return 0;
    }
  } else {
    x->fcount=2;
  }
  floatinlet_new(&x->x_obj, &x->fcount);
  outlet_new(&x->x_obj, 0);
  return (x);
}

extern "C"
{
void gemrepeat_setup(void)
{
    gemrepeat_class = class_new(gensym("gemrepeat"),
                                (t_newmethod)gemrepeat_new, 0, sizeof(t_gemrepeat), CLASS_DEFAULT, A_GIMME, A_NULL);
    
    class_addanything(gemrepeat_class, gemrepeat_anything);
}
}
