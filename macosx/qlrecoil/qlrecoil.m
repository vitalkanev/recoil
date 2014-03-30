#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>

#import <Cocoa/Cocoa.h>

#include "../../recoil.h"


static RECOIL *recoil = NULL;


static void rgba(unsigned char *dest, const int *src, int length)
{
	int i;
	for (i = 0; i < length; i++) {
        int rgb = src[i];
        dest[4*i] = (rgb >> 16);
        dest[4*i+1] = (rgb >> 8);
        dest[4*i+2] = rgb;
        dest[4*i+3] = 0;
	}
}


CGImageRef RECOIL_createImage(CFURLRef url, bool withAlpha)
{
    CGImageRef image;
    recoil = RECOIL_New();
    @autoreleasepool{
        
        if (recoil == NULL){
            printf(" RECOIL failed to initialize ");
            return NULL;
        }
        
        const NSDataReadingOptions DataReadingOptions = NSDataReadingUncached;
        NSError * outError = nil;
        NSData * data = [NSData dataWithContentsOfURL:(__bridge NSURL *)(url)
                                              options:DataReadingOptions
                                                error:&outError];
        if (data == NULL){
            return NULL;
        }
        int length = data.length;
        const void * bytePtr = data.bytes;
        
        CFStringRef filenameRef = CFURLCopyLastPathComponent(url);
        if(filenameRef==NULL){
            return NULL;
        }
        const char *filename = CFStringGetCStringPtr( filenameRef, kCFStringEncodingUTF8 );
 
        printf(" filename: ");
        printf("%s",filename);
        printf(" length: ");
        printf("%i",length);
        
        if (!RECOIL_Decode(recoil, filename, bytePtr, length)){
            printf("qlrecoil failed to decode");
            return NULL;
        }
        
        CGSize size = CGSizeMake(RECOIL_GetWidth(recoil), RECOIL_GetHeight(recoil));
        printf(" witdh: ");
        printf("%f",size.width);
        printf(" height: ");
        printf("%f",size.height);
       
        const int *recoilPixels = RECOIL_GetPixels(recoil);

        unsigned char indexes[RECOIL_MAX_PIXELS_LENGTH];
        const int *palette;
        unsigned char *pixels = (unsigned char*)malloc(size.width*size.height*4);
        
        palette = RECOIL_ToPalette(recoil, indexes);
        if (palette == NULL) {
            printf(" no palette ");
        }else{
            int colors = RECOIL_GetColors(recoil);
            int bit_depth = colors <= 2 ? 1
                          : colors <= 4 ? 2
                          : colors <= 16 ? 4
                          : 8;
            printf(" palette colors ");
            printf("%d",colors);
            printf(" bit_depth ");
            printf("%d",bit_depth);
            printf("  ");
        }
        rgba(pixels,recoilPixels,size.width*size.height);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        
        if (!colorSpace){
          printf(" qlrecoil failed to create colorspace ");
          return NULL;
        }
        
        CGContextRef context = CGBitmapContextCreate(pixels, size.width, size.height, 8, 4*size.width, colorSpace,
                                                     kCGImageAlphaNoneSkipLast);
    
        if (!context){
            printf(" qlrecoil failed to create context ");
            return NULL;
        }
            
        image = CGBitmapContextCreateImage(context);
    
        CGColorSpaceRelease(colorSpace);
        CGContextRelease(context);
        CFRelease(filenameRef);
        free(pixels);
    }
    RECOIL_Delete(recoil);

    return image;
}