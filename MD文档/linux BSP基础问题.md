
Linux内核里面，内存申请有哪几个函数，各自的区别？

```
	void *kmalloc(size_t size, gfp_t flags)；
```
	

kmalloc() 申请的内存位于物理内存映射区域，而且在物理上也是连续的，它们与真实的物理地址只有一个固定的偏移，因为存在较简单的转换关系，所以对申请的内存大小有限制，不能超过128KB
	

```
较常用的 flags（分配内存的方法）：

	GFP_ATOMIC —— 分配内存的过程是一个原子过程，分配内存的过程不会被（高优先级进程或中断）打断；
	GFP_KERNEL —— 正常分配内存；
	GFP_DMA —— 给 DMA 控制器分配内存，需要使用该标志（DMA要求分配虚拟地址和物理地址连续）。
	flags 的参考用法： 
　		|– 进程上下文，可以睡眠		GFP_KERNEL 
　		|– 进程上下文，不可以睡眠　　　	GFP_ATOMIC 
　		|　　|– 中断处理程序　　　　　　	GFP_ATOMIC 
　		|　　|– 软中断　　　　　　　　　	GFP_ATOMIC 
　		|　　|– Tasklet　　　　　　　　		GFP_ATOMIC 
　		|– 用于DMA的内存，可以睡眠　　	GFP_DMA | GFP_KERNEL 
		|– 用于DMA的内存，不可以睡眠　	GFP_DMA |GFP_ATOMIC 
	void kfree(const void *objp);
```

	

```
/**
 	* kzalloc - allocate memory. The memory is set to zero.
 	* @size: how many bytes of memory are required.
 	* @flags: the type of memory to allocate (see kmalloc).
 	*/
	static inline void *kzalloc(size_t size, gfp_t flags)
	{
		return kmalloc(size, flags | __GFP_ZERO);
	}
```
	

kzalloc() 函数与 kmalloc() 非常相似，参数及返回值是一样的，可以说是前者是后者的一个变种，因为 kzalloc() 实际上只是额外附加了 __GFP_ZERO 标志。所以它除了申请内核内存外，还会对申请到的内存内容清零

```
	void kfree(const void *objp);
	void *vmalloc(unsigned long size);
```

vmalloc() 函数则会在虚拟内存空间给出一块连续的内存区，但这片连续的虚拟内存在物理内存中并不一定连续。由于 vmalloc() 没有保证申请到的是连续的物理内存，因此对申请的内存大小没有限制，如果需要申请较大的内存空间就需要用此函数了，该函数可以睡眠，不能在中断上下文中调用

	void vfree(const void *addr);

ARM处理器的2种工作状态和7种工作模式

```
	1、ARM状态：32位，ARM状态执行字对齐的32位ARM指令。
	2、Thumb状态，16位，执行半字对齐的16位指令
```
	

7种工作模式介绍

```
1.用户模式：
用户模式是用户程序的工作模式，它运行在操作系统的用户态，它没有权限去操作其它硬件资源，只能执行处理自己的数据，也不能切换到其它模式下，要想访问硬件资源或切换到其它模式只能通过软中断或产生异常。
2.系统模式：
系统模式是特权模式，不受用户模式的限制。用户模式和系统模式共用一套寄存器，操作系统在该模式下可以方便的访问用户模式的寄存器，而且操作系统的一些特权任务可以使用这个模式访问一些受控的资源。
 说明：用户模式与系统模式两者使用相同的寄存器，都没有SPSR（Saved Program Statement Register，已保存程序状态寄存器），但系统模式比用户模式有更高的权限，可以访问所有系统资源。
3.一般中断模式：
一般中断模式也叫普通中断模式，用于处理一般的中断请求，通常在硬件产生中断信号之后自动进入该模式，该模式为特权模式，可以自由访问系统硬件资源。
4.快速中断模式：
快速中断模式是相对一般中断模式而言的，它是用来处理对时间要求比较紧急的中断请求，主要用于高速数据传输及通道处理中。
5.管理模式（Supervisor，SVC） ：
管理模式是CPU上电后默认模式，因此在该模式下主要用来做系统的初始化，软中断处理也在该模式下。当用户模式下的用户程序请求使用硬件资源时，通过软件中断进入该模式。
说明：系统复位或开机、软中断时进入到SVC模式下。
6.终止模式：
中止模式用于支持虚拟内存或存储器保护，当用户程序访问非法地址，没有权限读取的内存地址时，会进入该模式，Linux下编程时经常出现的segment fault通常都是在该模式下抛出返回的。
7.未定义模式：
未定义模式用于支持硬件协处理器的软件仿真，CPU在指令的译码阶段不能识别该指令操作时，会进入未定义模式。
```

IRQ和FIQ有什么区别，在CPU里面是是怎么做的 interrupt request & fast interrupt request

	ARM的FIQ模式提供了更多的banked寄存器，r8到 r14还有SPSR，而IRQ模式就没有那么多，R8,R9,R10,R11,R12对应的banked的寄存器就没有，这就意味着在ARM的IRQ模式下，中断处理程序自己要保存R8到R12这几个寄存器，然后退出中断处理时程序要恢复这几个寄存器，而FIQ模式由于这几个寄存器都有banked寄存器，模式切换时CPU自动保存这些值到banked寄存器，退出FIQ模式时自动恢复，所以这个过程FIQ比IRQ快.
	FIQ比IRQ有更高优先级，如果FIQ和IRQ同时产生，那么FIQ先处理。
	在symbian系统里，当CPU处于FIQ模式处理FIQ 中断的过程中，预取指令异常，未定义指令异常，软件中断全被禁止，所有的中断被屏蔽。所以FIQ就会很快执行，不会被其他异常或者中断打断，所以它又比 IRQ快了。而IRQ不一样，当ARM处理IRQ模式处理IRQ中断时，如果来了一个FIQ中断请求，那正在执行的IRQ中断处理程序会被抢断，ARM切换到FIQ模式去执行这个FIQ，所以FIQ比IRQ快多了。

 linux 中断的上半部和下半部

softirq:
	

tasklet:
  

```
      Void my_tasklet_func(unsigned long)
        静态创建：static DECLARE_TASKLET(my_tasklet,my_tasklet_func,data)
        动态创建：DECLARE_TASKLET(my_tasklet,my_tasklet_func,data)
	Tasklet_schedule(&my_tasklet)
	struct timer_list mytimer;
	setup_timer(&mytimer, (*function)(unsigned long), unsigned long data);
	mytimer.expires = jiffies + 5*HZ;
```

定时器:
	

```
DEFINE_TIMER(timer_name, function_name, expires_value, data);

	struct timer_list mytimer;
	setup_timer(&mytimer, (*function)(unsigned long), unsigned long data);
	mytimer.expires = jiffies + 5*HZ;

	struct timer_list mytimer;
	init_timer(&mytimer);    
	mytimer ->timer.expires = jiffies + 5*HZ;
	mytimer ->timer.data = (unsigned long) dev;
	mytimer ->timer.function = &corkscrew_timer; /* timer handler */

	add_timer(struct timer_list *timer)
		mod_timer(struct timer_list *timer, unsigned long expires)

	del_timer(struct timer_list *timer) && del_timer_sync(struct timer_list *timer)
        
```

工作队列:
	http://www.cnblogs.com/wwang/archive/2010/10/27/1862202.html
	

```
1> 使用缺省的工作队列
	INIT_WORK(_work, _func, _data);//初始化指定工作，目的是把用户指定的函数_func及_func需要的参数_data赋给work_struct的func及data变量。
	int schedule_work(struct work_struct *work);//对工作进行调度，即把给定工作的处理函数提交给缺省的工作队列和工作者线程。工作者线程本质上是一个普通的内核线程，在默认情况下，每个CPU均有一个类型为“events”的工作者线程，当调用schedule_work时，这个工作者线程会被唤醒去执行工作链表上的所有工作。
	int schedule_delayed_work(struct work_struct *work, unsigned long delay);//延迟执行工作，与schedule_work类似。
	void flush_scheduled_work(void);//刷新缺省工作队列。此函数会一直等待，直到队列中的所有工作都被执行。
	int cancel_delayed_work(struct work_struct *work);//flush_scheduled_work并不取消任何延迟执行的工作，因此，如果要取消延迟工作，应该调用cancel_delayed_work。
	2> 自己创建一个工作队列
	struct workqueue_struct *create_workqueue(const char *name);//创建新的工作队列和相应的工作者线程，name用于该内核线程的命名
	int queue_work(struct workqueue_struct *wq, struct work_struct *work);//类似于schedule_work，区别在于queue_work把给定工作提交给创建的工作队列wq而不是缺省队列。
	int queue_delayed_work(struct workqueue_struct *wq, struct work_struct *work, unsigned long delay);//延迟执行工作。
	void flush_workqueue(struct workqueue_struct *wq);//刷新指定工作队列。
	void destroy_workqueue(struct workqueue_struct *wq);//释放创建的工作队列。

	INIT_WORK(struct work_struct *work, work_func_t func);
	INIT_DELAYED_WORK(struct delayed_work *work, work_func_t func);
	int schedule_work(struct work_struct *work);
	int schedule_delayed_work(struct delayed_work *work, unsigned long delay);
	struct workqueue_struct *create_workqueue(const char *name);
	int queue_work(struct workqueue_struct *wq, struct work_struct *work);
	int queue_delayed_work(struct workqueue_struct *wq, struct delayed_work *work, unsigned long delay);
	void flush_scheduled_work(void);
	void flush_workqueue(struct workqueue_struct *wq);
	int cancel_delayed_work(struct delayed_work *work);
	void destroy_workqueue(struct workqueue_struct *wq);
```

内核函数mmap的实现原理，机制
	

```
void *mmap(void *addr,size_t length ,int prot, int flags, int fd, off_t offset)
```
	文件映射是虚存的中心概念, 文件映射一方面给用户提供了一组措施, 好似用户将文件映射到自己地址空间的某个部分, 使用简单的内存访问指令读写文件；另一方面, 它也可以用于内核的基本组织模式, 在这种模式种, 内核将整个地址空间视为诸如文件之类的一组不同对象的映射. 中的传统文件访问方式是, 首先用open系统调用打开文件, 然后使用read, write以及lseek等调用进行顺序或者随即的I/O. 这种方式是非常低效的, 每一次I/O操作都需要一次系统调用. 另外, 如果若干个进程访问同一个文件, 每个进程都要在自己的地址空间维护一个副本, 浪费了内存空间. 而如果能够通过一定的机制将页面映射到进程的地址空间中, 也就是说首先通过简单的产生某些内存管理数据结构完成映射的创建. 当进程访问页面时产生一个缺页中断, 内核将页面读入内存并且更新页表指向该页面. 而且这种方式非常方便于同一副本的共享

驱动里面为什么要有并发、互斥的控制？如何实现？讲个例子

	并发（concurrency）指的是多个执行单元同时、并行被执行，而并发的执行单元对 共 享资源（硬件资源和软件上的全局变量、静态变量等）的访问则很容易导致竞态（race conditions） 。 解决竞态问题的途径是保证对共享资源的互斥访问， 所谓互斥访问就是指一个执行单 元 在访问共享资源的时候，其他的执行单元都被禁止访问。 访问共享资源的代码区域被称为临界区， 临界区需要以某种互斥机 制加以保护， 中断屏蔽， 原子操作，自旋锁，和信号量都是 linux 设备驱动中可采用的互斥途径

spinlock自旋锁是如何实现的

	自旋锁是一个互斥设备，它只有两个值：“锁定”和“解锁”。它通常实现为某个整数值中的某个位。希望获得某个特定锁得代码测试相关的位。如果锁可用，则“锁定”被设置，而代码继续进入临界区；相反，如果锁被其他人获得，则代码进入忙循环（而不是休眠，这也是自旋锁和一般锁的区别）并重复检查这个锁，直到该锁可用为止，这就是自旋的过程。“测试并设置位”的操作必须是原子的，这样，即使多个线程在给定时间自旋，也只有一个线程可获得该锁


