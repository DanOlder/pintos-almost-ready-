#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);


struct childs{
	int child_id;
	int parent_id;
	struct thread *child;
	struct thread *parent;

	struct childs *next;

};

#endif /* userprog/syscall.h */
