#include <Tpetra_ConfigDefs.hpp>
#ifdef HAVE_TPETRACORE_TEUCHOSKOKKOSCOMPAT
#include <KokkosCore_config.h>
#ifdef KOKKOS_HAVE_CUDA
#define KOKKOS_USE_CUDA_BUILD
#include "Map/Map_ExportTest_Bug5882.cpp"
#undef KOKKOS_USE_CUDA_BUILD
#endif
#endif