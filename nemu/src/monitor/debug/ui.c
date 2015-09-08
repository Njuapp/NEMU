#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"
#include "cpu/reg.h"
#include "cpu/helper.h"
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <ctype.h>
void cpu_exec(uint32_t);
extern const char *regsl[];
extern const char *regsw[];
extern const char *regsb[];

/* We use the ``readline'' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_d(char *args);
static int cmd_info(char * args);
static int cmd_x(char * args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "d","Delete watch point",cmd_d},
	{ "si","Debug by single step or any number of steps you want",cmd_si},
	{ "info","info r :Print states of GPRs\n      info w :Print states of watchpoints you set",cmd_info},
	{ "x","Scan the memory state",cmd_x},
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))
static int cmd_x(char *args){
	int num;
	swaddr_t addr;
	sscanf(args,"%d %x",&num,&addr);
	int i;
	for(i=0;i<num;i++)
		printf("%08x  :%08x \n",addr+i*4,instr_fetch(addr+i*4,4));
	return 0;
}
static int cmd_si(char *args){
	if(args==NULL){
		cpu_exec(1);
		return 0;
	}
	int n=0;
	char* steps=strtok(args," ");
	int k=0;
	for(;steps[k]!='\0';k++){
		if(!isdigit(steps[k])){
			printf("parameter should be a number.\n");
			return 0;
		}
	}	
	int i;
	for(i=0;steps[i]!='\0';i++)
		n+=n*10+steps[i]-'0';
	cpu_exec(n);
	return 0;
}

static int cmd_info(char *args){
	if(args==NULL){
		printf("lack of parameter.\n");
		return 0;
	}
	char* option=strtok(args," ");
	if(strcmp(option,"r")==0){
		int i;
		for(i=R_EAX;i<=R_EDI;i++)
			printf("%s : %08x\n",regsl[i],reg_l(i));
		for(i=R_AX;i<=R_EDI;i++)
			printf("%s : %04x\n",regsw[i],reg_w(i));
		for(i=R_AL;i<=R_BH;i++)
			printf("%s : %02x\n",regsb[i],reg_b(i));
	}
	return 0;
}
static int cmd_d(char *args){
	printf("Successfully delete %s watchpoint!\n",args);	
	return 0;
}
static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}
#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
