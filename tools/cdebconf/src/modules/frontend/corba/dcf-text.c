/*
 * cdebconf frontend, corba servant, text frontend
 *
 * $Id: dcf-text.c,v 1.2 2001/02/22 05:59:06 zw Exp $
 */

#include "dcf-textimpl.c"
#include <ORBitservices/CosNaming.h>

int main(int argc, char* argv[]) {
  CORBA_ORB                 orb;
  CORBA_Environment	    *ev;
  PortableServer_ObjectId   *oid;
  PortableServer_POA        root_poa;
  PortableServer_POAManager pm;
  CosNaming_NamingContext   root;
  CosNaming_NameComponent   name_component[2] = {{"Debconf", "subcontext"},
						 {"Frontend", "server"}};
  CosNaming_Name 	    name = {2, 2, name_component, CORBA_FALSE};
  Debconf_Frontend     	    dcf_text;
  CORBA_char                *objref;
  
  ev = g_new0(CORBA_Environment,1);

  CORBA_exception_init(ev);

  orb = CORBA_ORB_init (&argc, argv, "orbit-local-orb", ev);
  if (ev->_major != CORBA_NO_EXCEPTION) {
    fprintf(stderr, "Error: could not initializing CORBA ORB: %s\n", 
	    CORBA_exception_id(ev));
    exit(1);
  }

  root_poa = (PortableServer_POA) CORBA_ORB_resolve_initial_references(orb, "RootPOA",
								       ev);
  if (ev->_major != CORBA_NO_EXCEPTION) {
    fprintf(stderr, "Error: could not get RootPOA: %s\n", 
	    CORBA_exception_id(ev));
    exit(1);
  }

  dcf_text = impl_Debconf_Frontend__create(root_poa, ev);

  pm = PortableServer_POA__get_the_POAManager(root_poa, ev);
  PortableServer_POAManager_activate(pm, ev);

  objref = CORBA_ORB_object_to_string(orb, dcf_text, ev);
  fprintf(stderr, "%s\n", objref);

  root = (CosNaming_NamingContext) CORBA_ORB_resolve_initial_service (orb, "NameService",
								      ev);
  if (ev->_major != CORBA_NO_EXCEPTION) {
    fprintf(stderr, "Error: could not get name service: %s\n", 
	    CORBA_exception_id(ev));
    exit(1);
  }
  
  CosNaming_NamingContext_bind(root, &name, dcf_text, ev);
  if (ev->_major != CORBA_NO_EXCEPTION) {
    fprintf(stderr, "Error: could register object: %s\n", 
	    CORBA_exception_id(&ev));
    exit(1);
  }
  
  /*
   * TODO: SIGINT is ignored by the text UI, otherwise it interfers with
   * the way question/answers are handled. this is probably not optimal
   */
  signal(SIGINT, SIG_IGN);
  CORBA_ORB_run(orb, ev);

  CORBA_exception_free(ev);
  return 0;
}
