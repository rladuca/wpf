// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include <shlwapi.h>
using namespace System;
using namespace System::ComponentModel;
using namespace System::Reflection;
using namespace System::Runtime::CompilerServices;
using namespace System::Runtime::InteropServices;
using namespace System::Security;
using namespace System::Diagnostics;

[assembly:DependencyAttribute("System,", LoadHint::Always)];
[assembly:DependencyAttribute("WindowsBase,", LoadHint::Always)];

#ifndef ARRAYSIZE
#define ARRAYSIZE RTL_NUMBER_OF_V2 // from DevDiv's WinNT.h
#endif

//
// Add a module-level initialization code here.
//
// The constructor of below class should be called before any other
// code in this Assembly when the assembly is loaded into any AppDomain.
//

//
// We want to call SetProcessDPIAware from user32.dll only on machines
// running Vista or later OSs.  We provide our own declaration (the
// original is in winuser.h) here so we can specify the DllImport attribute
// which allows delayed loading of the function - thereby allowing us to
// run on pre-Vista OSs.
//

[DllImport("user32.dll", EntryPoint="SetProcessDPIAware")]
WINUSERAPI
BOOL
WINAPI
SetProcessDPIAware_Internal(
    VOID);


#define WINNT_VISTA_VERSION     0x06

private class CModuleInitialize
{
public:

    // Constructor of class CModuleInitialize
    __declspec(noinline) CModuleInitialize(void (*cleaningUpFunc)())
    {
        IsProcessDpiAware();
        atexit(cleaningUpFunc);
    }

private :

    //
    // A private helper method to handle the DpiAwareness issue for current application.
    // This method is set as noinline since the MC++ compiler may otherwise inline it in a 
    // Security Transparent method which will lead to a security violation where the transparent
    // method will be calling security critical code in this method.
    //
    __declspec(noinline) void IsProcessDpiAware( )
    {
        Version  ^osVersion = (Environment::OSVersion)->Version;

        if (osVersion->Major < WINNT_VISTA_VERSION)
        {
            // DPIAware feature is available only in Vista and after.
            return;
        }

        //
        // Below code is only for Vista and newer platform.
        //
        Assembly ^ assemblyApp;
        Type ^  disableDpiAwareType = System::Windows::Media::DisableDpiAwarenessAttribute::typeid;
        bool    bDisableDpiAware = false;

        // By default, Application is DPIAware.
        assemblyApp = Assembly::GetEntryAssembly();

        // Check if the Application has explicitly set DisableDpiAwareness attribute.
        if (assemblyApp != nullptr && Attribute::IsDefined(assemblyApp, disableDpiAwareType))
        {
            bDisableDpiAware = true;
        }


        if (!bDisableDpiAware)
        {
            // DpiAware composition is enabled for this application.
            SetProcessDPIAware_Internal( );
        }

        // Only when DisableDpiAwareness attribute is set in Application assembly,
        // It will ignore the SetProcessDPIAware API call.
    }

};

void CleanUp();

/// <summary>
/// This method is a workaround to bug in the compiler.
/// The compiler generates a static unsafe method to initialize cmiStartupRunner
/// which is not properly annotated with security tags.
/// To work around this issue we create our own static method that is properly annotated.
/// </summary>
__declspec(noinline) static System::IntPtr CreateCModuleInitialize()
{
    return System::IntPtr(new CModuleInitialize(CleanUp));
}

// Important Note: This variable is declared as System::IntPtr to fool the compiler into creating
// a safe static method that initialzes it. If this variable was declared as CModuleInitialize
// Then the generated method is unsafe, fails NGENing and causes Jitting.
__declspec(appdomain) static System::IntPtr cmiStartupRunner = CreateCModuleInitialize();

void CleanUp()
{
    CModuleInitialize* pCmiStartupRunner = static_cast<CModuleInitialize*>(cmiStartupRunner.ToPointer());

    delete pCmiStartupRunner;
    cmiStartupRunner = System::IntPtr(NULL);
}
