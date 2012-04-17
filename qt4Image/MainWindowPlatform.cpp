/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2012 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

// Platform specific code

#if defined(_WIN32)
# define WINDOWS_LEAN_AND_MEAN
# include <Windows.h>
# include <Shobjidl.h>
#endif

#include "MainWindow.h"

#include <QDebug>
#include <QSysInfo>



namespace
{
#if defined(_WIN32)
static const WCHAR* APP_REGISTRY_NAME = L"PCG.qt4Image.1";
#endif
} // namespace



void MainWindow::fileAssociations()
{
#if defined(_WIN32)
    IApplicationAssociationRegistrationUI *pUI = NULL;
    const IID iid = __uuidof(IApplicationAssociationRegistrationUI);
    HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationRegistrationUI,
        NULL, CLSCTX_INPROC /*context*/, iid, reinterpret_cast<void**>(&pUI));
    if (SUCCEEDED(hr)) {
        hr = pUI->LaunchAdvancedAssociationUI(L"PCG.qt4Image.1");
        if (FAILED(hr)) {
            qDebug() << "LaunchAdvancedAssociationUI(" << APP_REGISTRY_NAME
                << ") error: " << hr;
        }
        pUI->Release();
    } else {
        qDebug() << "IApplicationAssociationRegistrationUI error: " << hr;
    }
#endif
}
