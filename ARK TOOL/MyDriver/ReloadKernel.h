#pragma once
#include "ntifs.h"
#include <ntimage.h>



NTSTATUS reloadNT(PDRIVER_OBJECT driver);

VOID InstallSysenterHook();
void unstallKiFastCallEntryHook();
void installKiFastCallEntryHook();
VOID UnstallSysenterHook();

