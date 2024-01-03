#ifndef PATHPARSER_H
#define PATHPARSER_H

struct path_root {
    int drive_no;
    struct path_part* first;
};

// A path part is a directory or file name (kinda a linked list)
struct path_part {
    const char* part;
    struct path_part* next;
};

struct path_root* pathparser_parse(const char* path, const char* curr_dir_path);
void pathparser_free(struct path_root* root);

#endif