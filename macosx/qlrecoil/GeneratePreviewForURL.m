#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>

#import <Cocoa/Cocoa.h>

#include "qlrecoil.h"

OSStatus GeneratePreviewForURL(void *thisInterface, QLPreviewRequestRef preview, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options);
void CancelPreviewGeneration(void *thisInterface, QLPreviewRequestRef preview);

/* -----------------------------------------------------------------------------
   Generate a preview for file

   This function's job is to create preview for designated file
   ----------------------------------------------------------------------------- */

OSStatus GeneratePreviewForURL(void *thisInterface, QLPreviewRequestRef preview, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options)
{
    @autoreleasepool{
        CGImageRef image = RECOIL_createImage(url, false);
    
        if (!image)
        {
            return noErr;
        }
    
        size_t width = CGImageGetWidth(image);
        size_t height = CGImageGetHeight(image);
    
        CGContextRef context = QLPreviewRequestCreateContext(preview, CGSizeMake(width, height), false, NULL);
    
        if (!context)
        {
            return noErr;
        }
    
        CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
    
        CGImageRelease(image);
    
        QLPreviewRequestFlushContext(preview, context);
        CGContextRelease(context);
    }
    return noErr;
}

void CancelPreviewGeneration(void *thisInterface, QLPreviewRequestRef preview)
{
    // Implement only if supported
}
