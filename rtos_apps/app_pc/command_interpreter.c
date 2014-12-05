#include "simple_udp.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

const char app_version[] = "1.0\n";
const char default_name[] = "PC\n";

void simpleudp_init_commands() {
	add_action("pc_standby", "Standby", ActTypeStateless, 0, 0, 0);
	add_action("pc_start_kodi", "Starte Kodi", ActTypeStateless, 0, 0, 0);
}

void simpleudp_command(struct ActionEntry* entry, int value)
{
    if (strcmp(entry->id, "pc_standby")==0) {
	system("systemctl suspend");
    } else if (strcmp(entry->id, "pc_start_kodi")==0) {
        if (fork()==0) {
            printf("Starte Kodi\n");
            execl("/usr/bin/kodi", "", (char *) 0);
            exit(-1);
        }
    }
}
