// SHE library
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/win/pen.h"

#include "base/debug.h"
#include "base/fs.h"
#include "base/log.h"
#include "base/string.h"

#include <iostream>

typedef UINT (API* WTInfoW_Func)(UINT, UINT, LPVOID);
typedef HCTX (API* WTOpenW_Func)(HWND, LPLOGCONTEXTW, BOOL);
typedef BOOL (API* WTClose_Func)(HCTX);
typedef BOOL (API* WTPacket_Func)(HCTX, UINT, LPVOID);

namespace she {

static WTInfoW_Func WTInfo;
static WTOpenW_Func WTOpen;
static WTClose_Func WTClose;
static WTPacket_Func WTPacket;

PenAPI::PenAPI()
  : m_wintabLib(nullptr)
{
}

PenAPI::~PenAPI()
{
  if (!m_wintabLib)
    return;

  base::unload_dll(m_wintabLib);
  m_wintabLib = nullptr;
}

HCTX PenAPI::open(HWND hwnd)
{
  if (!m_wintabLib && !loadWintab())
    return nullptr;

  // Log Wintab ID
  {
    UINT nchars = WTInfo(WTI_INTERFACE, IFC_WINTABID, nullptr);
    if (nchars > 0 && nchars < 1024) {
      std::vector<WCHAR> buf(nchars+1, 0);
      WTInfo(WTI_INTERFACE, IFC_WINTABID, &buf[0]);
      LOG("PEN: Wintab ID \"%s\"\n", base::to_utf8(&buf[0]).c_str());
    }
  }

  // Log Wintab version for debugging purposes
  {
    WORD specVer = 0;
    WORD implVer = 0;
    UINT options = 0;
    WTInfo(WTI_INTERFACE, IFC_SPECVERSION, &specVer);
    WTInfo(WTI_INTERFACE, IFC_IMPLVERSION, &implVer);
    WTInfo(WTI_INTERFACE, IFC_CTXOPTIONS, &options);
    LOG("PEN: Wintab spec v%d.%d impl v%d.%d options 0x%x\n",
        (specVer & 0xff00) >> 8, (specVer & 0xff),
        (implVer & 0xff00) >> 8, (implVer & 0xff), options);
  }

  LOGCONTEXTW logctx;
  memset(&logctx, 0, sizeof(LOGCONTEXTW));
  UINT infoRes = WTInfo(WTI_DEFSYSCTX, 0, &logctx);

  // TODO Sometimes we receive infoRes=88 from WTInfo and logctx.lcOptions=0
  //      while sizeof(LOGCONTEXTW) is 212
  ASSERT(infoRes == sizeof(LOGCONTEXTW));
  ASSERT(logctx.lcOptions & CXO_SYSTEM);

#if 0 // We shouldn't bypass WTOpen() if the return value from
      // WTInfo() isn't the expected one, WTOpen() should just fail
      // anyway.
  if (infoRes != sizeof(LOGCONTEXTW)) {
    LOG(ERROR)
      << "PEN: Not supported WTInfo:\n"
      << "     Expected context size: " << sizeof(LOGCONTEXTW) << "\n"
      << "     Actual context size: " << infoRes << "\n"
      << "     Options: " << logctx.lcOptions << ")\n";
    return nullptr;
  }
#endif

  logctx.lcOptions |=
    CXO_MESSAGES |
    CXO_CSRMESSAGES;
  logctx.lcPktData = PACKETDATA;
  logctx.lcPktMode = PACKETMODE;
  logctx.lcMoveMask = PACKETDATA;

  LOG("PEN: Opening context, options 0x%x\n", logctx.lcOptions);
  HCTX ctx = WTOpen(hwnd, &logctx, TRUE);
  if (!ctx) {
    LOG("PEN: Error attaching pen to display\n");
    return nullptr;
  }

  LOG("PEN: Pen attached to display, new context %p\n", ctx);
  return ctx;
}

void PenAPI::close(HCTX ctx)
{
  LOG("PEN: Closing context %p\n", ctx);
  if (ctx) {
    ASSERT(m_wintabLib);
    LOG("PEN: Pen detached from window\n");
    WTClose(ctx);
  }
}

bool PenAPI::packet(HCTX ctx, UINT serial, LPVOID packet)
{
  return (WTPacket(ctx, serial, packet) ? true: false);
}

bool PenAPI::loadWintab()
{
  ASSERT(!m_wintabLib);

  m_wintabLib = base::load_dll("wintab32.dll");
  if (!m_wintabLib) {
    LOG(ERROR) << "PEN: wintab32.dll is not present\n";
    return false;
  }

  WTInfo = base::get_dll_proc<WTInfoW_Func>(m_wintabLib, "WTInfoW");
  WTOpen = base::get_dll_proc<WTOpenW_Func>(m_wintabLib, "WTOpenW");
  WTClose = base::get_dll_proc<WTClose_Func>(m_wintabLib, "WTClose");
  WTPacket = base::get_dll_proc<WTPacket_Func>(m_wintabLib, "WTPacket");
  if (!WTInfo || !WTOpen || !WTClose || !WTPacket) {
    LOG(ERROR) << "PEN: wintab32.dll does not contain all required functions\n";
    return false;
  }

  LOG("PEN: Wintab library loaded\n");
  return true;
}

} // namespace she
