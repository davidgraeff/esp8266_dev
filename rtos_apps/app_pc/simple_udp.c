#include "simple_udp.h"
#include "user_network.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

const char packet_type_detect[] = "SimpleUDP_detect\n";
const char packet_type_cmd[] = "SimpleUDP_cmd\n";
const char packet_header_info[] = "SimpleUDP_info\n";
const char packet_header_info_ack[] = "SimpleUDP_info_ack\n";

const int detect_len = sizeof(packet_type_detect)-1;
const int cmd_len = sizeof(packet_type_cmd)-1;

const char* action_type[ActTypeLast] = { "TOGGLE", "VALUE", "STATELESS" };
const char* command_type[CmdTypeLast] = { "TOGGLE", "SET" };

char sendBuffer[256];

struct ActionEntry* list_start = 0;

struct ActionEntry* find_action(const char* id) {
    struct ActionEntry* list = list_start;
    while (list != NULL) {
        if (strcmp(id, list->id)==0)
            return list;
        list = list->next;
    }
    return NULL;
}

void add_action(const char* id, const char* name, enum ActionEntryType type, int value, int min, int max) {
	struct ActionEntry* entry = (struct ActionEntry*)malloc(sizeof(struct ActionEntry));
	if (entry == NULL) {
		printf("E > Not enough memory for: %s\n\r", id);
		return;
	}

	entry->id = strdup(id);
	entry->name = strdup(name);
	entry->type = type;
	entry->value = value;
	entry->min = min;
	entry->max = max;

        entry->next = list_start;
	list_start = entry;
}

void  network_before_start() {
	simpleudp_init_commands();
}

/**
 * Return size in bytes for action string
 */
static char* getAllActions(char* destination) {
	struct ActionEntry* list = list_start;
	while (list != NULL) {
		int len = sprintf(destination, "%s\t\%s\t%s\t%d\t%d\t%d\n", action_type[list->type], list->id, list->name, list->value, list->min, list->max);
		destination += len;
		list = list->next;
	}
	return destination;
}

/**
 * Return size in bytes for action string. Copy uid to destination including newline.
 */
static int getName(char* destination) {
	int len = strlen(default_name);
	memcpy(destination, default_name, len);
	return len;
}

static int getUID(char* destination) {
// 	if (netif_default) {
// 		return sprintf(destination,"%d:%d:%d:%d:%d:%d\n",netif_default->hwaddr[0],netif_default->hwaddr[1],netif_default->hwaddr[2],
// 			netif_default->hwaddr[3],netif_default->hwaddr[4],netif_default->hwaddr[5]);
// 	}

	return getName(destination);
}


static char* prepare_header(bool isInfo) {
    char *buffer = sendBuffer;
    if (isInfo) {
	    memcpy(buffer, packet_header_info, sizeof(packet_header_info));
	    buffer += sizeof(packet_header_info)-1;
    } else { // isAck
	    memcpy(buffer, packet_header_info_ack, sizeof(packet_header_info_ack));
	    buffer += sizeof(packet_header_info_ack)-1;
    }

    buffer += getUID(buffer);
    buffer += getName(buffer);
    int len = strlen(app_version);
    memcpy(buffer, app_version, len);
    buffer += len;
    buffer = getAllActions(buffer);
    *buffer = 0;
    ++buffer;

    return buffer;
}

void action_changed(struct ActionEntry* entry) {
     char* buffer = prepare_header(true);
     network_send_broadcast(sendBuffer, buffer-sendBuffer);
}

int nextToggleValue(struct ActionEntry* entry) {
	int v = (entry->value + 1) % (entry->max + 1);
	if (v < entry->min) v = entry->min;
	return v;
}

int request_no_list[3] = {0};
unsigned char request_no_list_current = 0;

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

void network_data_received(void *arg, char *data, unsigned short len)
{
    //printf("RECEIVED %s\n", data);
    int d1 = strncmp(data, packet_type_detect, MIN(len, detect_len));
    int d2 = strncmp(data, packet_type_cmd, MIN(len, cmd_len));

    bool isInfo;
    if (d1==0) {
	data += detect_len;
        isInfo = true;
    } else if (d2==0) {
	data += cmd_len;
        isInfo = false;
    } else return;

    char* peer_uid=0;

    if (isInfo) {
     peer_uid = data;
     char* buffer = prepare_header(true);
     //printf("D > Detect\n");
     network_send_broadcast(sendBuffer, buffer-sendBuffer);
     return;
    } else {
     peer_uid=strchr(data,'\n');
     if (peer_uid == 0) return;
     *peer_uid = 0; // replace \n by 0.
     data = peer_uid + 1;

     //printf("D > Cmd: %s\n", sendBuffer);

     while (data) {
      char *temp, *cmd_type_str, *action_id=0;

      temp = strchr(data,'\t');
      if (temp==0) goto next_line;
      *temp = 0; // replace \t by 0.
      cmd_type_str = data;
      data = temp + 1;

      enum CommandEntryType cmd_type = CmdTypeLast;
      for (int i=0;i<CmdTypeLast-1; ++i) {
	if (strcmp(cmd_type_str, command_type[i])==0) {
	  cmd_type = (enum CommandEntryType)i;
	  break;
	}
      }
      if (cmd_type == CmdTypeLast) {
	printf("E > Cmd type not recognized: %s\n\r", cmd_type_str);
        goto next_line;
      }

     //printf("D > action_type: %s\n", action_type);

      temp = strchr(data,'\t');
      if (temp==0) goto next_line;
      *temp = 0; // replace \t by 0.
      action_id = data;
      data = temp + 1;

      struct ActionEntry* entry = find_action(action_id);
      if (entry == NULL) {
	printf("E > No such action: %s\n\r", action_id);
        goto next_line;
      }

     //printf("D > action_id: %s\n", action_id);

      temp = strchr(data,'\t');
      if (temp==0) goto next_line;
      *temp = 0; // replace \t by 0.
      int value = atoi(data);
      data = temp + 1;

      temp = strchr(data,'\t');
      if (temp==0) goto next_line;
      *temp = 0; // replace \t by 0.
      int request_no = atoi(data);
      data = temp + 1;

      bool already_executed = false;
      if (request_no_list[0] == request_no) already_executed = true;
      else if (request_no_list[1] == request_no) already_executed = true;
      else if (request_no_list[2] == request_no) already_executed = true;

      request_no_list_current = (request_no_list_current+1)%2;
      request_no_list[request_no_list_current] = request_no;

      if (!already_executed) {
        printf("D > request_no: %d @%d\n", request_no, request_no_list_current);
        simpleudp_command(entry, cmd_type == CmdTypeToggle ? nextToggleValue(entry) : value);
      } else {
        printf("D > Already executed: %d\n", request_no);
      }

	// go to next action line
	next_line: data = strchr(data,'\n');
     }

     char* buffer = prepare_header(false);
     network_send(arg, sendBuffer, buffer-sendBuffer);
    }
}
