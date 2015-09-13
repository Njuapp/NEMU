#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
enum {
	NOTYPE = 256,OR,AND,EQ,PLUS,MULTP,DEREF,REG,NUM,ADDR,L_PAR,R_PAR

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE},				// spaces
	{"\\|\\|",OR},
	{"\\&&",AND},
	{"==|!=", EQ},	
	{"\\+|\\-", PLUS},				//+,-
	{"\\*|\\/",MULTP},					// *,/
	{"!",DEREF},
	{"\\$e[a-d]x|\\$e[sbi]p|\\$e[sd]i|\\$[a-d]x|\\$[sb]p|\\$[sd]i|\\$[a-d]l|\\$[a-d]h",REG},
	{"0x[0-9a-f]+",ADDR},
	{"[0-9]+",NUM},
	{"\\(",L_PAR},
	{"\\)",R_PAR},
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret != 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;//####Changing the direction of PRESENT-POSITION

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain 
				 * types of tokens, some extra actions should be performed.
				 */
				if(rules[i].token_type!=NOTYPE){
					tokens[nr_token].type=rules[i].token_type;
					strncpy(tokens[nr_token].str,substr_start,substr_len);
					tokens[nr_token].str[substr_len]='\0';
					nr_token++;
				}
				if(nr_token==32)
					return false;
				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}
	printf("a regular expression!\n");
	return true; 
}
static bool checkpar(int p,int q){
	if(tokens[p].type==L_PAR&&tokens[q].type==R_PAR)
		return true;
	else 
		return false;
}
bool eval_flag=true;
static uint32_t eval(int p,int q){
	if(p>q){
		eval_flag=false;
		return -1;
	}
	if(p==q){
		uint32_t ret;
		if(tokens[p].type==ADDR){
			sscanf(tokens[p].str,"%x",&ret);
			return ret;
		}
		else if(tokens[p].type==REG){
			if(strcmp(tokens[p].str+1,"eip")==0){
				ret=cpu.eip;
				return ret;
			}
			int j;
			for(j=0;j<8;j++){
				if(strcmp(tokens[p].str+1,regsl[j])==0){
					ret=reg_l(j);
					return ret;
				}
			}
			for(j=0;j<8;j++){
				if(strcmp(tokens[p].str+1,regsw[j])==0){
					ret=reg_w(j);
					return ret;
				}
			}
			for(j=0;j<8;j++){
				if(strcmp(tokens[p].str+1,regsb[j])==0){
					ret=reg_b(j);
					return ret;
				}
			}
			eval_flag=false;
			return -1;//####Unnecessary exception handler
		}
		else if(tokens[p].type==NUM){
			sscanf(tokens[p].str,"%d",&ret);
			return ret;
		}
		else {
			eval_flag=false;
			return -1;
		}
		//TODO:Determines the value of an exact position in tokens[] by its content.
	}
	else if(checkpar(p,q)==true){
		return eval(p+1,q-1);
	}
	else if(checkpar(p,q)==false){
		//TODO:Find the exact position of dominant op.
		int k=p;
		int op=p;
		bool inpar=false;
		for(;k<=q;k++){
			if(!inpar&&tokens[k].type==L_PAR)
				inpar=true;
			else if(inpar&&tokens[k].type==R_PAR){
				inpar=false;
				continue;
			}
			if(inpar)
				continue;
			if(tokens[k].type<tokens[op].type)
				op=k;
		}
		if(tokens[op].type==DEREF&&strcmp(tokens[op].str,"*")==0){
			uint32_t address=eval(p+1,q);
			return swaddr_read(address,4);
		}
		else if(tokens[op].type==DEREF&&strcmp(tokens[op].str,"!")==0){
			uint32_t result=eval(p+1,q);
			if(result==0)
				return 1;
			else 
				return 0;
		}
		else if(tokens[op].type==OR){
			uint32_t sub1=eval(p,op-1);
			uint32_t sub2=eval(op+1,q);
			if(sub1==0&&sub2==0)
				return 0;
			else
				return 1; 
		}
		else if(tokens[op].type==AND){
			uint32_t sub1=eval(p,op-1);
			uint32_t sub2=eval(op+1,q);
			if(sub1!=0&&sub2!=0)
				return 1;
			else
				return 0;
		}
		else if(tokens[op].type==EQ){
			uint32_t sub1=eval(p,op-1);
			uint32_t sub2=eval(op+1,q);
			if(strcmp(tokens[op].str,"!=")==0&&sub1!=sub2)
				return 1;
			else if(strcmp(tokens[op].str,"==")==0&&sub1==sub2)
				return 1;
			else
				return 0;
		}		
		else {
			uint32_t sub1=eval(p,op-1);
			uint32_t sub2=eval(op+1,q);
		//TODO:Depending on op,calculate by sub1 and sub2.
			if(strcmp(tokens[op].str,"+")==0)
				return sub1+sub2;
			else if(strcmp(tokens[op].str,"-")==0)
				return sub1-sub2;
			else if(strcmp(tokens[op].str,"*")==0)
				return sub1*sub2;
			else if(strcmp(tokens[op].str,"/")==0)
				return sub1/sub2;
			else {
				eval_flag=false;
				return -1;
			}
		}
		
	}
	eval_flag=false;
	return -1;
	
}
uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		eval_flag=false;
		return -1;
	}
	int i;
	for(i = 0; i < nr_token; i ++) {
	if(strcmp(tokens[i].str,"*") == 0 && (i == 0 || !(tokens[i-1].type==NUM||tokens[i-1].type==REG||tokens[i-1].type==ADDR) )) {
			tokens[i].type = DEREF;
		}
	}
	/* TODO: Insert codes to evaluate the expression. ####257:DEREF,PLUS,EQ,REG,NUM,ADDR,L_PAR,R_PAR*/
	return eval(0,nr_token-1);
}

