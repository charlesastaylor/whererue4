// Wild.
#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>

#include "Windows.h"
#include "Shlobj.h"

#define Assert(expression, error) if (!(expression)) { \
printf(error); \
__debugbreak(); \
} \

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

struct String {
    size_t length;
    char* data;
};

#define StringLiteral(s) {sizeof(s) - 1, s}
#define StringExpand(s) (s32)s.length, s.data

// allocates!!
// @HACK xd, the use of this is used to fopen a file so need a null terminated c string... oops...
// @TODO think I decided that just easier to store all strings as null terminated even though internally I'll always
//       just use length param instead anyway. Hadn't done this for the string_copy's because forgot
String string_concat_null_terminated(String s, String t) {
    String result = {0};
    result.length = s.length + t.length;
    result.data   = (char*)malloc(result.length + 1);
    char* tmp = result.data;
    s32 i = 0;
    while(i++ < s.length) *tmp++ = *s.data++;
    i = 0;
    while (i++ < t.length) *tmp++ = *t.data++;
    *tmp = 0;
    return result;
}

bool string_equals(String s, String t) {
    if (s.length != t.length) return false;
    for (s32 i = 0; i < s.length; i++) {
        if (s.data[i] != t.data[i]) return false;
    }
    return true;
}

bool string_starts_with(String s, String suffix) {
    if (s.length < suffix.length) return false;
    s.length = suffix.length;
    return string_equals(s, suffix);
}

void string_replace_in_place(String s, char from, char to) {
    // @TODO how do for each "iterator" over string chars?
    for(s32 i = 0; i < s.length; i++) {
        if (s.data[i] == from) s.data[i] = to;
    }
}

// allocates and copies string 
String string_copy(size_t length, char* data) {
    //@TODO don't use malloc
    String result = {0};
    result.length = length;
    result.data = (char*)malloc(length);
    char* tmp = result.data;
    for (s32 i = 0; i < length; i++) {
        *tmp++ = *data++;
    }
    return result;
}

// allocates and copies wise string 
String string_copy(size_t length, wchar_t* data) {
    //@TODO don't use malloc
    String result = {0};
    result.length = length;
    result.data = (char*)malloc(length);
    char* tmp = result.data;
    for (s32 i = 0; i < length; i++) {
        // @TODO better to use wcstombs_s apparently :shrug:
        Assert(*data >= 0 && *data < 128, "Invalied character");
        *tmp++ = (char)*data++;
    }
    return result;
}

// @TODO get rid of use of reference. Just couldnt figure out the syntax for using double pointer
bool find_next_string(char*& data, String* found) {
    // Look for opening "
    char *start, *end;
    while (1) {
        if (*data == 0) return false;
        if (*data++ == '"') break;
    }
    start = data; // what if " is end of file...
    
    // Look for closign "
    while (1) {
        if (*data == 0) return false;
        if (*data++ == '"') break;
    } 
    end = data - 1;
    
    found->data = start;
    found->length = end - start;
    return true;
}


int main(int num_args, char *args[]) {
    String engine_version = {};
    if (num_args > 1) {
        if (num_args > 2) {
            printf("Too many arguments!\n");
            return 1;
        }
        
        // @TODO check arugment is of form x.y and x >= 4
        engine_version.data = args[1];
        engine_version.length = strlen(engine_version.data);
    }
    
    // Find launcher installed data file path
    wchar_t* program_data_path_wide;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, 0, &program_data_path_wide);
    if (SUCCEEDED(result)) {
        String installed_list_relative_dir = StringLiteral("\\Epic\\UnrealEngineLauncher\\LauncherInstalled.dat");
        String program_data_path = string_copy(wcslen(program_data_path_wide), program_data_path_wide);
        String launcher_installed_data_path = string_concat_null_terminated(program_data_path, installed_list_relative_dir);
        //printf("%s\n", launcher_installed_data_path);
        
        // Open file
        FILE* file = fopen(launcher_installed_data_path.data, "r");
        Assert(file, "Failed to launcher data file!\n");
        
        fseek(file, 0, SEEK_END);
        s32 file_size = ftell(file);
        Assert(file_size != -1, "um");
        fseek(file, 0, SEEK_SET);
        
        char* launcher_installed_data = (char*)malloc(file_size + 1);
        fread(launcher_installed_data, 1, file_size, file);
        launcher_installed_data[file_size] = 0;
        //printf("%.*s\n ...\n\n", 50, launcher_installed_data);
        
        //
        // Parse file to find app name and install location
        //
        // @TODO dont store apps not starting with UE_
        char* head = launcher_installed_data;
        
        String INSTALL_LOCATION = StringLiteral("InstallLocation");
        String APP_NAME = StringLiteral("AppName");
        
#define MAX_APPS 100 // @TODO dynamic?
        String install_locations[MAX_APPS];
        String app_names[MAX_APPS];
        s32 location_i = 0;
        s32 app_i = 0;
        
        String s = {};
        while (find_next_string(head, &s)) {
            Assert(location_i < MAX_APPS, "Too many launcher installed things bruh\n");
            if (string_equals(s, INSTALL_LOCATION)) {
                String t = {};
                Assert(find_next_string(head, &t), "Failed to find matching app name");
                
                // Remove escaped backslashes
                // NB: This is modifying the file data in place!
                s32 offset = 0;
                for (s32 i = 0; i < t.length - offset; i++) {
                    if (t.data[i + offset] == '\\') offset += 1;
                    t.data[i] = t.data[i + offset];
                }
                t.length -= offset;
                
                install_locations[location_i++] = t;
            } else if (string_equals(s, APP_NAME)) {
                String t = {};
                Assert(find_next_string(head, &t), "Failed to find matching install location");
                app_names[app_i++] = t;
            }
        }
        
        Assert(location_i == app_i, "install location / app name mismatch");
        
        
        //
        // Enumerate the per-user installations
        //
        HKEY key_handle;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Epic Games\\Unreal Engine\\Builds", 0, KEY_ALL_ACCESS, &key_handle) == ERROR_SUCCESS) {
            
            // Enumerate all the installations
            //TArray<FString> InvalidKeys;
            for (DWORD index = 0;; index++) {
                char value_name[256];
                char value_data[MAX_PATH];
                DWORD value_type = 0;
                DWORD value_name_length = sizeof(value_name) / sizeof(value_name[0]);
                DWORD value_data_size = sizeof(value_data);
                
                LRESULT reg_result = RegEnumValueA(key_handle, index, value_name, &value_name_length, 0, &value_type, (BYTE*)&value_data[0], &value_data_size);
                if (reg_result == ERROR_SUCCESS) {
                    Assert(value_type == REG_SZ, "unexpected reg value type\n");
                    
                    String name = string_copy(value_name_length, value_name); // Not using install id atm
                    String data = string_copy(value_data_size, value_data);
                    string_replace_in_place(data, '/', '\\');
                    // @HACK to fit in with how im filtering apps below @TODO just dont put not engine installs in the arrays above
                    app_names[app_i++] = StringLiteral("UE_Source");
                    install_locations[location_i++] = data;
                    //printf("name: %.*s, value: %.*s\n", StringExpand(name), StringExpand(data));
                    
                    // @TODO "normalize" the data, eg paths might have incorrect types of slashes in \\ -> \
                    // @TODO check directory is valid engine dir:
                    //       - Check that there's an Engine\Binaries directory underneath the root
                    //       - Also check there's an Engine\Build directory. This will filter out anything that has an engine-like directory structure but doesn't allow building code projects - like the launcher.
                }
                else if(reg_result == ERROR_NO_MORE_ITEMS) break;
            }
            
            RegCloseKey(key_handle);
        }
        
        //
        // Print results
        //
        for (s32 i = 0; i < location_i; i++) {
            if (string_starts_with(app_names[i], StringLiteral("UE_"))) {
                // cut off "UE_"
                String tmp = app_names[i];
                tmp.data += 3;
                tmp.length -= 3;
                if (engine_version.data) {
                    if (string_equals(engine_version, tmp)) {
                        printf("%.*s\n", StringExpand(install_locations[i]));
                        goto End; // @TODO could just return here if decide that dont care about freeing program_data_path_wide
                    }
                } else {
                    printf("%.*s: %.*s\n", StringExpand(tmp), StringExpand(install_locations[i]));
                }
            };
        }
        
        if (engine_version.data) printf("Couldn't find engine version: %.*s\n", StringExpand(engine_version));
    }
End:
    // Is it necessary to release this when we are just exiting? How would I even check that?
    CoTaskMemFree(program_data_path_wide);
}
