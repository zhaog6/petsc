#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: inherit.c,v 1.14 1997/02/22 02:23:29 bsmith Exp balay $";
#endif
/*
     Provides utility routines for manulating any type of PETSc object.
*/
#include "petsc.h"  /*I   "petsc.h"    I*/

#undef __FUNC__  
#define __FUNC__ "PetscObjectInherit_DefaultCopy" /* ADIC Ignore */
/*
    The default copy simply copies the pointer and adds one to the 
  reference counter.

*/
static int PetscObjectInherit_DefaultCopy(void *in, void **out)
{
  PetscObject obj = (PetscObject) in;

  obj->refct++;
  *out = in;
  return 0;
}

#undef __FUNC__  
#define __FUNC__ "PetscObjectInherit_DefaultDestroy" /* ADIC Ignore */
/*
    The default destroy treats it as a PETSc object and calls 
  its destroy routine.
*/
static int PetscObjectInherit_DefaultDestroy(void *in)
{
  int         ierr;
  PetscObject obj = (PetscObject) in;

  ierr = (*obj->destroy)(obj); CHKERRQ(ierr);
  return 0;
}

#undef __FUNC__  
#define __FUNC__ "PetscObjectReference" /* ADIC Ignore */
/*@C
   PetscObjectReference - Indicate to any PetscObject that it is being
       referenced in another PetscObject. This increases the reference
       count for that object by one.

   Input Parameter:
.   obj - the PETSc object

.seealso: PetscObjectInherit()
@*/
int PetscObjectReference(PetscObject obj)
{
  PetscValidHeader(obj);
  obj->refct++;
  return 0;
}

#undef __FUNC__  
#define __FUNC__ "PetscObjectInherit" /* ADIC Ignore */
/*@C
   PetscObjectInherit - Associate another object with a given PETSc object. 
                        This is to provide a limited support for inheritence.

   Input Parameters:
.  obj - the PETSc object
.  ptr - the other object to associate with the PETSc object
.  copy - a function used to copy the other object when the PETSc object 
          is copied, or PETSC_NULL to indicate the pointer is copied.

   Notes:
   PetscObjectInherit() can be used with any PETSc object, such at
   Mat, Vec, KSP, SNES, etc. Current limitation: each object can have
   only one child - we may extend this eventually.

.keywords: object, inherit

.seealso: PetscObjectGetChild()
@*/
int PetscObjectInherit(PetscObject obj,void *ptr, int (*copy)(void *,void **),
                       int (*destroy)(void*))
{
/*
  if (obj->child) 
    SETERRQ(1,0,"Child already set;object can have only 1 child");
*/
  if (copy == PETSC_NULL)    copy = PetscObjectInherit_DefaultCopy;
  if (destroy == PETSC_NULL) destroy = PetscObjectInherit_DefaultDestroy;
  obj->child        = ptr;
  obj->childcopy    = copy;
  obj->childdestroy = destroy;
  return 0;
}

#undef __FUNC__  
#define __FUNC__ "PetscObjectGetChild" /* ADIC Ignore */
/*@C
   PetscObjectGetChild - Gets the child of any PetscObject.

   Input Parameter:
.  obj - any PETSc object, for example a Vec, Mat or KSP.

   Output Parameter:
.  type - the child, if it has been set (otherwise PETSC_NULL)

.keywords: object, get, child

.seealso: PetscObjectInherit()
@*/
int PetscObjectGetChild(PetscObject obj,void **child)
{
  if (!obj) SETERRQ(1,0,"Null object");
  *child = obj->child;
  return 0;
}

