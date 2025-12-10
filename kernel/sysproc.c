#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  uint64 base;            // Địa chỉ ảo bắt đầu
  int len;                // Số lượng trang
  uint64 mask_user_addr;  // Địa chỉ buffer của user
  
  argaddr(0, &base);
  argint(1, &len);
  argaddr(2, &mask_user_addr);

  if(len > 32) 
    len = 32;

  unsigned int abits = 0; // Bitmask kết quả
  struct proc *p = myproc();

  // Duyệt qua từng trang
  for(int i = 0; i < len; i++){
    uint64 va = base + i * PGSIZE;
    
    // Tìm PTE
    pte_t *pte = walk(p->pagetable, va, 0);

    // Kiểm tra: Tồn tại + Hợp lệ (V) + Đã truy cập (A)
    if(pte != 0 && (*pte & PTE_V) && (*pte & PTE_A)){
      
      // Bật bit thứ i
      abits = abits | (1 << i);

      // Xóa bit Access để reset trạng thái cho lần sau
      *pte &= ~PTE_A; 
    }
  }
  sfence_vma();

  // Sao chép kết quả ra userspace
  // copyout trả về int (-1 nếu lỗi)
  if(copyout(p->pagetable, mask_user_addr, (char *)&abits, sizeof(abits)) < 0)
    return -1;
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
