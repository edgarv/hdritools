/* Custom actions for WiX */

#define WINDOWS_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <Shlobj.h>
#include <msi.h>

UINT __stdcall NotifyAssocChanged (MSIHANDLE handle)
{
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    return ERROR_SUCCESS;    
}
