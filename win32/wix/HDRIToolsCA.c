/* Custom actions for WiX */

#define WINDOWS_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <Shlobj.h>
#include <msi.h>
#include <intrin.h>

UINT __stdcall NotifyAssocChanged (MSIHANDLE handle)
{
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    return ERROR_SUCCESS;    
}

UINT __stdcall CheckHasAVX (MSIHANDLE handle)
{
#if (_MSC_FULL_VER < 160040219)
    return ERROR_INSTALL_FAILURE;
#else
    BOOL avxSupported, hasOSXSAVE, hasAVX;
    unsigned __int64 xcrFeatureMask;
    int cpuInfo[4];

    avxSupported = FALSE;
    
    /* Visual Studio 2010 SP1 or later */
    /* AVX requires XSAVE/XSTORE, AVX flag and support from the OS */
    __cpuid(cpuInfo, 1);
    hasOSXSAVE = (cpuInfo[2] & (1 << 27)) != 0;
    hasAVX     = (cpuInfo[2] & (1 << 28)) != 0;
    if (hasOSXSAVE && hasAVX) {
        /* Check if the OS is saving the AVX registers */
        xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        avxSupported = (xcrFeatureMask & 0x6) == 0x6;
    }
    return avxSupported ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
#endif
}
