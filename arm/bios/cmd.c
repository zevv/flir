

#include <stdint.h>
#include <string.h>

#include "bios/cmd.h"
#include "bios/uart.h"
#include "bios/evq.h"
#include "bios/printd.h"
#include "bios/sysinfo.h"

#define CMD_BUF_SIZE 64
#define CMD_HIST_NUM 4
#define CMD_MAX_PARTS 12

struct cmd {
	char buf[CMD_BUF_SIZE];
	char hist[CMD_BUF_SIZE];
	uint8_t state;
	int8_t pos;
	int8_t len;
};


static void on_ev_uart(event_t *ev, void *data);


static int32_t split(char *buf, char *part[], int8_t maxparts)
{
	int8_t parts = 0;
	int32_t o2 = 0;

	if(buf[0] != '\0') {

		part[0] = buf;
		parts ++;

		while(parts < maxparts) {
			while((buf[o2] != '\0') && (buf[o2] != ' ')) {
				o2 ++;
			}

			if(buf[o2] != '\0') {

				buf[o2] = '\0';
				o2 ++;

				while(buf[o2] == ' ') {
					o2 ++;
				}
			}
			
			if(buf[o2] == '\0') {
				break;
			}

			part[parts] = &buf[o2];
			parts ++;
		}
	}

	return parts;
}


static void cmd_handler_foreach(void (*cb)(struct cmd_handler_t *h))
{
#ifndef _lint
	extern struct cmd_handler_t __start_cmd;
	extern struct cmd_handler_t __stop_cmd;
	struct cmd_handler_t *h = &__start_cmd;

	while(h < &__stop_cmd) {
		cb(h);
		h++;
	}
#endif
}


/*
 * Handle cmd line string
 */

static void handle_cmd(char *line)
{
	rv r = RV_ENOENT;
	char *part[CMD_MAX_PARTS];
	int32_t parts;
	struct cmd_handler_t *h_found = NULL;
	uint8_t match_count = 0;

	parts = split(line, part, CMD_MAX_PARTS);

	if(parts > 0) {
	
		size_t l = strlen(part[0]);

		void find_exact(struct cmd_handler_t *h)
		{
			if(strcmp(part[0], h->name) == 0) {
				h_found = h;
				match_count ++;
				return;
			}
		}
		
		void find_fuzzy(struct cmd_handler_t *h)
		{
			if(strncmp(part[0], h->name, l) == 0) {
				h_found = h;
				match_count ++;
			}
		}

		cmd_handler_foreach(find_exact);

		if(h_found == NULL) {
			cmd_handler_foreach(find_fuzzy);
		}
		
		if(match_count == 1) {
			int32_t argc = parts - 1;
			char **argv = &part[1];
			r = h_found->fn(argc, argv);
			if(r != RV_OK) {
				printd("%s\n", rv_str(r));
			}
			if(r == RV_EINVAL) {
				printd("%-10s %s\n", h_found->name, h_found->help);
			}
		} else {
			if(match_count == 0) {
				printd("unknown cmd\n");
			} else {
				printd("ambigious cmd\n");
			}
		}
	}
}


/*
 * Simple cmd line editor
 */

static void handle_char(struct cmd *cmd, uint8_t c)
{

	void move(int8_t d)
	{
		char *p = cmd->buf + cmd->pos;
		uint8_t l = cmd->len - cmd->pos;
		memmove(p+d, p, l);
		cmd->pos += d;
		cmd->len += d;
		p += d;
	}

	switch(cmd->state) {
	case 0u:
		switch(c) {
		case 8u:
		case 127u:
			if(cmd->pos > 0) {
				move(-1);
			}
			break;
		case 12u:
			printd("\e[2J\e[H");
			break;
		case 1u:
			cmd->pos = 0;
			break;
		case 4u:
			if(cmd->pos < cmd->len) {
				cmd->pos ++;
				move(-1);
			}
			break;
		case 5u:
			cmd->pos = cmd->len;
			break;
		case 10u:
		case 13u:
			printd("\r\n");
			strcpy(cmd->hist, cmd->buf);
			handle_cmd(cmd->buf);
			cmd->len = cmd->pos = 0;
			break;
		case 27u:
			cmd->state = 1u;
			break;
		default:
			if(c >= 32u && c <= 128u) {
				if(cmd->len < CMD_BUF_SIZE-1) {
					move(+1);
					cmd->buf[cmd->pos-1] = c;
				}
			}
			break;
		}
		break;
	case 1u:
		switch(c) {
		case '[':
			cmd->state = 2u;
			break;
		default:
			cmd->state = 0u;
			break;
		}
		break;
	case 2u:
		switch(c) {
		case 'A':
			strcpy(cmd->buf, cmd->hist);
			cmd->pos = cmd->len = strlen(cmd->buf);
			break;
		case 'B':
			cmd->buf[0] = '\0';
			cmd->len = cmd->pos = 0;
			break;
		case 'C':
			if(cmd->pos < cmd->len) cmd->pos ++;
			break;
		case 'D':
			if(cmd->pos > 0) cmd->pos --;
			break;
		}
		cmd->state = 0u;
		break;
	}

	cmd->buf[cmd->len] = '\0';

	char hostname[16];
	sysinfo_get(SYSINFO_NAME, hostname, sizeof(hostname));

	printd("\r%s> %s\e[K", hostname, cmd->buf);
	if(cmd->len != cmd->pos) {
		printd("\e[%dD", cmd->len - cmd->pos);
	}
}


static void on_ev_uart(event_t *ev, void *data)
{
	static struct cmd cmd;
	handle_char(&cmd, ev->uart.data);
}


EVQ_REGISTER(EV_UART, on_ev_uart);


static rv on_cmd_help(uint8_t argc, char **argv)
{
	struct cmd_handler_t *cur = NULL;
	struct cmd_handler_t *next = NULL;

	/* lazy O(n^2) sort */

	do {

		void aux(struct cmd_handler_t *h)
		{
			if(cur == NULL || strcmp(h->name, cur->name) > 0) {
				if(next == NULL || strcmp(h->name, next->name) < 0) {
					next = h;
				}
			}
		}

		next = NULL;
		cmd_handler_foreach(aux);
		if(next) {
			cur = next;
			printd("%-10s %s\n", cur->name, cur->help);
		}
	} while(next != NULL);

	return RV_OK;
}


CMD_REGISTER(help, on_cmd_help, "");

/*
 * End
 */
