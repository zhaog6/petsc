#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: mpiu.c,v 1.71 1997/05/17 23:55:44 bsmith Exp balay $";
#endif
/*
      Some PETSc utilites routines to add simple IO capability.
*/
#include "petsc.h"        
#include "sys.h"             /*I    "sys.h"   I*/
#include <stdio.h>
#include <stdarg.h>
#if defined(HAVE_STDLIB_H)
#include <stdlib.h>
#endif
#include "pinclude/petscfix.h"

/*
   If petsc_history is on then all Petsc*Printf() results are saved
   if the appropriate (usually .petschistory) file.
*/
extern FILE *petsc_history;

/* ----------------------------------------------------------------------- */

typedef struct _PrintfQueue *PrintfQueue;
struct _PrintfQueue {
  char        string[256];
  PrintfQueue next;
};
static PrintfQueue queue       = 0,queuebase = 0;
static int         queuelength = 0;

#undef __FUNC__  
#define __FUNC__ "PetscSynchronizedPrintf" /* ADIC Ignore */
/*@C
    PetscSynchronizedPrintf - Prints output from several processors that
        is synchronized so that printed by first processor is followed by 
        second etc.

    Input Parameters:
.   comm - the communicator
.   format - the usual printf() format string 

    Notes:
     You cannot mix usage with more then one MPI communicator without an 
     intervening call to PetscSynchronizedFlush().
     The length of the formatted message cannot be more then 256 charactors.

.seealso: PetscSynchronizedFlush(), PetscFPrintf(), PetscPrintf()
@*/
int PetscSynchronizedPrintf(MPI_Comm comm,char *format,...)
{
  int rank;
  MPI_Comm_rank(comm,&rank);
  
  /* First processor prints immediately to stdout */
  if (!rank) {
    va_list Argp;
    va_start( Argp, format );
#if (__GNUC__ == 2 && __GNUC_MINOR__ >= 7 && defined(PARCH_freebsd) )
    vfprintf(stdout,format,(char*)Argp);
#else
    vfprintf(stdout,format,Argp);
#endif
    fflush(stdout);
    if (petsc_history) {
#if (__GNUC__ == 2 && __GNUC_MINOR__ >= 7 && defined(PARCH_freebsd) )
      vfprintf(petsc_history,format,(char *)Argp);
#else
      vfprintf(petsc_history,format,Argp);
#endif
      fflush(petsc_history);
    }
    va_end( Argp );
  } else { /* other processors add to local queue */
    int         len;
    va_list     Argp;
    PrintfQueue next = PetscNew(struct _PrintfQueue); CHKPTRQ(next);
    if (queue) {queue->next = next; queue = next;}
    else       {queuebase   = queue = next;}
    queuelength++;
    va_start( Argp, format );
#if (__GNUC__ == 2 && __GNUC_MINOR__ >= 7 && defined(PARCH_freebsd) )
    vsprintf(next->string,format,(char *)Argp);
#else
    vsprintf(next->string,format,Argp);
#endif
    va_end( Argp );
    len = PetscStrlen(next->string);
    if (len > 256) SETERRQ(1,0,"Formated string longer then 256 bytes");
  }
    
  return 0;
}
 
#undef __FUNC__  
#define __FUNC__ "PetscSynchronizedFlush" /* ADIC Ignore */
/*@C
    PetscSynchronizedFlush - Flushes to the screen output from all processors 
        involved in previous PetscSynchronizedPrintf() calls.

    Input Parameters:
.   comm - the communicator

    Notes:
     You cannot mix usage with more then one MPI communicator without an 
     intervening call to PetscSynchronizedFlush().

.seealso: PetscSynchronizedPrintf(), PetscFPrintf(), PetscPrintf()
@*/
int PetscSynchronizedFlush(MPI_Comm comm)
{
  int        rank,size,i,j,n, tag = 12341;
  char       message[256];
  MPI_Status status;

  MPI_Comm_rank(comm,&rank);
  MPI_Comm_size(comm,&size);
  
  /* First processor waits for messages from all other processors */
  if (!rank) {
    for ( i=1; i<size; i++ ) {
      MPI_Recv(&n,1,MPI_INT,i,tag,comm,&status);
      for ( j=0; j<n; j++ ) {
        MPI_Recv(message,256,MPI_CHAR,i,tag,comm,&status);
        printf("%s",message);
        if (petsc_history) {
          fprintf(petsc_history,"%s",message);
        }
      }
    }
    fflush(stdout);
    if (petsc_history) fflush(petsc_history);
  } else { /* other processors send queue to processor 0 */
    PrintfQueue next = queuebase,previous;

    MPI_Send(&queuelength,1,MPI_INT,0,tag,comm);
    for ( i=0; i<queuelength; i++ ) {
      MPI_Send(next->string,256,MPI_CHAR,0,tag,comm);
      previous = next; 
      next     = next->next;
      PetscFree(previous);
    }
    queue       = 0;
    queuelength = 0;
  }
  return 0;
}

/* ---------------------------------------------------------------------------------------*/

#undef __FUNC__  
#define __FUNC__ "PetscFPrintf" /* ADIC Ignore */
/*@C
    PetscFPrintf - Prints to a file, only from the first
    processor in the communicator.

    Input Parameters:
.   comm - the communicator
.   fd - the file pointer
.   format - the usual printf() format string 

    Fortran Note:
    This routine is not supported in Fortran.

.keywords: parallel, fprintf

.seealso: PetscPrintf(), PetscSynchronizedPrintf()
@*/
int PetscFPrintf(MPI_Comm comm,FILE* fd,char *format,...)
{
  int rank;
  MPI_Comm_rank(comm,&rank);
  if (!rank) {
    va_list Argp;
    va_start( Argp, format );
#if (__GNUC__ == 2 && __GNUC_MINOR__ >= 7 && defined(PARCH_freebsd) )
    vfprintf(fd,format,(char*)Argp);
#else
    vfprintf(fd,format,Argp);
#endif
    fflush(fd);
    if (petsc_history) {
#if (__GNUC__ == 2 && __GNUC_MINOR__ >= 7 && defined(PARCH_freebsd) )
      vfprintf(petsc_history,format,(char *)Argp);
#else
      vfprintf(petsc_history,format,Argp);
#endif
      fflush(petsc_history);
    }
    va_end( Argp );
  }
  return 0;
}

#undef __FUNC__  
#define __FUNC__ "PetscPrintf" /* ADIC Ignore */
/*@C
    PetscPrintf - Prints to standard out, only from the first
    processor in the communicator.

   Input Parameters:
.  comm - the communicator
.  format - the usual printf() format string 

    Fortran Note:
    This routine is not supported in Fortran.

.keywords: parallel, printf

.seealso: PetscFPrintf(), PetscSynchronizedPrintf()
@*/
int PetscPrintf(MPI_Comm comm,char *format,...)
{
  int rank;
  if (!comm) comm = PETSC_COMM_WORLD;
  MPI_Comm_rank(comm,&rank);
  if (!rank) {
    va_list Argp;
    va_start( Argp, format );
#if (__GNUC__ == 2 && __GNUC_MINOR__ >= 7 && defined(PARCH_freebsd) )
    vfprintf(stdout,format,(char *)Argp);
#else
    vfprintf(stdout,format,Argp);
#endif
    fflush(stdout);
    if (petsc_history) {
#if (__GNUC__ == 2 && __GNUC_MINOR__ >= 7 && defined(PARCH_freebsd) )
      vfprintf(petsc_history,format,(char *)Argp);
#else
      vfprintf(petsc_history,format,Argp);
#endif
      fflush(petsc_history);
    }
    va_end( Argp );
  }
  return 0;
}

/*
     PetscSetDisplay - Tries to set the X windows display variable for all processors.
                       The variable PetscDisplay contains the X windows display variable.

*/
static char PetscDisplay[128]; 

#undef __FUNC__  
#define __FUNC__ "PetscSetDisplay" /* ADIC Ignore */
int PetscSetDisplay()
{
  int  size,rank,len,ierr,flag;
  char *str;

  ierr = OptionsGetString(0,"-display",PetscDisplay,128,&flag);CHKERRQ(ierr);
  if (flag) return 0;

  MPI_Comm_size(PETSC_COMM_WORLD,&size);
  MPI_Comm_rank(PETSC_COMM_WORLD,&rank);  
  if (!rank) {
    str = getenv("DISPLAY");
    if (!str || (str[0] == ':' && size > 1)) {
      ierr = PetscGetHostName(PetscDisplay,124); CHKERRQ(ierr);
      PetscStrcat(PetscDisplay,":0.0");
    }
    else {
      PetscStrncpy(PetscDisplay,str,128);
    }
    len = PetscStrlen(PetscDisplay);
    MPI_Bcast(&len,1,MPI_INT,0,PETSC_COMM_WORLD);
    MPI_Bcast(PetscDisplay,len,MPI_CHAR,0,PETSC_COMM_WORLD);
  }
  else {
    MPI_Bcast(&len,1,MPI_INT,0,PETSC_COMM_WORLD);
    MPI_Bcast(PetscDisplay,len,MPI_CHAR,0,PETSC_COMM_WORLD);
    PetscDisplay[len] = 0;
  }
  return 0;  
}

#undef __FUNC__  
#define __FUNC__ "PetscGetDisplay" /* ADIC Ignore */
/*
     PetscGetDisplay - Gets the display variable for all processors.

  Input Parameters:
.   n - length of string display

  Output Parameters:
.   display - the display string, may (and should) be freed.

*/
int PetscGetDisplay(char *display,int n)
{
  PetscStrncpy(display,PetscDisplay,n);
  return 0;  
}

#undef __FUNC__  
#define __FUNC__ "PetscSequentialPhaseBegin_Private" /* ADIC Ignore */
int PetscSequentialPhaseBegin_Private(MPI_Comm comm,int ng )
{
  int        lidx, np, tag = 0;
  MPI_Status status;

  MPI_Comm_size( comm, &np );
  if (np == 1) return 0;
  MPI_Comm_rank( comm, &lidx );
  if (lidx != 0) {
    MPI_Recv( 0, 0, MPI_INT, lidx-1, tag, comm, &status );
  }
  /* Send to the next process in the group unless we are the last process */ 
  if ((lidx % ng) < ng - 1 && lidx != np - 1) {
    MPI_Send( 0, 0, MPI_INT, lidx + 1, tag, comm );
  }
  return 0;
}

#undef __FUNC__  
#define __FUNC__ "PetscSequentialPhaseEnd_Private" /* ADIC Ignore */
int PetscSequentialPhaseEnd_Private(MPI_Comm comm,int ng )
{
  int        lidx, np, tag = 0;
  MPI_Status status;

  MPI_Comm_rank( comm, &lidx );
  MPI_Comm_size( comm, &np );
  if (np == 1) return 0;

  /* Send to the first process in the next group */
  if ((lidx % ng) == ng - 1 || lidx == np - 1) {
    MPI_Send( 0, 0, MPI_INT, (lidx + 1) % np, tag, comm );
  }
  if (lidx == 0) {
    MPI_Recv( 0, 0, MPI_INT, np-1, tag, comm, &status );
  }
  return 0;
}

/* ---------------------------------------------------------------------*/
/*
    The variable Petsc_Seq_keyval is used to indicate an MPI attribute that
  is attached to a communicator that manages the sequential phase code below.
*/
static int Petsc_Seq_keyval = MPI_KEYVAL_INVALID;

#undef __FUNC__  
#define __FUNC__ "PetscSequentialPhaseBegin" /* ADIC Ignore */
/*@C
   PetscSequentialPhaseBegin - Begins a sequential section of code.  

   Input Parameters:
.  comm - Communicator to sequentialize.  
.  ng   - Number in processor group.  This many processes are allowed to execute
   at the same time (usually 1)

   Notes:
   PetscSequentialPhaseBegin() and PetscSequentialPhaseEnd() provide a
   way to force a section of code to be executed by the processes in
   rank order.  Typically, this is done with
$  PetscSequentialPhaseBegin( comm, 1 );
$  <code to be executed sequentially>
$  PetscSequentialPhaseEnd( comm, 1 );
$
   Often, the sequential code contains output statements (e.g., printf) to
   be executed.  Note that you may need to flush the I/O buffers before
   calling PetscSequentialPhaseEnd().  Also, note that some systems do
   not propagate I/O in any order to the controling terminal (in other words, 
   even if you flush the output, you may not get the data in the order
   that you want).

.seealso: PetscSequentialPhaseEnd()

.keywords: sequential, phase, begin
@*/
int PetscSequentialPhaseBegin(MPI_Comm comm,int ng )
{
  int        ierr, np;
  MPI_Comm   local_comm,*addr_local_comm;

  MPI_Comm_size( comm, &np );
  if (np == 1) return 0;

  /* Get the private communicator for the sequential operations */
  if (Petsc_Seq_keyval == MPI_KEYVAL_INVALID) {
    MPI_Keyval_create(MPI_NULL_COPY_FN,MPI_NULL_DELETE_FN,&Petsc_Seq_keyval,0);
  }

  MPI_Comm_dup( comm, &local_comm );
  addr_local_comm  = (MPI_Comm *) PetscMalloc(sizeof(MPI_Comm));CHKPTRQ(addr_local_comm);
  *addr_local_comm = local_comm;
  MPI_Attr_put( comm, Petsc_Seq_keyval, (void *) addr_local_comm );
  ierr = PetscSequentialPhaseBegin_Private(local_comm,ng); CHKERRQ(ierr);
  return 0;
}

#undef __FUNC__  
#define __FUNC__ "PetscSequentialPhaseEnd" /* ADIC Ignore */
/*@C
   PetscSequentialPhaseEnd - Ends a sequential section of code.

   Input Parameters:
.  comm - Communicator to sequentialize.  
.  ng   - Number in processor group.  This many processes are allowed to execute
   at the same time (usually 1)


   Notes:
   See PetscSequentialPhaseBegin() for more details.

.seealso: PetscSequentialPhaseBegin()

.keywords: sequential, phase, end
@*/
int PetscSequentialPhaseEnd(MPI_Comm comm,int ng )
{
  int        ierr, np, flag;
  MPI_Comm   local_comm,*addr_local_comm;

  MPI_Comm_size( comm, &np );
  if (np == 1) return 0;

  MPI_Attr_get( comm, Petsc_Seq_keyval, (void **)&addr_local_comm, &flag );
  if (!flag) MPI_Abort( comm, MPI_ERR_UNKNOWN );
  local_comm = *addr_local_comm;

  ierr = PetscSequentialPhaseEnd_Private(local_comm,ng); CHKERRQ(ierr);

  PetscFree(addr_local_comm); 
  MPI_Comm_free(&local_comm);
  MPI_Attr_delete(comm,Petsc_Seq_keyval);

  return 0;
}
/* ---------------------------------------------------------------- */
/*
   A simple way to manage tags inside a private 
   communicator.  It uses the attribute to determine if a new communicator
   is needed.

   Notes on the implementation

   The tagvalues to use are stored in a two element array.  The first element
   is the first free tag value.  The second is used to indicate how
   many "copies" of the communicator there are used in destroying.
*/

static int Petsc_Tag_keyval = MPI_KEYVAL_INVALID;

#undef __FUNC__  
#define __FUNC__ "Petsc_DelTag" /* ADIC Ignore */
/*
   Private routine to delete internal storage when a communicator is freed.
  This is called by MPI, not by users.

  The binding for the first argument changed from MPI 1.0 to 1.1; in 1.0
  it was MPI_Comm *comm.  
*/
static int Petsc_DelTag(MPI_Comm comm,int keyval,void* attr_val,void* extra_state )
{
  PetscFree( attr_val );
  return MPI_SUCCESS;
}

#undef __FUNC__  
#define __FUNC__ "PetscObjectGetNewTag" /* ADIC Ignore */
/*@
    PetscObjectGetNewTag - Gets a unique new tag from a PETSc object. All 
    processors that share the object MUST call this routine EXACTLY the same
    number of times.  This tag should only be used with the current object's
    communicator; do NOT use it with any other MPI communicator.

    Input Parameter:
.   obj - the PETSc object

    Output Parameter:
.   tag - the new tag

.keywords: object, get, new, tag

.seealso: PetscObjectRestoreNewTag()
@*/
int PetscObjectGetNewTag(PetscObject obj,int *tag)
{
  int ierr,*tagvalp=0,flag;

  PetscValidHeader(obj);
  PetscValidIntPointer(tag);

  ierr = MPI_Attr_get(obj->comm,Petsc_Tag_keyval,(void**)&tagvalp,&flag);CHKERRQ(ierr);
  if (!flag) SETERRQ(1,0,"Bad comm in PETSc object");

  if (*tagvalp < 1) SETERRQ(1,0,"Out of tags for object");
  *tag = tagvalp[0]--;
  return 0;
}

#undef __FUNC__  
#define __FUNC__ "PetscObjectRestoreNewTag" /* ADIC Ignore */
/*@
    PetscObjectRestoreNewTag - Restores a new tag from a PETSc object. All 
    processors that share the object MUST call this routine EXACTLY the same
    number of times. 

    Input Parameter:
.   obj - the PETSc object

    Output Parameter:
.   tag - the new tag

.keywords: object, restore, new, tag

.seealso: PetscObjectGetNewTag()
@*/
int PetscObjectRestoreNewTag(PetscObject obj,int *tag)
{
  int ierr,*tagvalp=0,flag;

  PetscValidHeader(obj);
  PetscValidIntPointer(tag);

  ierr = MPI_Attr_get(obj->comm,Petsc_Tag_keyval,(void**)&tagvalp,&flag);CHKERRQ(ierr);
  if (!flag) SETERRQ(1,0,"Bad comm in PETSc object");

  if (*tagvalp == *tag - 1) {
    tagvalp[0]++;
  }
  return 0;
}

#undef __FUNC__  
#define __FUNC__ "PetscCommDup_Private" /* ADIC Ignore */
/*
  PetscCommDup_Private - Duplicates the communicator only if it is not already PETSc 
                         communicator.

  Input Parameters:
. comm_in - Input communicator

  Output Parameters:
. comm_out - Output communicator.  May be 'comm_in'.
. first_tag - First tag available

  Notes:
  This routine returns one tag number.
  Call Petsc_Comm_free() when finished with the communicator.
*/
int PetscCommDup_Private(MPI_Comm comm_in,MPI_Comm *comm_out,int* first_tag)
{
  int ierr = MPI_SUCCESS, *tagvalp, *maxval, flag;

  if (Petsc_Tag_keyval == MPI_KEYVAL_INVALID) {
    /* 
       The calling sequence of the 2nd argument to this function changed
       between MPI Standard 1.0 and the revisions 1.1 Here we match the 
       new standard, if you are using an MPI implementation that uses 
       the older version you will get a warning message about the next line;
       it is only a warning message and should do no harm.
    */
    MPI_Keyval_create(MPI_NULL_COPY_FN, Petsc_DelTag,&Petsc_Tag_keyval,(void*)0);
  }

  ierr = MPI_Attr_get(comm_in,Petsc_Tag_keyval,(void**)&tagvalp,&flag);CHKERRQ(ierr);

  if (!flag) {
    /* This communicator is not yet known to this system, so we dup it and set its value */
    MPI_Comm_dup( comm_in, comm_out );
    MPI_Attr_get( MPI_COMM_WORLD, MPI_TAG_UB, (void**)&maxval, &flag );
    tagvalp    = (int *) PetscMalloc( 2*sizeof(int) ); CHKPTRQ(tagvalp);
    tagvalp[0] = *maxval;
    tagvalp[1] = 0;
    MPI_Attr_put(*comm_out,Petsc_Tag_keyval, tagvalp);
  }
  else {
    *comm_out = comm_in;
  }

  if (*tagvalp < 1) SETERRQ(1,0,"Out of tags for object");
  *first_tag = tagvalp[0]--;
  tagvalp[1]++;
#if defined(PETSC_BOPT_g)
  if (*comm_out == comm_in) {
    int size;
    MPI_Comm_size(*comm_out,&size);
    if (size > 1) {
      int tag1 = *first_tag, tag2;
      MPI_Allreduce(&tag1,&tag2,1,MPI_INT,MPI_BOR,*comm_out);
      if (tag2 != tag1) {
        SETERRQ(1,0,"Communicator was used on subset\n of processors.");
      }
    }
  }
#endif
  return 0;
}

#undef __FUNC__  
#define __FUNC__ "PetscCommFree_Private" /* ADIC Ignore */
/*
  PetscCommFree_Private - Frees communicator.  Use in conjunction with PetscCommDup_Private().
*/
int PetscCommFree_Private(MPI_Comm *comm)
{
  int ierr,*tagvalp,flag;

  ierr = MPI_Attr_get(*comm,Petsc_Tag_keyval,(void**)&tagvalp,&flag);CHKERRQ(ierr);
  if (!flag) {
    SETERRQ(1,0,"Error freeing PETSc object, problem with corrupted memory");
  }
  tagvalp[1]--;
  if (!tagvalp[1]) {MPI_Comm_free(comm);}
  return 0;
}




