#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#pragma pack(1)
struct BootRecord {
    u16 bytes_per_sector;    // 每扇区字节数
    u8 sector_per_cluster;
    u16 boot_sec_count;    // boot占用的扇区数
    u8 FAT_count;    // FAT数
    u16 root_entry_count;    // 根目录文件数
    u16 total_sector_count;
    u8 media_descriptor;
    u16 sector_per_FAT;    // 每个FAT占用扇区数
    u16 sector_per_track;
    u16 heads_count;
    u32 hidden_sector_count;
    u32 large_sector_count;
};

struct DirectoryEntry {
    u8 name[8];
    u8 extension[3];
    u8 attributes;
    u8 reserved[14];
    u16 cluster_low;
    u32 size;
    int status;//1 for root, 2 for invalid
};
#pragma pack()

struct AnalyzedCommand {
    int mode;//0 for ls, 1 for cat, 2 for quit
    bool valid;
    bool detail;
    char path[200];
};

FILE *image;
int fat1_offset = 512;
int root_entry_area_offset;
int data_area_offset;
struct DirectoryEntry root_entry[5000];
int root_entry_count;
char entry_path[200];

extern void myPrint(char *str);

void removeSuffix(char str[]) {
    char tmp[200];
    strcpy(tmp, str);
    int len = strlen(tmp);
    for (int i = 0; i < len; i++) {
        if (tmp[i] == ' ' || tmp[i] == '\0' || tmp[i] == 16) {
            tmp[i] = '\0';
            break;
        }
    }
    strcpy(str, tmp);
}

bool checkExtexsion(char str[]){
    for(int i = 0; i< 3; i++){
        if(str[i]>='A'&&str[i]<='Z'){
            return true;
        }
    }
    return false;
}

u16 getNextClusterNum(int index) {
    u32 byte_offset = fat1_offset + index + (index / 2);
    u16 value;
    u8 value_low;
    u8 value_high;
    fseek(image, byte_offset, SEEK_SET);
    fread(&value_low, sizeof(u8), 1, image);
    fread(&value_high, sizeof(u8), 1, image);
    if (index % 2) value = (value_high << 4) + (value_low >> 4);
    else value = value_low + ((u16) (value_high & 0x0F) << 8);
    return value;
}

bool bootRecordReader(struct BootRecord *bpb) {
    if (fseek(image, 11, SEEK_SET) == -1) return false;
    if (fread(bpb, 1, 25, image) != 25) return false;
    return true;
}

void loadRootEntry(struct BootRecord bpb) {
    fseek(image, root_entry_area_offset, SEEK_SET);
    for (int i = 0; i < bpb.root_entry_count; i++) {
        fread(&root_entry[i], sizeof(u8), 32, image);
        if (root_entry[i].name[0] == 229) {//Invalid Name
            i--;
            continue;
        }
        if (root_entry[i].attributes == 15) {//LFN
            i--;
            continue;
        }
        removeSuffix(root_entry[i].name);
        root_entry[i].status = 0;//init
        if (root_entry[i].name[0] == '\000') {
            root_entry_count = i;
            break;
        }
    }
}

bool checkNameValid(char s) {
    return (s == '/' || s == '.' || (s >= 'A' && s <= 'Z') || (s >= '0' && s <= '9'));
}

struct DirectoryEntry readEntry(int cluster, int n) {
    fseek(image, data_area_offset + (cluster - 2) * 512 + n * 32, SEEK_SET);
    struct DirectoryEntry entry;
    fread(&entry, sizeof(u8), 32, image);
    removeSuffix(entry.name);
    return entry;
}

struct AnalyzedCommand analyzeCommand(const char *command) {
    struct AnalyzedCommand ret;
    ret.valid = true, ret.detail = false;
    memset(ret.path, 0, sizeof(ret.path));
    int pos;
    if (command[0] == 'l' && command[1] == 's' && (command[2] == ' ' || command[2] == '\n' || command[2] == '\0')) {
        ret.mode = 0;
        pos = 2;
    } else if (command[0] == 'c' && command[1] == 'a' && command[2] == 't' &&
               (command[3] == ' ' || command[3] == '\n' || command[3] == '\0')) {
        ret.mode = 1;
        pos = 3;
    } else if (command[0] == 'e' && command[1] == 'x' && command[2] == 'i' && command[3] == 't' &&
               (command[4] == ' ' || command[4] == '\n' || command[4] == '\0')) {
        ret.mode = 2;
        return ret;
    } else {
        myPrint("ERROR: Invalid Command! Your command should start with 'ls', 'cat' or 'exit'. \n");
        ret.valid = false;
        return ret;
    }
    while (command[pos] != '\n' && command[pos] != '\0') {
        while (command[pos] == ' ') pos++;
        if (checkNameValid(command[pos])) {
            if (ret.path[0] != 0) {
                myPrint("ERROR: Invalid Command! You can only input one path.\n");
                ret.valid = false;
                return ret;
            }
            int pathPos = 0;
            while (command[pos] != ' ' && command[pos] != '\n' && command[pos] != '\0') {
                if (checkNameValid(command[pos])) {
                    ret.path[pathPos] = command[pos];
                } else {
                    myPrint("ERROR: Invalid Command! Path name is not valid.\n");
                    ret.valid = false;
                    return ret;
                }
                pathPos++, pos++;
            }
        } else if (command[pos] == '-' && ret.mode == 0) {
            pos++;
            if (command[pos] != 'l') {
                myPrint("ERROR: Invalid Command!\n");
                ret.valid = false;
                return ret;
            } else {
                pos++;
            }
            while (command[pos] != ' ' && command[pos] != '\n' && command[pos] != '\0') {
                if (command[pos] != 'l') {
                    myPrint("ERROR: Invalid Command!\n");
                    ret.valid = false;
                    return ret;
                }
                pos++;
            }
            ret.detail = true;
        } else {
            myPrint("ERROR: Invalid Command!\n");
            ret.valid = false;
            return ret;
        }
    }
    if (ret.path[0] == '/') {
        int length = (int) strlen(ret.path);
        memmove(ret.path, ret.path + 1, length);
        ret.path[length - 1] = '\0';
    }

    return ret;
}

struct DirectoryEntry getEntry(char *path) {
    struct DirectoryEntry stack[300];
    int top = 0;
    if (path[0] == '\0') {
        struct DirectoryEntry ret;
        ret.status = 1;//is root
        entry_path[0] = '/', entry_path[1] = '\0';
        return ret;
    }
    int pos = 0;
    char name[200];
    while (path[pos] == '/' || !pos) {
        if (pos)pos++;
        memset(name, 0, sizeof(name));
        int namePos = 0;
        while (path[pos] != '/' && path[pos] != '\0') {
            name[namePos] = path[pos];
            namePos++, pos++;
        }
        if (!strcmp(name, "..")) {
            if (top) top--;
            continue;
        }
        if (top) {
            bool found = false;
            int cluster = stack[top - 1].cluster_low;
            for (int i = 0; i < 16; i++) {
                struct DirectoryEntry entry = readEntry(cluster, i);
                if (entry.name[0] == 0)break;
                if (i == 15)cluster = getNextClusterNum(cluster), i = 0;
                char tmp[200];
                strcpy(tmp, entry.name);
                if(checkExtexsion(entry.extension)){
                    strncat(tmp, ".",1);
                    strncat(tmp, entry.extension, 3);
                }
                int len = strlen(tmp);
                for (int i = len; i >= 0; i--) {
                    if (tmp[i] == ' ' || tmp[i] == '\0' || tmp[i] == 16) {
                        tmp[i] = '\0';
                    } else {
                        break;
                    }
                }
                if (!strcmp(tmp, name)) {
                    found = true;
                    stack[top++] = entry;
                }
            }
            if (!found) {
                myPrint("ERROR: Invalid Path! No such file or directory.\n");
                struct DirectoryEntry ret;
                ret.status = 2;//error
                return ret;
            }
        } else {
            bool found = false;
            for (int i = 0; i < root_entry_count; i++) {
                char tmp[200];
                strcpy(tmp, root_entry[i].name);
                if(checkExtexsion(root_entry[i].extension)){
                    strncat(tmp, ".",1);
                    strncat(tmp, root_entry[i].extension, 3);
                }
                int len = strlen(tmp);
                for (int i = len; i >= 0; i--) {
                    if (tmp[i] == ' ' || tmp[i] == '\0' || tmp[i] == 16) {
                        tmp[i] = '\0';
                    } else {
                        break;
                    }
                }
                if (!strcmp(tmp, name)) {
                    found = true;
                    stack[top++] = root_entry[i];
                }
            }
            if (!found) {
                myPrint("ERROR: Invalid Path! No such file or directory.\n");
                struct DirectoryEntry ret;
                ret.status = 2;//error
                return ret;
            }
        }
    }
    if (!top) {
        struct DirectoryEntry ret;
        ret.status = 1;//is root
        entry_path[0] = '/', entry_path[1] = '\0';
        return ret;
    } else {
        int entry_path_pos = 0;
        for (int i = 0; i < top; i++) {
            entry_path[entry_path_pos++] = '/';
            int len = strlen(stack[i].name);
            for (int j = 0; j < len; j++) {
                if (stack[i].name[j] == ' ' || stack[i].name[j] == 16)continue;
                entry_path[entry_path_pos++] = stack[i].name[j];
            }
        }
        entry_path[entry_path_pos++] = '\0';
        return stack[top - 1];
    }
}

void ls(struct DirectoryEntry now, bool detailed) {
    if (now.status == 2) return;//ERROR
    if (now.attributes != 0x10 && now.status != 1) {
        myPrint("ERROR: Invalid Path! This path is not a directory.\n");
        return;
    }
    if (!detailed) {
        myPrint(entry_path);
        if (entry_path[0] == '/' && entry_path[1] == '\0')myPrint(":\n");
        else myPrint("/:\n");
    }
    struct DirectoryEntry sons[300];
    int numberOfSons = 0;
    if (now.status == 1) {//is root.
        numberOfSons = root_entry_count;
        for (int i = 0; i < root_entry_count; i++) {
            sons[i] = root_entry[i];
        }
    } else {
        int nowCluster = now.cluster_low;
        for (int i = 0; i < 16; i++) {
            struct DirectoryEntry entry = readEntry(nowCluster, i);
            if (entry.name[0] == 0)break;
            if (i == 15) {
                nowCluster = getNextClusterNum(nowCluster);
                i = 0;
            }
            if (entry.attributes == 0xf || entry.name[0] == 229)continue;
            sons[numberOfSons++] = entry;
        }
    }
    if (!detailed) {
        for (int i = 0; i < numberOfSons; i++) {
            if (sons[i].attributes == 0x10)myPrint("\033[31m");
            myPrint(sons[i].name);
            if(checkExtexsion(sons[i].extension)){
                myPrint(".");
                char ext[4];
                memset(ext,0, sizeof(ext));
                strncpy(ext,sons[i].extension,3);
                myPrint(ext);
            }
            myPrint("\033[0m");
            myPrint(" ");
        }
        myPrint("\n");
    } else {
        int nowNumberOfFile = 0;
        int nowNumberOfDir = 0;
        for (int i = 0; i < numberOfSons; i++) {
            if (sons[i].attributes == 0x10 && ((!strcmp(sons[i].name, ".")) || (!strcmp(sons[i].name, ".."))))continue;
            if (sons[i].attributes == 0x10)nowNumberOfDir++;
            else nowNumberOfFile++;
        }
        myPrint(entry_path);
        if (entry_path[0] == '/' && entry_path[1] == '\0')myPrint(" ");
        else myPrint("/ ");
        char str_buffer[15];
        sprintf(str_buffer, "%d %d:\n", nowNumberOfDir, nowNumberOfFile);
        myPrint(str_buffer);
        for (int i = 0; i < numberOfSons; i++) {
            if (sons[i].attributes == 0x10)myPrint("\033[31m");
            myPrint(sons[i].name);
            if(checkExtexsion(sons[i].extension)){
                myPrint(".");
                char ext[4];
                memset(ext,0, sizeof(ext));
                strncpy(ext,sons[i].extension,3);
                myPrint(ext);
            }
            myPrint("\033[0m");
            if (!((strcmp(sons[i].name, ".")) && (strcmp(sons[i].name, "..")))) {
                myPrint("\n");
            }
            if (sons[i].attributes == 0x10 && (strcmp(sons[i].name, ".")) && (strcmp(sons[i].name, ".."))) {
                int sonNumberOfFile = 0;
                int sonNumberOfDir = 0;
                for (int j = 0; j < 16; j++) {
                    struct DirectoryEntry entry = readEntry(sons[i].cluster_low, j);
                    if (entry.name[0] == 0)break;
                    if (entry.attributes == 0xf || entry.name[0] == 229)continue;
                    if (entry.attributes == 0x10 && ((!strcmp(entry.name, ".")) || (!strcmp(entry.name, ".."))))
                        continue;
                    if (entry.attributes == 0x10)sonNumberOfDir++;
                    else sonNumberOfFile++;
                }
                char str_buffer[15];
                sprintf(str_buffer, " %d %d\n", sonNumberOfDir, sonNumberOfFile);
                myPrint(str_buffer);
            } else if ((strcmp(sons[i].name, ".")) && (strcmp(sons[i].name, ".."))) {
                char str_buffer[15];
                sprintf(str_buffer, " %d\n", sons[i].size);
                myPrint(str_buffer);
            }
        }
        myPrint("\n");
    }
    for (int i = 0; i < numberOfSons; i++) {
        if (sons[i].attributes == 0x10 && (strcmp(sons[i].name, ".")) && (strcmp(sons[i].name, ".."))) {
            int res = strlen(entry_path);
            if (!(entry_path[0] == '/' && entry_path[1] == 0))strcat(entry_path, "/");
            strcat(entry_path, sons[i].name);
            ls(sons[i], detailed);
            entry_path[res] = 0;
        }
    }
}

void cat(struct DirectoryEntry now) {
    if (now.attributes != 32) {
        myPrint("ERROR: Invalid Path! This path is not a file.\n");
        return;
    }
    int cluster = now.cluster_low;
    while (cluster < 0xff8) {
        char buffer[513];
        fseek(image, data_area_offset + 512 * (cluster - 2), SEEK_SET);
        fread(buffer, sizeof(u8), 512, image);
        buffer[512] = 0;
        myPrint(buffer);
        cluster = getNextClusterNum(cluster);
    }
}

void exitProgramWithErrorMsg(char *errorMsg) {
    myPrint(errorMsg);
    myPrint("Press any key to quit.\n");
    getchar();
    if (image != NULL) fclose(image);
    exit(0);
}

int main() {
    //Init.
    image = fopen("a.img", "rb");
    if (image == NULL)exitProgramWithErrorMsg("FATAL: THIS IMAGE DOES NOT EXIST!");
    struct BootRecord bpb;
    if (!bootRecordReader(&bpb))exitProgramWithErrorMsg("ERROR: Read boot record failed!");
    //Set Offset.
    root_entry_area_offset = (bpb.boot_sec_count + (bpb.FAT_count * bpb.sector_per_FAT)) * bpb.bytes_per_sector;
    data_area_offset = root_entry_area_offset + bpb.root_entry_count * 32;
    //Load Root Entry.
    loadRootEntry(bpb);
    //Read Command.
    while (true) {
        char command[200];
        gets(command);
        struct AnalyzedCommand nowCommand = analyzeCommand(command);
        if (!nowCommand.valid) continue;
        memset(entry_path, 0, sizeof(entry_path));
        struct DirectoryEntry thisEntry = getEntry(nowCommand.path);
        if (nowCommand.mode == 2) break;
        else if (nowCommand.mode == 0) ls(thisEntry, nowCommand.detail);
        else if (nowCommand.mode == 1) cat(thisEntry);
    }
    myPrint("Bye\n");
    fclose(image);
}