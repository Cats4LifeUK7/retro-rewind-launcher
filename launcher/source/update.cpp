#include <wiisocket.h>
#include <stdio.h>
#include <curl/curl.h>
#include <mbedtls/sha1.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gctypes.h>
#include <gccore.h>
#include <fat.h>
#include <malloc.h>
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>
#include <sys/stat.h>
#include <sdcard/wiisd_io.h>
#include "update.h"
#include <files.h>
#include <errno.h>
#include <sha1.h>
#include "haxx.h"
#include "menu.h"
#include "launcher.h"
#include "riivolution_config.h"

#include "init.h"
#include <unistd.h>
#include <files.h>

using std::vector;

RiiDisc Disc2;
vector<int> Mounted2;
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define MAX_URL_LENGTH 256
#define MAX_FILENAME_LENGTH 128
#define MAX_DIRECTORY_LENGTH 128
#define MAX_VERSION_LENGTH 16
#define MAX_PATH_LENGTH 256
#define MAX_FIELD_LENGTH 112

bool done2 = false;
bool die2 = false;

void timetostop() { die2 = done2; }
static bool initfat = false;

struct memory {
  char *response;
  size_t size;
};

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    FILE *fp = (FILE *)userp;
    return fwrite(contents, size, nmemb, fp);
}

int compareVersions(const char *localVersion, const char *onlineVersion) {
    // Split version strings into components
    int localMajor, localMinor, localPatch;
    int onlineMajor, onlineMinor, onlinePatch;

    sscanf(localVersion, "%d.%d.%d", &localMajor, &localMinor, &localPatch);
    sscanf(onlineVersion, "%d.%d.%d", &onlineMajor, &onlineMinor, &onlinePatch);

    // Compare major version
    if (localMajor < onlineMajor) {
        return -1;
    } else if (localMajor > onlineMajor) {
        return 1;
    }

    // Compare minor version
    if (localMinor < onlineMinor) {
        return -1;
    } else if (localMinor > onlineMinor) {
        return 1;
    }

    // Compare patch version
    if (localPatch < onlinePatch) {
        return -1;
    } else if (localPatch > onlinePatch) {
        return 1;
    }

    // Versions are equal
    return 0;
}

// Modify downloadFile function to create directory if needed
void downloadFile(const char *url, const char *outputFilename) {
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {

        FILE *fp = fopen(outputFilename, "wb");
        if (fp == NULL) {
            printf("Error opening file %s for writing.\n", outputFilename);
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
        fclose(fp);
    }

    curl_global_cleanup();
}

void updateLocalVersion(const char *newVersion) {
    FILE *localVersionFile = fopen("PUT SD CARD VERSION TXT HERE", "wb");
    if (localVersionFile == NULL) {
        printf("Error opening version.txt for writing.\n");
        exit(0);
    }

    fprintf(localVersionFile, "%s", newVersion);
    fclose(localVersionFile);
}

void downloadFilesFromVersionFile(const char *versionFileURL, const char *localVersion) {
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        FILE *versionFile = fopen("PUT SD CARD FILELIST TXT HERE", "wb");
        if (versionFile == NULL) {
            printf("Error opening version.txt for writing.\n");
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, versionFileURL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, versionFile);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        fclose(versionFile);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    printf("Contents of version.txt:\n");
    FILE *versionFileContents = fopen("PUT SD CARD FILELIST TXT HERE", "r");
    if (versionFileContents) {
        char onlineVersion[MAX_VERSION_LENGTH];
        char url[MAX_URL_LENGTH];
        char outputFilename[MAX_FILENAME_LENGTH];
        char directoryName[MAX_FIELD_LENGTH];

        while (fscanf(versionFileContents, "%s %s %s %s", onlineVersion, url, outputFilename, directoryName) == 4) {
            // Construct the full path including the directory
            char fullPath[MAX_FILENAME_LENGTH];
            snprintf(fullPath, sizeof(fullPath), "sd:/RetroRewind/%s", directoryName);

            // Extract directory path from the filePath
            char directory[MAX_FILENAME_LENGTH];
            strncpy(directory, fullPath, strrchr(fullPath, '/') - fullPath);
            directory[strrchr(fullPath, '/') - fullPath] = '\0';

            mkdir(fullPath, 0777);

            int comparisonResult = compareVersions(localVersion, onlineVersion);

            if (comparisonResult < 0) {
                downloadFile(url, outputFilename);
                printf("Downloaded: %s\n", outputFilename);
                updateLocalVersion(onlineVersion);
            } else if (comparisonResult == 0) {
                printf("Local version is up-to-date.\n");
            } else {
                printf("Skipping download for version %s (already up-to-date).\n", onlineVersion);
            }
        }

        fclose(versionFileContents);
    }
}

void DeleteFilesFromVersionFile(const char *DeleteFileURL) {
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        FILE *DeleteFile = fopen("PUT DELETION FILE TXT HERE", "wb");
        if (DeleteFile == NULL) {
            printf("Error opening delete.txt for writing.\n");
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, DeleteFileURL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, DeleteFile);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        fclose(DeleteFile);
        curl_easy_cleanup(curl);
    }

    FILE *DeleteFileContents = fopen("PUT DELETION FILE TXT HERE", "r");
    if (DeleteFileContents) {
        char FileToDelete[MAX_DIRECTORY_LENGTH];

        while (fscanf(DeleteFileContents, "%s", FileToDelete) == 1) {
            unlink(FileToDelete);
        }

        unlink("PUT DELETION FILE TXT HERE");
        fclose(DeleteFileContents);
    }
}

void UpdateIsConfirmed() {
    Haxx_UnMount(&Mounted2);
	initfat = fatInitDefault();
	if (!initfat) {
	    curl_global_init(CURL_GLOBAL_ALL);
	    VIDEO_Init();
	    rmode = VIDEO_GetPreferredMode(NULL);
	    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	    console_init(xfb, 0, 0, rmode->fbWidth, rmode->xfbHeight,
		            rmode->fbWidth * VI_DISPLAY_PIX_SZ);
	    VIDEO_Configure(rmode);
	    VIDEO_SetNextFramebuffer(xfb);
	    VIDEO_SetBlack(FALSE);
	    VIDEO_Flush();
	    VIDEO_WaitVSync();
	    if (rmode->viTVMode & VI_NON_INTERLACE) {
		    VIDEO_WaitVSync();
	    }

        printf("failure\n");
        return;
    } else {
	    curl_global_init(CURL_GLOBAL_ALL);

	    printf("libcurl version: %s\n", curl_version());

	    // try a few times to initialize libwiisocket (?)
	    int socket_init_success = -1;
	    for (int attempts = 0;attempts < 3;attempts++) {
		    socket_init_success = wiisocket_init();
		    printf("attempt: %d wiisocket_init: %d\n", attempts, socket_init_success);
		    if (socket_init_success == 0)
			    break;
	    }
	    if (socket_init_success != 0) {
		    puts("failed to init wiisocket");
		    return;
	    }
	    // try a few times to get an ip (?)
	    u32 ip = 0;
	    for (int attempts = 0; attempts < 3; attempts++) {
		    ip = gethostid();
		    printf("attempt: %d gethostid: %x\n", attempts, ip);
		    if (ip)
			    break;
	    }
	    if (!ip) {
		    puts("failed to get ip");
		    return;
	    }

        const char *fileToDelete = "PUT LINK FOR DELETE HERE";

        const char *fileListURL = "PUT LINK FOR FILELIST HERE";

        // Read local version from a local file
        FILE *localVersionFile = fopen("PUT SD CARD VERSION TXT HERE", "r");
        if (localVersionFile == NULL) {
            printf("Error opening version.txt for reading.\n");
            return;
        }

        // Delete files
        DeleteFilesFromVersionFile(fileToDelete);

        char localVersion[MAX_VERSION_LENGTH];
        fscanf(localVersionFile, "%s", localVersion);
        fclose(localVersionFile);

        // Download version.txt (which also contains filelist)
        downloadFilesFromVersionFile(fileListURL, localVersion);

        FILE *fileListFile = fopen("PUT SD CARD FILELIST TXT HERE", "r");
        if (fileListFile == NULL) {
            printf("Error opening version2.txt for reading.\n");
            return;
        }

        char url[MAX_URL_LENGTH];
        char outputFilename[MAX_FILENAME_LENGTH];

        // Download files listed in version.txt (filelist.txt)
        while (fscanf(fileListFile, "%s %s", url, outputFilename) == 2) {
            downloadFile(url, outputFilename);
            printf("Downloaded: %s\n", outputFilename);
        }
        unlink("DELETE FILELIST HERE");
        fclose(fileListFile);
    }
    return;
}