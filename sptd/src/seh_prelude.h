// Force-included into the recompiled code (see CMakeLists.txt).
//
// generate_exception_handlers emits SEH_TRY / SEH_CATCH_ALL / SEH_END, but the
// generated PCH of ReXGlue v0.10.0 includes neither the header that defines
// them nor, on Windows, the one that declares GetExceptionCode().
#pragma once

#if defined(_WIN32)
#include <excpt.h>
#endif
#include <rex/platform/exceptions.h>
