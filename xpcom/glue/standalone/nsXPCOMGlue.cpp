/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:set ts=4 sw=4 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsXPCOMGlue.h"
#include "nsGlueLinking.h"

#include "nspr.h"
#include "nsDebug.h"
#include "nsIServiceManager.h"
#include "nsGREDirServiceProvider.h"
#include "nsXPCOMPrivate.h"
#include "nsCOMPtr.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef XP_WIN
#include <windows.h>
#include <mbstring.h>
#include <malloc.h>
#define snprintf _snprintf
#endif

// functions provided by nsDebug.cpp
nsresult GlueStartupDebug();
void GlueShutdownDebug();

static XPCOMFunctions xpcomFunctions;

extern "C"
nsresult XPCOMGlueStartup(const char* xpcomFile)
{
    xpcomFunctions.version = XPCOM_GLUE_VERSION;
    xpcomFunctions.size    = sizeof(XPCOMFunctions);

    GetFrozenFunctionsFunc func = nsnull;

    if (!xpcomFile)
        xpcomFile = XPCOM_DLL;

    func = XPCOMGlueLoad(xpcomFile);

    if (!func)
        return NS_ERROR_FAILURE;

    nsresult rv = (*func)(&xpcomFunctions, nsnull);
    if (NS_FAILED(rv)) {
        XPCOMGlueUnload();
        return rv;
    }

    rv = GlueStartupDebug();
    if (NS_FAILED(rv)) {
        memset(&xpcomFunctions, 0, sizeof(xpcomFunctions));
        XPCOMGlueUnload();
        return rv;
    }

    return NS_OK;
}

#if defined(XP_WIN) || defined(XP_OS2)
#define READ_TEXTMODE "t"
#else
#define READ_TEXTMODE
#endif

void
XPCOMGlueLoadDependentLibs(const char *xpcomDir, DependentLibsCallback cb)
{
    char buffer[MAXPATHLEN];
    sprintf(buffer, "%s" XPCOM_FILE_PATH_SEPARATOR XPCOM_DEPENDENT_LIBS_LIST,
            xpcomDir);

    FILE *flist = fopen(buffer, "r" READ_TEXTMODE);
    if (!flist)
        return;

    while (fgets(buffer, sizeof(buffer), flist)) {
        int l = strlen(buffer);

        // ignore empty lines and comments
        if (l == 0 || *buffer == '#')
            continue;

        // cut the trailing newline, if present
        if (buffer[l - 1] == '\n')
            buffer[l - 1] = '\0';

        char buffer2[MAXPATHLEN];
        snprintf(buffer2, sizeof(buffer2),
                 "%s" XPCOM_FILE_PATH_SEPARATOR "%s",
                 xpcomDir, buffer);
        cb(buffer2);
    }

    fclose(flist);
}

extern "C"
nsresult XPCOMGlueShutdown()
{
    GlueShutdownDebug();

    XPCOMGlueUnload();
    
    memset(&xpcomFunctions, 0, sizeof(xpcomFunctions));
    return NS_OK;
}

extern "C" NS_COM nsresult
NS_InitXPCOM2(nsIServiceManager* *result, 
              nsIFile* binDirectory,
              nsIDirectoryServiceProvider* appFileLocationProvider)
{
    if (!xpcomFunctions.init)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.init(result, binDirectory, appFileLocationProvider);
}

extern "C" NS_COM nsresult
NS_InitXPCOM3(nsIServiceManager* *result,
              nsIFile* binDirectory,
              nsIDirectoryServiceProvider* appFileLocationProvider,
              nsStaticModuleInfo const *staticComponents,
              PRUint32 componentCount)
{
    if (!xpcomFunctions.init3)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.init3(result, binDirectory, appFileLocationProvider,
                                staticComponents, componentCount);
}

extern "C" NS_COM nsresult
NS_ShutdownXPCOM(nsIServiceManager* servMgr)
{
    if (!xpcomFunctions.shutdown)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.shutdown(servMgr);
}

extern "C" NS_COM nsresult
NS_GetServiceManager(nsIServiceManager* *result)
{
    if (!xpcomFunctions.getServiceManager)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.getServiceManager(result);
}

extern "C" NS_COM nsresult
NS_GetComponentManager(nsIComponentManager* *result)
{
    if (!xpcomFunctions.getComponentManager)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.getComponentManager(result);
}

extern "C" NS_COM nsresult
NS_GetComponentRegistrar(nsIComponentRegistrar* *result)
{
    if (!xpcomFunctions.getComponentRegistrar)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.getComponentRegistrar(result);
}

extern "C" NS_COM nsresult
NS_GetMemoryManager(nsIMemory* *result)
{
    if (!xpcomFunctions.getMemoryManager)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.getMemoryManager(result);
}

extern "C" NS_COM nsresult
NS_NewLocalFile(const nsAString &path, PRBool followLinks, nsILocalFile* *result)
{
    if (!xpcomFunctions.newLocalFile)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.newLocalFile(path, followLinks, result);
}

extern "C" NS_COM nsresult
NS_NewNativeLocalFile(const nsACString &path, PRBool followLinks, nsILocalFile* *result)
{
    if (!xpcomFunctions.newNativeLocalFile)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.newNativeLocalFile(path, followLinks, result);
}

extern "C" NS_COM nsresult
NS_RegisterXPCOMExitRoutine(XPCOMExitRoutine exitRoutine, PRUint32 priority)
{
    if (!xpcomFunctions.registerExitRoutine)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.registerExitRoutine(exitRoutine, priority);
}

extern "C" NS_COM nsresult
NS_UnregisterXPCOMExitRoutine(XPCOMExitRoutine exitRoutine)
{
    if (!xpcomFunctions.unregisterExitRoutine)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.unregisterExitRoutine(exitRoutine);
}

extern "C" NS_COM nsresult
NS_GetDebug(nsIDebug* *result)
{
    if (!xpcomFunctions.getDebug)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.getDebug(result);
}


extern "C" NS_COM nsresult
NS_GetTraceRefcnt(nsITraceRefcnt* *result)
{
    if (!xpcomFunctions.getTraceRefcnt)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.getTraceRefcnt(result);
}


extern "C" NS_COM nsresult
NS_StringContainerInit(nsStringContainer &aStr)
{
    if (!xpcomFunctions.stringContainerInit)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.stringContainerInit(aStr);
}

extern "C" NS_COM nsresult
NS_StringContainerInit2(nsStringContainer &aStr,
                        const PRUnichar   *aData,
                        PRUint32           aDataLength,
                        PRUint32           aFlags)
{
    if (!xpcomFunctions.stringContainerInit2)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.stringContainerInit2(aStr, aData, aDataLength, aFlags);
}

extern "C" NS_COM void
NS_StringContainerFinish(nsStringContainer &aStr)
{
    if (xpcomFunctions.stringContainerFinish)
        xpcomFunctions.stringContainerFinish(aStr);
}

extern "C" NS_COM PRUint32
NS_StringGetData(const nsAString &aStr, const PRUnichar **aBuf, PRBool *aTerm)
{
    if (!xpcomFunctions.stringGetData) {
        *aBuf = nsnull;
        return 0;
    }
    return xpcomFunctions.stringGetData(aStr, aBuf, aTerm);
}

extern "C" NS_COM PRUint32
NS_StringGetMutableData(nsAString &aStr, PRUint32 aLen, PRUnichar **aBuf)
{
    if (!xpcomFunctions.stringGetMutableData) {
        *aBuf = nsnull;
        return 0;
    }
    return xpcomFunctions.stringGetMutableData(aStr, aLen, aBuf);
}

extern "C" NS_COM PRUnichar *
NS_StringCloneData(const nsAString &aStr)
{
    if (!xpcomFunctions.stringCloneData)
        return nsnull;
    return xpcomFunctions.stringCloneData(aStr);
}

extern "C" NS_COM nsresult
NS_StringSetData(nsAString &aStr, const PRUnichar *aBuf, PRUint32 aCount)
{
    if (!xpcomFunctions.stringSetData)
        return NS_ERROR_NOT_INITIALIZED;

    return xpcomFunctions.stringSetData(aStr, aBuf, aCount);
}

extern "C" NS_COM nsresult
NS_StringSetDataRange(nsAString &aStr, PRUint32 aCutStart, PRUint32 aCutLength,
                      const PRUnichar *aBuf, PRUint32 aCount)
{
    if (!xpcomFunctions.stringSetDataRange)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.stringSetDataRange(aStr, aCutStart, aCutLength, aBuf, aCount);
}

extern "C" NS_COM nsresult
NS_StringCopy(nsAString &aDest, const nsAString &aSrc)
{
    if (!xpcomFunctions.stringCopy)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.stringCopy(aDest, aSrc);
}


extern "C" NS_COM nsresult
NS_CStringContainerInit(nsCStringContainer &aStr)
{
    if (!xpcomFunctions.cstringContainerInit)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.cstringContainerInit(aStr);
}

extern "C" NS_COM nsresult
NS_CStringContainerInit2(nsCStringContainer &aStr,
                         const char         *aData,
                         PRUint32           aDataLength,
                         PRUint32           aFlags)
{
    if (!xpcomFunctions.cstringContainerInit2)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.cstringContainerInit2(aStr, aData, aDataLength, aFlags);
}

extern "C" NS_COM void
NS_CStringContainerFinish(nsCStringContainer &aStr)
{
    if (xpcomFunctions.cstringContainerFinish)
        xpcomFunctions.cstringContainerFinish(aStr);
}

extern "C" NS_COM PRUint32
NS_CStringGetData(const nsACString &aStr, const char **aBuf, PRBool *aTerm)
{
    if (!xpcomFunctions.cstringGetData) {
        *aBuf = nsnull;
        return 0;
    }
    return xpcomFunctions.cstringGetData(aStr, aBuf, aTerm);
}

extern "C" NS_COM PRUint32
NS_CStringGetMutableData(nsACString &aStr, PRUint32 aLen, char **aBuf)
{
    if (!xpcomFunctions.cstringGetMutableData) {
        *aBuf = nsnull;
        return 0;
    }
    return xpcomFunctions.cstringGetMutableData(aStr, aLen, aBuf);
}

extern "C" NS_COM char *
NS_CStringCloneData(const nsACString &aStr)
{
    if (!xpcomFunctions.cstringCloneData)
        return nsnull;
    return xpcomFunctions.cstringCloneData(aStr);
}

extern "C" NS_COM nsresult
NS_CStringSetData(nsACString &aStr, const char *aBuf, PRUint32 aCount)
{
    if (!xpcomFunctions.cstringSetData)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.cstringSetData(aStr, aBuf, aCount);
}

extern "C" NS_COM nsresult
NS_CStringSetDataRange(nsACString &aStr, PRUint32 aCutStart, PRUint32 aCutLength,
                       const char *aBuf, PRUint32 aCount)
{
    if (!xpcomFunctions.cstringSetDataRange)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.cstringSetDataRange(aStr, aCutStart, aCutLength, aBuf, aCount);
}

extern "C" NS_COM nsresult
NS_CStringCopy(nsACString &aDest, const nsACString &aSrc)
{
    if (!xpcomFunctions.cstringCopy)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.cstringCopy(aDest, aSrc);
}

extern "C" NS_COM nsresult
NS_CStringToUTF16(const nsACString &aSrc, nsCStringEncoding aSrcEncoding, nsAString &aDest)
{
    if (!xpcomFunctions.cstringToUTF16)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.cstringToUTF16(aSrc, aSrcEncoding, aDest);
}

extern "C" NS_COM nsresult
NS_UTF16ToCString(const nsAString &aSrc, nsCStringEncoding aDestEncoding, nsACString &aDest)
{
    if (!xpcomFunctions.utf16ToCString)
        return NS_ERROR_NOT_INITIALIZED;
    return xpcomFunctions.utf16ToCString(aSrc, aDestEncoding, aDest);
}

extern "C" NS_COM void*
NS_Alloc(PRSize size)
{
    if (!xpcomFunctions.allocFunc)
        return nsnull;
    return xpcomFunctions.allocFunc(size);
}

extern "C" NS_COM void*
NS_Realloc(void* ptr, PRSize size)
{
    if (!xpcomFunctions.reallocFunc)
        return nsnull;
    return xpcomFunctions.reallocFunc(ptr, size);
}

extern "C" NS_COM void
NS_Free(void* ptr)
{
    if (xpcomFunctions.freeFunc)
        xpcomFunctions.freeFunc(ptr);
}

// Default GRE startup/shutdown code

extern "C"
nsresult GRE_Startup()
{
    const char* xpcomLocation = GRE_GetXPCOMPath();

    // Startup the XPCOM Glue that links us up with XPCOM.
    nsresult rv = XPCOMGlueStartup(xpcomLocation);
    
    if (NS_FAILED(rv)) {
        NS_WARNING("gre: XPCOMGlueStartup failed");
        return rv;
    }

#ifdef XP_WIN
    // On windows we have legacy GRE code that does not load the GRE dependent
    // libs (seamonkey GRE, not libxul)... add the GRE to the PATH.
    // See bug 301043.

    const char *lastSlash = strrchr(xpcomLocation, '\\');
    if (lastSlash) {
        int xpcomPathLen = lastSlash - xpcomLocation;
        DWORD pathLen = GetEnvironmentVariable("PATH", nsnull, 0);

        char *newPath = (char*) _alloca(xpcomPathLen + pathLen + 1);
        strncpy(newPath, xpcomLocation, xpcomPathLen);
        // in case GetEnvironmentVariable fails
        newPath[xpcomPathLen] = ';';
        newPath[xpcomPathLen + 1] = '\0';

        GetEnvironmentVariable("PATH", newPath + xpcomPathLen + 1, pathLen);
        SetEnvironmentVariable("PATH", newPath);
    }
#endif

    nsGREDirServiceProvider *provider = new nsGREDirServiceProvider();
    if ( !provider ) {
        NS_WARNING("GRE_Startup failed");
        XPCOMGlueShutdown();
        return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIServiceManager> servMan;
    NS_ADDREF( provider );
    rv = NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, provider);
    NS_RELEASE(provider);

    if ( NS_FAILED(rv) || !servMan) {
        NS_WARNING("gre: NS_InitXPCOM failed");
        XPCOMGlueShutdown();
        return rv;
    }

    return NS_OK;
}

extern "C"
nsresult GRE_Shutdown()
{
    NS_ShutdownXPCOM(nsnull);
    XPCOMGlueShutdown();
    return NS_OK;
}
