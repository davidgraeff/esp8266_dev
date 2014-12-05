#pragma once

enum CommandEntryType {
    CmdTypeToggle, CmdTypeSet, CmdTypeLast
};

enum ActionEntryType {
    ActTypeToggle, ActTypeValue, ActTypeStateless, ActTypeLast
};

struct ActionEntry {
    char* id;
    char* name;
    int value;
    int min;
    int max;
    enum ActionEntryType type;
    struct ActionEntry* next;
};

void add_action(const char* id, const char* name, enum ActionEntryType type, int value, int min, int max);
struct ActionEntry* find_action(const char* id);
void action_changed(struct ActionEntry* entry);

//// To be implemented by the simpleUDP application. //
extern const char app_version[];
extern const char default_name[];

void simpleudp_command(struct ActionEntry* entry, int value);
void simpleudp_init_commands();
