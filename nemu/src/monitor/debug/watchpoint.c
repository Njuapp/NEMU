#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_list[NR_WP];
static WP *head, *free_;
extern bool eval_flag;
void init_wp_list() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_list[i].NO = i;
		wp_list[i].next = &wp_list[i + 1];
		wp_list[i].expr_value=0;
		wp_list[i].expr[0]='\0';
	}
	wp_list[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_list;
}
void add_wp(char *args){
	if(free_==NULL){
		printf("NO more space to set new watchpoints\n");
		return ;
	}
	eval_flag=true;
	uint32_t expr_value=expr(args,&eval_flag);
	if(eval_flag==false){
		printf("INVALID WATCHPOINT\n");
		return ;
	}
	WP* newwp=free_;
	free_=free_->next;
	strcpy(newwp->expr,args);
	newwp->expr_value=expr_value;
	newwp->next=head;
	head=newwp;
}

void delete_wp(char *args){
	int no;
	sscanf(args,"%d",&no);
	WP* freewp=head;
	if(head->NO==no){
		head=head->next;
		freewp->next=free_;
		free_=freewp;
		return ;
	}
	WP* pre=head;
	WP* cur=head->next;
	while(cur!=NULL){
		if(cur->NO==no)
			break;
		pre=pre->next;
		cur=cur->next;
	}
	if(cur==NULL){
		printf("NO matched watchpoints.\n");
		return ;
	}
	freewp=cur;
	pre->next=cur->next;
	freewp->next=free_;
	free_=freewp;
}
void print_wp(){
	WP* wp_head=head;
	for(;wp_head!=NULL;wp_head=wp_head->next){
		printf("NO.%d watchpoint:\nexpression:%s    value:%08x\n",wp_head->NO,wp_head->expr,wp_head->expr_value);
	}
}
bool check_wp(){
	WP* wp=head;
	bool unchanged=true;
	for(;wp!=NULL;wp=wp->next){
		eval_flag=true;
		uint32_t uptodate=expr(wp->expr,&eval_flag);
		if(uptodate!=wp->expr_value){
			printf("NO.%d watchpoint changed:\nexpression:%s    value:%08x->%08x\n",wp->NO,wp->expr,wp->expr_value,uptodate);
			wp->expr_value=uptodate;
			unchanged=false;
		}
	}
	return unchanged;
}

/* TODO: Implement the functionality of watchpoint */


