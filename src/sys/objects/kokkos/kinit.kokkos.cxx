#include <petscsys.h>
#include <petsc/private/petscimpl.h>
#include <Kokkos_Core.hpp>

PetscErrorCode PetscKokkosFinalize_Private(void)
{
  PetscFunctionBegin;
  Kokkos::finalize();
  PetscFunctionReturn(0);
}

PetscErrorCode PetscKokkosIsInitialized_Private(PetscBool *isInitialized)
{
  PetscFunctionBegin;
  *isInitialized = Kokkos::is_initialized() ? PETSC_TRUE : PETSC_FALSE;
  PetscFunctionReturn(0);
}

/* Initialize Kokkos if not yet */
PetscErrorCode PetscKokkosInitializeCheck(void)
{
  PetscErrorCode        ierr;
  Kokkos::InitArguments args;
  int                   devId = -1;

  PetscFunctionBegin;
  if (!Kokkos::is_initialized()) {
   #if defined(KOKKOS_ENABLE_CUDA)
    ierr = PetscCUDAInitializeCheck();CHKERRQ(ierr);
    cudaGetDevice(&devId);
   #elif defined(KOKKOS_ENABLE_HIP) /* Kokkos does not support CUDA and HIP at the same time */
    ierr = PetscHIPInitializeCheck();CHKERRQ(ierr);
    hipGetDevice(&devId);
   #endif
    args.device_id   = devId;
    Kokkos::initialize(args);
    PetscBeganKokkos = PETSC_TRUE;
  }
  PetscFunctionReturn(0);
}