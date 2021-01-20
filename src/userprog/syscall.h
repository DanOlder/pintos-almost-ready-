#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);


struct childs{
	int load_status;
	int child_id;
	int parent_id;	//del
	struct thread *child;	//del
	struct thread *parent;	


	struct childs *next;

};

#endif /* userprog/syscall.h */
