#include <payload/payload.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mach-o/loader.h>
#include <CoreFoundation/CoreFoundation.h>
#include <pthread.h>
#include <libjailbreak/libjailbreak.h>
#include <dlfcn.h>

mach_port_t (*SBSSpringBoardServerPort)(void);
static CFRunLoopRef loop;

void sb_launched(CFNotificationCenterRef __unused center, void __unused *observer,
				 CFStringRef __unused name, const void __unused *object, CFDictionaryRef __unused info) {
    CFRunLoopStop(loop);
}

int launchdaemons(uint32_t payload_options, uint64_t pflags) {
    printf("plooshInit launchdaemons...\n");
    int platform = get_platform();
    if (platform == -1) {
        fprintf(stderr, "get_platform(): %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    if (platform == PLATFORM_IOS) {
        void *springboardservices = dlopen("/System/Library/PrivateFrameworks/SpringBoardServices.framework/SpringBoardServices", RTLD_NOW);
        if (springboardservices) {
            SBSSpringBoardServerPort = dlsym(springboardservices, "SBSSpringBoardServerPort");
            if (SBSSpringBoardServerPort && !MACH_PORT_VALID(SBSSpringBoardServerPort())) {
                loop = CFRunLoopGetCurrent();
                CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), NULL, &sb_launched, CFSTR("SBSpringBoardDidLaunchNotification"), NULL, 0);
                CFRunLoopRun();
            }
            dlclose(springboardservices);
        } else {
            fprintf(stderr, "failed to dlopen springboardservices\n");
        }
    } else if (platform == PLATFORM_TVOS) {
#if 0
        sleep(10);
        if ((payload_options & payload_option_userspace_rebooted) == 0) {
            xpc_object_t xdict, xreply;
            xdict = xpc_dictionary_create(NULL, NULL, 0);
            xpc_dictionary_set_uint64(xdict, "cmd", LAUNCHD_CMD_RUN_BOOTSCREEND);
            int ret = jailbreak_send_launchd_message(xdict, &xreply);
            if (ret) {
                print_jailbreakd_reply(xreply);
            }
            xpc_release(xdict);
            xpc_release(xreply);
        }
        sleep(5); // ???
#endif
        sleep(15);
    }

    if (pflags & palerain_option_safemode) {
        CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        if (dict) {
            CFDictionarySetValue(dict, kCFUserNotificationAlertHeaderKey, CFSTR("Entered Safe Mode"));
            if (pflags & palerain_option_failure)
                CFDictionarySetValue(dict, kCFUserNotificationAlertMessageKey, CFSTR(
                    "palera1n entered safe mode due to a failure\n\n"
                    "palera1n /did not/ cause this problem, rather, it has protected you from it\n\n"
                    "You can exit safe mode by clicking on exit safe mode in the palera1n app\n\n"
                    "If this issue persists, then most likely you have a bad tweak installed, and you should uninstall it from your package manager."
                ));
                else
            CFDictionarySetValue(dict, kCFUserNotificationAlertMessageKey, CFSTR("palera1n entered safe mode due to a user request"));

            CFUserNotificationRef notif = CFUserNotificationCreate(kCFAllocatorDefault, 0, 0, NULL, dict);
            CFRelease(dict);
            if (notif) CFRelease(notif);
        }
    }
    
    if (access("/usr/bin/uicache", F_OK) == 0)
        runCommand((char*[]){ "/usr/bin/uicache", "-a", NULL });
    else if (access("/var/jb/usr/bin/uicache", F_OK) == 0)
        runCommand((char*[]){ "/var/jb/usr/bin/uicache", "-a", NULL });
    else if (platform != PLATFORM_BRIDGEOS)
        runCommand((char*[]){ "/cores/binpack/usr/bin/uicache", "-a", NULL });

    /* just in case the above commands are bad, we run them last so the user can still get the loader */
    switch (platform) {
        case PLATFORM_IOS:
            runCommand((char*[]){ "/cores/binpack/usr/bin/uicache", "-p", "/cores/binpack/Applications/palera1nLoader.app", NULL });
            break;
        case PLATFORM_TVOS:
            runCommand((char*[]){ "/cores/binpack/usr/bin/uicache", "-p", "/cores/binpack/Applications/palera1nLoaderTV.app", NULL });
        case PLATFORM_BRIDGEOS:
            break;
        default:
            fprintf(stderr, "uicache_loader: unsupported platform\n");
    }


    printf("plooshInit launchdaemons: Goodbye!\n");
    return 0;
}
