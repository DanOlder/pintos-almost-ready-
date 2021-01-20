#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);


struct childs{
	int load_status;
	int on_wait;
	//int on_delete;
	int child_id;
	struct thread *parent;	

	struct childs *next;

};

#endif /* userprog/syscall.h */
