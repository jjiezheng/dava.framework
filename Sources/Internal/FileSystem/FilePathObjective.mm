/*==================================================================================
    Copyright (c) 2008, DAVA Consulting, LLC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the DAVA Consulting, LLC nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE DAVA CONSULTING, LLC AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL DAVA CONSULTING, LLC BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    Revision History:
        * Created by Vitaliy Borodovsky 
=====================================================================================*/
#include "FileSystem/FilePath.h"

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)

#import <Foundation/NSString.h>
#import <Foundation/NSBundle.h>

namespace DAVA
{

	
#if defined(__DAVAENGINE_IPHONE__)
	NSString * FilepathRelativeToBundleObjC(const FilePath &virtualBundlePath, NSString * relativePathname)
	{
		NSString * filePath;
		if(virtualBundlePath.IsEmpty())
		{
			NSString * bundlePath = [[[NSBundle mainBundle] bundlePath] stringByAppendingString: @"/"];
			filePath = [bundlePath stringByAppendingString: relativePathname];
		}
		else
		{
			NSString * bundlePath = [NSString stringWithUTF8String: virtualBundlePath.GetAbsolutePathname().c_str()];
			filePath = [bundlePath stringByAppendingString: relativePathname];
		}
		
		return filePath;
	}
#elif defined(__DAVAENGINE_MACOS__)
    NSString * FilepathRelativeToBundleObjC(const FilePath &virtualBundlePath,
                                            NSString * relativePathname)
	{
        NSString * filePath;
        if(virtualBundlePath.IsEmpty())
        {
            NSString * bundlePath = [[[NSBundle mainBundle] bundlePath] stringByAppendingString: @"/Contents/Resources/"];
            filePath = [bundlePath stringByAppendingString: relativePathname];
        }
        else
        {
            NSString * bundlePath = [NSString stringWithUTF8String: virtualBundlePath.GetAbsolutePathname().c_str()];
            filePath = [bundlePath stringByAppendingString: relativePathname];
        }
		
		return filePath;
	}
#endif	//#elif defined(__DAVAENGINE_MACOS__)
	
    
    FilePath FilePath::FilepathRelativeToBundle(const char * relativePathname)
	{
		NSString * filePath = FilepathRelativeToBundleObjC(virtualBundlePath, [NSString stringWithUTF8String: relativePathname]);
		return [filePath UTF8String];
	}
}

#endif //#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)



