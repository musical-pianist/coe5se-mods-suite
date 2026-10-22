// The part of macOS's CoreFoundation that CoE5SE's Mac dylibs call, declared as in Apple's SDK.
#ifndef COE5SE_MAC_SDK_COREFOUNDATION_H
#define COE5SE_MAC_SDK_COREFOUNDATION_H

#include <stdint.h>

typedef int32_t SInt32;
typedef uint32_t UInt32;
typedef unsigned long CFOptionFlags;
typedef double CFTimeInterval;
typedef UInt32 CFStringEncoding;
typedef const void *CFTypeRef;
typedef const struct __CFAllocator *CFAllocatorRef;
typedef const struct __CFString *CFStringRef;
typedef const struct __CFURL *CFURLRef;

#define kCFStringEncodingUTF8 ((CFStringEncoding)0x08000100)

enum {
    kCFUserNotificationStopAlertLevel = 0,
    kCFUserNotificationNoteAlertLevel = 1,
    kCFUserNotificationCautionAlertLevel = 2,
    kCFUserNotificationPlainAlertLevel = 3
};
enum {
    kCFUserNotificationDefaultResponse = 0,
    kCFUserNotificationAlternateResponse = 1,
    kCFUserNotificationOtherResponse = 2,
    kCFUserNotificationCancelResponse = 3
};

// As in the SDK: a constant string the compiler lays out.
#define CFSTR(cStr) ((CFStringRef)__builtin___CFStringMakeConstantString("" cStr ""))

void CFRelease(CFTypeRef cf);
CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding);
SInt32 CFUserNotificationDisplayAlert(CFTimeInterval timeout, CFOptionFlags flags, CFURLRef iconURL, CFURLRef soundURL,
                                      CFURLRef localizationURL, CFStringRef alertHeader, CFStringRef alertMessage,
                                      CFStringRef defaultButtonTitle, CFStringRef alternateButtonTitle,
                                      CFStringRef otherButtonTitle, CFOptionFlags *responseFlags);

#endif
