// Developer aid (Windows): when guest code reads or writes the guest null page,
// log the host call stack, symbolized with the exe's PDB, before the runtime's
// own handler reports "Unhandled guest access violation". The recompiled
// function names (sub_XXXXXXXX) say where in the game it happened.
//
// Enabled with --sptd_null_trace=true. Other access violations (the runtime
// uses them for GPU register access) are left alone.

#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(sptd_null_trace, false, "SPTD/Debug",
                    "Log a symbolized host stack when the game touches the guest null page");

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <dbghelp.h>

#include <cstdint>
#include <string>

#pragma comment(lib, "dbghelp.lib")

namespace sptd {
namespace {

uintptr_t g_guest_base = 0;

std::string Symbol(HANDLE process, DWORD64 address) {
  alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 256] = {};
  auto* info = reinterpret_cast<SYMBOL_INFO*>(buffer);
  info->SizeOfStruct = sizeof(SYMBOL_INFO);
  info->MaxNameLen = 255;
  DWORD64 displacement = 0;
  if (!SymFromAddr(process, address, &displacement, info)) return "?";
  std::string name(info->Name);
  IMAGEHLP_LINE64 line = {sizeof(line)};
  DWORD line_displacement = 0;
  if (SymGetLineFromAddr64(process, address, &line_displacement, &line))
    name += " (" + std::string(line.FileName ? line.FileName : "") + ":" + std::to_string(line.LineNumber) + ")";
  return name;
}

LONG CALLBACK OnException(EXCEPTION_POINTERS* info) {
  const auto* record = info->ExceptionRecord;
  if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || record->NumberParameters < 2) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  const uintptr_t target = record->ExceptionInformation[1];
  if (target < g_guest_base || target >= g_guest_base + 0x10000) return EXCEPTION_CONTINUE_SEARCH;

  static bool busy = false;  // one report at a time; the runtime re-raises
  if (busy) return EXCEPTION_CONTINUE_SEARCH;
  busy = true;
  HANDLE process = GetCurrentProcess();
  HANDLE thread = GetCurrentThread();
  static bool initialized = SymInitialize(process, nullptr, TRUE);
  (void)initialized;

  REXLOG_ERROR("SPTD null trace: {} of guest 0x{:08X}", record->ExceptionInformation[0] ? "write" : "read",
               uint32_t(target - g_guest_base));
  CONTEXT context = *info->ContextRecord;
  STACKFRAME64 frame = {};
  frame.AddrPC.Offset = context.Rip;
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrFrame.Offset = context.Rbp;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrStack.Offset = context.Rsp;
  frame.AddrStack.Mode = AddrModeFlat;
  for (int i = 0; i < 24; ++i) {
    if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &frame, &context, nullptr, SymFunctionTableAccess64,
                     SymGetModuleBase64, nullptr) ||
        !frame.AddrPC.Offset) {
      break;
    }
    REXLOG_ERROR("  #{:02} {}", i, Symbol(process, frame.AddrPC.Offset));
  }
  busy = false;
  return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

void InstallNullTrace(const uint8_t* guest_base) {
  if (!REXCVAR_GET(sptd_null_trace)) return;
  g_guest_base = reinterpret_cast<uintptr_t>(guest_base);
  AddVectoredExceptionHandler(1, OnException);
  REXLOG_INFO("SPTD: null-page trace on");
}

}  // namespace sptd

#else

#include <cstdint>

namespace sptd {
void InstallNullTrace(const uint8_t*) {}
}  // namespace sptd

#endif
