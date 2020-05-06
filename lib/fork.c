// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800


extern void _pgfault_upcall(void);
//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//

static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;
	pte_t pte;
	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	pte = uvpt[(uint32_t)addr >> PTXSHIFT];
	if((err & FEC_WR) == 0 || !(pte & PTE_COW))
	{
		panic("fork.c: pgfault: error.\n");
	}
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	r = sys_page_alloc(sys_getenvid(), PFTEMP, PTE_P|PTE_U|PTE_W);
	if(r < 0)
	{
		cprintf("fork.c: pgfault 1: %e\n", r);
		sys_env_destroy(sys_getenvid());
	}
	addr = ROUNDDOWN(addr, PGSIZE);
	memmove(PFTEMP, addr, PGSIZE);
	r = sys_page_map(sys_getenvid(), PFTEMP, sys_getenvid(), addr, PTE_P|PTE_W|PTE_U);
	if(r < 0)
	{
		cprintf("fork.c: pgfault 2: %e\n", r);
		sys_env_destroy(sys_getenvid());
	}
	//panic("pgfault not implemented");
}

/*
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).
	if ((err & FEC_WR) == 0)
		panic("pgfault: faulting address [%08x] not a write\n", addr);

	void *page_aligned_addr = (void *) ROUNDDOWN(addr, PGSIZE);
	uint32_t page_num = (uint32_t) page_aligned_addr / PGSIZE;
	if (!(uvpt[page_num] & PTE_COW))
		panic("pgfault: fault was not on a copy-on-write page\n");

	// LAB 4: Your code here.

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	// Allocate
	if ((r = sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W)) < 0)
		panic("sys_page_alloc: %e\n", r);

	// Copy over
	void *src_addr = (void *) ROUNDDOWN(addr, PGSIZE);
	memmove(PFTEMP, src_addr, PGSIZE);

	// Remap
	if ((r = sys_page_map(0, PFTEMP, 0, src_addr, PTE_P | PTE_U | PTE_W)) < 0)
		panic("sys_page_map: %e\n", r);

}
*/

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//

static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	// LAB 4: Your code here.
	uint32_t perm = uvpt[pn] & 0xfff;
	if(perm & PTE_W || perm & PTE_COW)
	{
		perm &= 0x7;
		perm &= ~PTE_W;
		perm |= PTE_COW;
	}
	r = sys_page_map(thisenv->env_id, (void *)(pn*PGSIZE), envid, (void *)(pn*PGSIZE), perm);
	if(r < 0)
	{
		cprintf("fork.c: duppage 1: %e\n", r);
		return r;
	}
	if(perm & PTE_COW)
	{
		r = sys_page_map(thisenv->env_id, (void *)(pn*PGSIZE), thisenv->env_id, (void *)(pn*PGSIZE), perm);
	}
	if(r < 0)
	{
		cprintf("fork.c: duppage 2: %e\n", r);
		return r;
	}
	//panic("duppage not implemented");
	return 0;
}

/*
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	uint32_t perm = PTE_P | PTE_COW;
	envid_t this_envid = thisenv->env_id;

	// LAB 4: Your code here.
	if (uvpt[pn] & PTE_COW || uvpt[pn] & PTE_W) {
		if (uvpt[pn] & PTE_U)
			perm |= PTE_U;

		// Map page COW, U and P in child
		if ((r = sys_page_map(this_envid, (void *) (pn*PGSIZE), envid, (void *) (pn*PGSIZE), perm)) < 0)
			panic("sys_page_map: %e\n", r);

		// Map page COW, U and P in parent
		if ((r = sys_page_map(this_envid, (void *) (pn*PGSIZE), this_envid, (void *) (pn*PGSIZE), perm)) < 0)
			panic("sys_page_map: %e\n", r);

	} else { // map pages that are present but not writable or COW with their original permissions
		if ((r = sys_page_map(this_envid, (void *) (pn*PGSIZE), envid, (void *) (pn*PGSIZE), uvpt[pn] & 0xFFF)) < 0)
			panic("sys_page_map: %e\n", r);
	}


	return 0;
}
//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
*/
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t envid;
	uint8_t *addr;
	int r;
	uint32_t i;

	set_pgfault_handler(pgfault);
	// Allocate a new child environment.
	// The kernel will initialize it with a copy of our register state,
	// so that the child will appear to have called sys_exofork() too -
	// except that in the child, this "fake" call to sys_exofork()
	// will return 0 instead of the envid of the child.
	envid = sys_exofork();
	if (envid < 0)
		panic("fork.c: fork 1: %e", envid);
	if (envid == 0) {
		// We're the child.
		// The copied value of the global variable 'thisenv'
		// is no longer valid (it refers to the parent!).
		// Fix it and return 0.
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	// we are the parent
	// memory mapping setup
	for(i = 0; i < (UTOP - PGSIZE)>>PTXSHIFT; i++)
	{
		if((uvpd[i >> 10] & PTE_P) && (uvpt[i] & PTE_P))
		{
			r = duppage(envid, i);
		    if(r < 0)
	        {
				cprintf("uvpd: %x, uvpt: %x", uvpd[i >> 10], uvpt[i]);
		        cprintf("fork.c: fork 2: %e\n", r);
		        return r;
	        }
		}
		
	}
	// new exception stack
	r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_P|PTE_U|PTE_W);
	if(r < 0)
	{
		cprintf("fork.c: fork 3: %e\n", r);
		return r;
	}
	// page fault entrypoint for child
	r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
	if(r < 0)
	{
		cprintf("fork.c: fork 4: %e\n", r);
		return r;
	}
	// marks the child as runnable
	r = sys_env_set_status(envid, ENV_RUNNABLE);
	if(r < 0)
	{
		cprintf("fork.c: fork 5: %e\n", r);
		return r;
	}
	return envid;
	//panic("fork not implemented");
}

/*
envid_t
fork(void)
{
	// LAB 4: Your code here.
	int r;
	envid_t child_envid;

	set_pgfault_handler(pgfault);

	child_envid = sys_exofork();
	if (child_envid < 0)
		panic("sys_exofork: %e\n", child_envid);
	if (child_envid == 0) { // child
		// Fix thisenv like dumbfork does and return 0
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	// We're in the parent

	// Iterate over all pages until UTOP. Map all pages that are present
	// and let duppage worry about the permissions.
	// Note that we don't remap anything above UTOP because the kernel took
	// care of that for us in env_setup_vm().
	uint32_t page_num;
	pte_t *pte;
	for (page_num = 0; page_num < PGNUM(UTOP - PGSIZE); page_num++) {
		uint32_t pdx = ROUNDDOWN(page_num, NPDENTRIES) / NPDENTRIES;
		if ((uvpd[pdx] & PTE_P) == PTE_P &&
			((uvpt[page_num] & PTE_P) == PTE_P)) {
				duppage(child_envid, page_num);
		}
	}

	// Allocate exception stack space for child. The child can't do this themselves
	// because the mechanism by which it would is to run the pgfault handler, which
	// needs to run on the exception stack (catch 22).
	if ((r = sys_page_alloc(child_envid, (void *) (UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
		panic("sys_page_alloc: %e\n", r);

	// Set page fault handler for the child
	if ((r = sys_env_set_pgfault_upcall(child_envid, _pgfault_upcall)) < 0)
		panic("sys_env_set_pgfault_upcall: %e\n", r);

	// Mark child environment as runnable
	if ((r = sys_env_set_status(child_envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e\n", r);

	return child_envid;
}
*/

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
